/**
 * UDP Server JNI封装
 * 包名需要根据实际项目修改：com.example.hpsocket
 */

#include <jni.h>
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <hpsocket/HPSocket.h>
#include "common.h"

// UDP Server监听器实现
class CUdpServerListenerImpl : public CUdpServerListener {
private:
    JavaVM* jvm;
    jobject javaCallbackObj;  // Java层的回调对象（全局引用）
    
public:
    CUdpServerListenerImpl(JavaVM* vm, jobject callback) 
        : jvm(vm), javaCallbackObj(nullptr) {
        if (callback) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                javaCallbackObj = env->NewGlobalRef(callback);
            }
        }
    }
    
    ~CUdpServerListenerImpl() {
        if (javaCallbackObj) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                env->DeleteGlobalRef(javaCallbackObj);
            }
        }
    }
    
    virtual EnHandleResult OnPrepareListen(IUdpServer* pSender, SOCKET soListen) override {
        LOGD("OnPrepareListen");
        return HR_OK;
    }
    
    virtual EnHandleResult OnAccept(IUdpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override {
        LOGD("OnAccept: ConnID=%llu", (unsigned long long)dwConnID);
        
        // 调用Java层回调
        if (javaCallbackObj) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                CallJavaVoidMethod(env, javaCallbackObj, "onAccept", "(J)V", (jlong)dwConnID);
            }
        }
        
        return HR_OK;
    }
    
    virtual EnHandleResult OnReceive(IUdpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override {
        LOGD("OnReceive: ConnID=%llu, Length=%d", (unsigned long long)dwConnID, iLength);
        
        // 调用Java层回调
        if (javaCallbackObj && pData && iLength > 0) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                jbyteArray jdata = env->NewByteArray(iLength);
                if (jdata) {
                    env->SetByteArrayRegion(jdata, 0, iLength, (const jbyte*)pData);
                    CallJavaVoidMethod(env, javaCallbackObj, "onReceive", "(J[B)V", 
                                     (jlong)dwConnID, jdata);
                    env->DeleteLocalRef(jdata);
                }
            }
        }
        
        return HR_OK;
    }
    
    virtual EnHandleResult OnSend(IUdpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override {
        LOGD("OnSend: ConnID=%llu, Length=%d", (unsigned long long)dwConnID, iLength);
        
        // 调用Java层回调
        if (javaCallbackObj) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                CallJavaVoidMethod(env, javaCallbackObj, "onSend", "(JI)V", 
                                 (jlong)dwConnID, (jint)iLength);
            }
        }
        
        return HR_OK;
    }
    
    virtual EnHandleResult OnClose(IUdpServer* pSender, CONNID dwConnID, 
                                   EnSocketOperation enOperation, int iErrorCode) override {
        LOGD("OnClose: ConnID=%llu, ErrorCode=%d", (unsigned long long)dwConnID, iErrorCode);
        
        // 调用Java层回调
        if (javaCallbackObj) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                CallJavaVoidMethod(env, javaCallbackObj, "onClose", "(JI)V", 
                                 (jlong)dwConnID, (jint)iErrorCode);
            }
        }
        
        return HR_OK;
    }
    
    virtual EnHandleResult OnShutdown(IUdpServer* pSender) override {
        LOGD("OnShutdown");
        
        // 调用Java层回调
        if (javaCallbackObj) {
            JNIEnv* env = nullptr;
            jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (env) {
                CallJavaVoidMethod(env, javaCallbackObj, "onShutdown", "()V");
            }
        }
        
        return HR_OK;
    }
};

// 全局Server管理
static std::mutex g_serverMutex;
static std::map<jlong, std::unique_ptr<CUdpServer>> g_servers;
static std::map<jlong, std::unique_ptr<CUdpServerListenerImpl>> g_listeners;
static jlong g_nextServerHandle = 1;

