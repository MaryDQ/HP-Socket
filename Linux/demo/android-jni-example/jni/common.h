#ifndef HPSOCKET_JNI_COMMON_H
#define HPSOCKET_JNI_COMMON_H

#include <jni.h>
#include <android/log.h>

// 日志宏定义
#define TAG "HPSocket-JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// JNI辅助函数：调用Java回调方法
inline void CallJavaVoidMethod(JNIEnv* env, jobject obj, const char* methodName, const char* signature, ...) {
    if (!env || !obj) return;
    
    jclass cls = env->GetObjectClass(obj);
    if (!cls) {
        LOGE("Failed to get class for callback");
        return;
    }
    
    jmethodID mid = env->GetMethodID(cls, methodName, signature);
    env->DeleteLocalRef(cls);
    
    if (!mid) {
        LOGE("Failed to get method %s with signature %s", methodName, signature);
        return;
    }
    
    va_list args;
    va_start(args, signature);
    env->CallVoidMethodV(obj, mid, args);
    va_end(args);
    
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

#endif // HPSOCKET_JNI_COMMON_H
