/**
 * UDP Client JNI封装
 * 包名需要根据实际项目修改：com.example.hpsocket
 */

#include <jni.h>
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <hpsocket/HPSocket.h>
#include "common.h"

// UDP Client监听器实现
class CUdpClientListenerImpl : public CUdpClientListener {
private:
    JavaVM* jvm;
    jobject javaCallbackObj;  // Java层的回调对象（全局引用）
    
public:
    CUdpClientListenerImpl(JavaVM* vm, jobject callback) 
        : jvm(vm), javaCallbackObj(nullptr) {
        if (callback) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                javaCallbackObj = env->NewGlobalRef(callback);
            }
        }
    }
    
    ~CUdpClientListenerImpl() {
        if (javaCallbackObj) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                env->DeleteGlobalRef(javaCallbackObj);
            }
        }
    }
    
    virtual EnHandleResult OnPrepareConnect(IUdpClient* pSender, CONNID dwConnID, SOCKET socket) override {
        LOGD("OnPrepareConnect");
        return HR_OK;
    }
    
    virtual EnHandleResult OnConnect(IUdpClient* pSender, CONNID dwConnID) override {
        LOGD("OnConnect: ConnID=%llu", (unsigned long long)dwConnID);
        
        // 调用Java层回调
        if (javaCallbackObj) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                CallJavaVoidMethod(env, javaCallbackObj, "onConnect", "()V");
            }
        }
        
        return HR_OK;
    }
    
    virtual EnHandleResult OnReceive(IUdpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override {
        LOGD("OnReceive: Length=%d", iLength);
        
        // 调用Java层回调
        if (javaCallbackObj && pData && iLength > 0) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                jbyteArray jdata = env->NewByteArray(iLength);
                if (jdata) {
                    env->SetByteArrayRegion(jdata, 0, iLength, (const jbyte*)pData);
                    CallJavaVoidMethod(env, javaCallbackObj, "onReceive", "([B)V", jdata);
                    env->DeleteLocalRef(jdata);
                }
            }
        }
        
        return HR_OK;
    }
    
    virtual EnHandleResult OnSend(IUdpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override {
        LOGD("OnSend: Length=%d", iLength);
        
        // 调用Java层回调
        if (javaCallbackObj) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                CallJavaVoidMethod(env, javaCallbackObj, "onSend", "(I)V", (jint)iLength);
            }
        }
        
        return HR_OK;
    }
    
    virtual EnHandleResult OnClose(IUdpClient* pSender, CONNID dwConnID, 
                                   EnSocketOperation enOperation, int iErrorCode) override {
        LOGD("OnClose: ErrorCode=%d", iErrorCode);
        
        // 调用Java层回调
        if (javaCallbackObj) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                CallJavaVoidMethod(env, javaCallbackObj, "onClose", "(I)V", (jint)iErrorCode);
            }
        }
        
        return HR_OK;
    }
};

// 全局Client管理
static std::mutex g_clientMutex;
static std::map<jlong, std::unique_ptr<CUdpClient>> g_clients;
static std::map<jlong, std::unique_ptr<CUdpClientListenerImpl>> g_clientListeners;
static jlong g_nextClientHandle = 1;