extern "C" {

/**
 * 创建UDP Server
 * Java签名: native long nativeCreate(Object callback);
 */
JNIEXPORT jlong JNICALL
Java_com_example_hpsocket_UdpServer_nativeCreate(JNIEnv* env, jobject thiz, jobject callback) {
    std::lock_guard<std::mutex> lock(g_serverMutex);
    
    JavaVM* jvm = nullptr;
    env->GetJavaVM(&jvm);
    
    jlong handle = g_nextServerHandle++;
    
    try {
        auto listener = std::make_unique<CUdpServerListenerImpl>(jvm, callback);
        auto server = std::make_unique<CUdpServer>(listener.get());
        
        g_listeners[handle] = std::move(listener);
        g_servers[handle] = std::move(server);
        
        LOGI("UDP Server created with handle: %lld", (long long)handle);
        return handle;
    } catch (const std::exception& e) {
        LOGE("Failed to create UDP Server: %s", e.what());
        return 0;
    }
}

/**
 * 启动UDP Server
 * Java签名: native boolean nativeStart(long handle, String bindAddr, int port);
 */
JNIEXPORT jboolean JNICALL
Java_com_example_hpsocket_UdpServer_nativeStart(JNIEnv* env, jobject thiz, 
                                                 jlong handle, jstring bindAddr, jint port) {
    std::lock_guard<std::mutex> lock(g_serverMutex);
    
    auto it = g_servers.find(handle);
    if (it == g_servers.end()) {
        LOGE("Invalid server handle: %lld", (long long)handle);
        return JNI_FALSE;
    }
    
    const char* addr = env->GetStringUTFChars(bindAddr, nullptr);
    if (!addr) {
        LOGE("Failed to get bind address string");
        return JNI_FALSE;
    }
    
    bool result = it->second->Start(addr, (USHORT)port);
    
    if (result) {
        LOGI("UDP Server started on %s:%d", addr, port);
    } else {
        LOGE("Failed to start UDP Server: %d - %s", 
             it->second->GetLastError(), 
             it->second->GetLastErrorDesc());
    }
    
    env->ReleaseStringUTFChars(bindAddr, addr);
    return result ? JNI_TRUE : JNI_FALSE;
}

/**
 * 停止UDP Server
 * Java签名: native boolean nativeStop(long handle);
 */
JNIEXPORT jboolean JNICALL
Java_com_example_hpsocket_UdpServer_nativeStop(JNIEnv* env, jobject thiz, jlong handle) {
    std::lock_guard<std::mutex> lock(g_serverMutex);
    
    auto it = g_servers.find(handle);
    if (it == g_servers.end()) {
        LOGE("Invalid server handle: %lld", (long long)handle);
        return JNI_FALSE;
    }
    
    bool result = it->second->Stop();
    
    if (result) {
        LOGI("UDP Server stopped");
    } else {
        LOGE("Failed to stop UDP Server: %d", it->second->GetLastError());
    }
    
    return result ? JNI_TRUE : JNI_FALSE;
}

/**
 * 发送数据
 * Java签名: native boolean nativeSend(long handle, long connId, byte[] data);
 */
JNIEXPORT jboolean JNICALL
Java_com_example_hpsocket_UdpServer_nativeSend(JNIEnv* env, jobject thiz, 
                                                jlong handle, jlong connId, jbyteArray data) {
    std::lock_guard<std::mutex> lock(g_serverMutex);
    
    auto it = g_servers.find(handle);
    if (it == g_servers.end()) {
        LOGE("Invalid server handle: %lld", (long long)handle);
        return JNI_FALSE;
    }
    
    if (!data) {
        LOGE("Data is null");
        return JNI_FALSE;
    }
    
    jsize length = env->GetArrayLength(data);
    jbyte* bytes = env->GetByteArrayElements(data, nullptr);
    
    bool result = it->second->Send((CONNID)connId, (const BYTE*)bytes, (int)length);
    
    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
    
    if (!result) {
        LOGE("Failed to send data: %d", it->second->GetLastError());
    }
    
    return result ? JNI_TRUE : JNI_FALSE;
}

/**
 * 销毁UDP Server
 * Java签名: native void nativeDestroy(long handle);
 */
JNIEXPORT void JNICALL
Java_com_example_hpsocket_UdpServer_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
    std::lock_guard<std::mutex> lock(g_serverMutex);
    
    auto it = g_servers.find(handle);
    if (it != g_servers.end()) {
        it->second->Stop();
        g_servers.erase(it);
    }
    
    auto lit = g_listeners.find(handle);
    if (lit != g_listeners.end()) {
        g_listeners.erase(lit);
    }
    
    LOGI("UDP Server destroyed: %lld", (long long)handle);
}

/**
 * 获取连接数
 * Java签名: native int nativeGetConnectionCount(long handle);
 */
JNIEXPORT jint JNICALL
Java_com_example_hpsocket_UdpServer_nativeGetConnectionCount(JNIEnv* env, jobject thiz, jlong handle) {
    std::lock_guard<std::mutex> lock(g_serverMutex);
    
    auto it = g_servers.find(handle);
    if (it == g_servers.end()) {
        return 0;
    }
    
    return (jint)it->second->GetConnectionCount();
}

} // extern "C"