extern "C" {

/**
 * 创建UDP Client
 * Java签名: native long nativeCreate(Object callback);
 */
JNIEXPORT jlong JNICALL
Java_com_example_hpsocket_UdpClient_nativeCreate(JNIEnv* env, jobject thiz, jobject callback) {
    std::lock_guard<std::mutex> lock(g_clientMutex);
    
    JavaVM* jvm = nullptr;
    env->GetJavaVM(&jvm);
    
    jlong handle = g_nextClientHandle++;
    
    try {
        auto listener = std::make_unique<CUdpClientListenerImpl>(jvm, callback);
        auto client = std::make_unique<CUdpClient>(listener.get());
        
        g_clientListeners[handle] = std::move(listener);
        g_clients[handle] = std::move(client);
        
        LOGI("UDP Client created with handle: %lld", (long long)handle);
        return handle;
    } catch (const std::exception& e) {
        LOGE("Failed to create UDP Client: %s", e.what());
        return 0;
    }
}

/**
 * 连接到服务器
 * Java签名: native boolean nativeConnect(long handle, String host, int port);
 */
JNIEXPORT jboolean JNICALL
Java_com_example_hpsocket_UdpClient_nativeConnect(JNIEnv* env, jobject thiz, 
                                                   jlong handle, jstring host, jint port) {
    std::lock_guard<std::mutex> lock(g_clientMutex);
    
    auto it = g_clients.find(handle);
    if (it == g_clients.end()) {
        LOGE("Invalid client handle: %lld", (long long)handle);
        return JNI_FALSE;
    }
    
    const char* hostStr = env->GetStringUTFChars(host, nullptr);
    if (!hostStr) {
        LOGE("Failed to get host string");
        return JNI_FALSE;
    }
    
    bool result = it->second->Start(hostStr, (USHORT)port);
    
    if (result) {
        LOGI("UDP Client connected to %s:%d", hostStr, port);
    } else {
        LOGE("Failed to connect: %d - %s", 
             it->second->GetLastError(), 
             it->second->GetLastErrorDesc());
    }
    
    env->ReleaseStringUTFChars(host, hostStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

/**
 * 断开连接
 * Java签名: native boolean nativeDisconnect(long handle);
 */
JNIEXPORT jboolean JNICALL
Java_com_example_hpsocket_UdpClient_nativeDisconnect(JNIEnv* env, jobject thiz, jlong handle) {
    std::lock_guard<std::mutex> lock(g_clientMutex);
    
    auto it = g_clients.find(handle);
    if (it == g_clients.end()) {
        LOGE("Invalid client handle: %lld", (long long)handle);
        return JNI_FALSE;
    }
    
    bool result = it->second->Stop();
    
    if (result) {
        LOGI("UDP Client disconnected");
    } else {
        LOGE("Failed to disconnect: %d", it->second->GetLastError());
    }
    
    return result ? JNI_TRUE : JNI_FALSE;
}

/**
 * 发送数据
 * Java签名: native boolean nativeSend(long handle, byte[] data);
 */
JNIEXPORT jboolean JNICALL
Java_com_example_hpsocket_UdpClient_nativeSend(JNIEnv* env, jobject thiz, 
                                                jlong handle, jbyteArray data) {
    std::lock_guard<std::mutex> lock(g_clientMutex);
    
    auto it = g_clients.find(handle);
    if (it == g_clients.end()) {
        LOGE("Invalid client handle: %lld", (long long)handle);
        return JNI_FALSE;
    }
    
    if (!data) {
        LOGE("Data is null");
        return JNI_FALSE;
    }
    
    jsize length = env->GetArrayLength(data);
    jbyte* bytes = env->GetByteArrayElements(data, nullptr);
    
    bool result = it->second->Send((const BYTE*)bytes, (int)length);
    
    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
    
    if (!result) {
        LOGE("Failed to send data: %d", it->second->GetLastError());
    }
    
    return result ? JNI_TRUE : JNI_FALSE;
}

/**
 * 销毁UDP Client
 * Java签名: native void nativeDestroy(long handle);
 */
JNIEXPORT void JNICALL
Java_com_example_hpsocket_UdpClient_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
    std::lock_guard<std::mutex> lock(g_clientMutex);
    
    auto it = g_clients.find(handle);
    if (it != g_clients.end()) {
        it->second->Stop();
        g_clients.erase(it);
    }
    
    auto lit = g_clientListeners.find(handle);
    if (lit != g_clientListeners.end()) {
        g_clientListeners.erase(lit);
    }
    
    LOGI("UDP Client destroyed: %lld", (long long)handle);
}

/**
 * 检查是否已连接
 * Java签名: native boolean nativeIsConnected(long handle);
 */
JNIEXPORT jboolean JNICALL
Java_com_example_hpsocket_UdpClient_nativeIsConnected(JNIEnv* env, jobject thiz, jlong handle) {
    std::lock_guard<std::mutex> lock(g_clientMutex);
    
    auto it = g_clients.find(handle);
    if (it == g_clients.end()) {
        return JNI_FALSE;
    }
    
    return (it->second->GetState() == SS_STARTED) ? JNI_TRUE : JNI_FALSE;
}

} // extern "C"
