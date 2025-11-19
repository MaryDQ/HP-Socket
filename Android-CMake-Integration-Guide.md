# HP-Socket Android CMake集成详细指南

本指南详细介绍如何使用CMake方式在Android项目中集成HP-Socket库，支持UDP、TCP、HTTP等所有组件。

## 目录
1. [准备工作](#准备工作)
2. [方式一：作为单Module引入（推荐）](#方式一作为单module引入推荐)
3. [方式二：作为预编译库引入](#方式二作为预编译库引入)
4. [方式三：源码编译方式](#方式三源码编译方式)
5. [完整示例项目](#完整示例项目)
6. [常见问题](#常见问题)

---

## 准备工作

### 1. 编译HP-Socket库

首先需要编译适用于Android的HP-Socket库文件：

```bash
cd HP-Socket/Linux
./script/build-android-ndk.sh
```

编译完成后，库文件位于：
```
HP-Socket/Linux/lib/android-ndk/
├── arm64-v8a/
│   ├── libhpsocket.a      # C++静态库
│   ├── libhpsocket.so     # C++动态库
│   ├── libhpsocket4c.a    # C接口静态库
│   └── libhpsocket4c.so   # C接口动态库
├── armeabi-v7a/
│   └── ...
├── x86_64/
│   └── ...
└── x86/
    └── ...
```

### 2. 准备头文件

从源码中复制头文件目录：
```
HP-Socket/Linux/include/hpsocket/
├── HPSocket.h          # C++接口主头文件
├── HPSocket4C.h        # C接口主头文件
├── HPTypeDef.h         # 类型定义
├── SocketInterface.h   # 接口定义
└── GlobalDef.h         # 全局定义
```

---

## 方式一：作为单Module引入（推荐）

这是最推荐的方式，将HP-Socket作为一个独立的CMake模块集成到Android项目中。

### 项目结构

```
MyAndroidApp/
├── app/
│   ├── src/
│   │   └── main/
│   │       ├── cpp/
│   │       │   ├── native-lib.cpp       # 你的JNI代码
│   │       │   └── CMakeLists.txt       # 主CMakeLists
│   │       ├── java/
│   │       └── AndroidManifest.xml
│   └── build.gradle
├── hpsocket/                            # HP-Socket模块目录
│   ├── libs/                            # 预编译库文件
│   │   ├── arm64-v8a/
│   │   │   └── libhpsocket.a
│   │   ├── armeabi-v7a/
│   │   │   └── libhpsocket.a
│   │   ├── x86_64/
│   │   │   └── libhpsocket.a
│   │   └── x86/
│   │       └── libhpsocket.a
│   ├── include/                         # 头文件目录
│   │   └── hpsocket/
│   │       ├── HPSocket.h
│   │       ├── HPSocket4C.h
│   │       ├── HPTypeDef.h
│   │       ├── SocketInterface.h
│   │       └── GlobalDef.h
│   └── CMakeLists.txt                   # HP-Socket模块CMakeLists
├── build.gradle
└── settings.gradle
```

### 步骤1：创建hpsocket模块目录

在项目根目录下创建`hpsocket`文件夹，并将编译好的库文件和头文件复制进去：

```bash
mkdir -p hpsocket/libs/{arm64-v8a,armeabi-v7a,x86_64,x86}
mkdir -p hpsocket/include

# 复制库文件
cp HP-Socket/Linux/lib/android-ndk/arm64-v8a/libhpsocket.a hpsocket/libs/arm64-v8a/
cp HP-Socket/Linux/lib/android-ndk/armeabi-v7a/libhpsocket.a hpsocket/libs/armeabi-v7a/
cp HP-Socket/Linux/lib/android-ndk/x86_64/libhpsocket.a hpsocket/libs/x86_64/
cp HP-Socket/Linux/lib/android-ndk/x86/libhpsocket.a hpsocket/libs/x86/

# 复制头文件
cp -r HP-Socket/Linux/include/hpsocket hpsocket/include/
```

### 步骤2：创建hpsocket/CMakeLists.txt

在`hpsocket`目录下创建`CMakeLists.txt`文件：

```cmake
cmake_minimum_required(VERSION 3.4.1)

project(hpsocket)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 根据不同的ABI选择对应的库文件
if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(HPSOCKET_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/arm64-v8a")
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(HPSOCKET_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/armeabi-v7a")
elseif(ANDROID_ABI STREQUAL "x86_64")
    set(HPSOCKET_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/x86_64")
elseif(ANDROID_ABI STREQUAL "x86")
    set(HPSOCKET_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/x86")
else()
    message(FATAL_ERROR "Unsupported ABI: ${ANDROID_ABI}")
endif()

# 导入HP-Socket静态库
add_library(hpsocket STATIC IMPORTED GLOBAL)
set_target_properties(hpsocket PROPERTIES
    IMPORTED_LOCATION "${HPSOCKET_LIB_DIR}/libhpsocket.a"
)

# 设置包含目录
target_include_directories(hpsocket INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 设置链接库（HP-Socket依赖的系统库）
target_link_libraries(hpsocket INTERFACE
    dl      # 动态链接库
    z       # zlib压缩库
    log     # Android日志库
)

# 导出头文件路径，供其他模块使用
set(HPSOCKET_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
```

### 步骤3：修改app/src/main/cpp/CMakeLists.txt

在你的主CMakeLists.txt中引入hpsocket模块：

```cmake
cmake_minimum_required(VERSION 3.4.1)

# 添加hpsocket子目录
add_subdirectory(${CMAKE_SOURCE_DIR}/../../../hpsocket ${CMAKE_BINARY_DIR}/hpsocket)

# 创建你的native库
add_library(native-lib SHARED
    native-lib.cpp
)

# 查找Android日志库
find_library(log-lib log)

# 链接库
target_link_libraries(native-lib
    hpsocket          # HP-Socket库
    ${log-lib}        # Android日志库
)
```

### 步骤4：修改app/build.gradle

确保配置了正确的ABI和CMake路径：

```gradle
android {
    compileSdkVersion 33
    
    defaultConfig {
        applicationId "com.example.myapp"
        minSdkVersion 21
        targetSdkVersion 33
        
        externalNativeBuild {
            cmake {
                cppFlags "-std=c++14 -frtti -fexceptions"
                // 指定要编译的ABI
                abiFilters 'arm64-v8a', 'armeabi-v7a', 'x86_64', 'x86'
            }
        }
        
        ndk {
            // 也可以在这里指定ABI
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
    
    externalNativeBuild {
        cmake {
            path "src/main/cpp/CMakeLists.txt"
            version "3.18.1"  // 使用较新的CMake版本
        }
    }
    
    buildTypes {
        release {
            minifyEnabled false
            proguardFiles getDefaultProguardFile('proguard-android-optimize.txt'), 'proguard-rules.pro'
        }
    }
}
```

### 步骤5：编写JNI代码示例

在`app/src/main/cpp/native-lib.cpp`中使用HP-Socket：

```cpp
#include <jni.h>
#include <string>
#include <android/log.h>
#include <hpsocket/HPSocket.h>

#define TAG "HP-Socket-JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// UDP服务器监听器
class CUdpServerListener : public CUdpServerListener
{
public:
    virtual EnHandleResult OnPrepareListen(IUdpServer* pSender, SOCKET soListen) override
    {
        LOGD("OnPrepareListen");
        return HR_OK;
    }

    virtual EnHandleResult OnAccept(IUdpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override
    {
        LOGD("OnAccept: ConnID=%llu", (unsigned long long)dwConnID);
        return HR_OK;
    }

    virtual EnHandleResult OnReceive(IUdpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
    {
        LOGD("OnReceive: ConnID=%llu, Length=%d", (unsigned long long)dwConnID, iLength);
        
        // 回显数据
        if(pSender->Send(dwConnID, pData, iLength))
        {
            LOGD("Echo success");
        }
        else
        {
            LOGE("Echo failed: %d", pSender->GetLastError());
        }
        
        return HR_OK;
    }

    virtual EnHandleResult OnSend(IUdpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
    {
        LOGD("OnSend: ConnID=%llu, Length=%d", (unsigned long long)dwConnID, iLength);
        return HR_OK;
    }

    virtual EnHandleResult OnClose(IUdpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override
    {
        LOGD("OnClose: ConnID=%llu, ErrorCode=%d", (unsigned long long)dwConnID, iErrorCode);
        return HR_OK;
    }

    virtual EnHandleResult OnShutdown(IUdpServer* pSender) override
    {
        LOGD("OnShutdown");
        return HR_OK;
    }
};

// 全局变量
static CUdpServerListener* g_pListener = nullptr;
static CUdpServer* g_pServer = nullptr;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_myapp_MainActivity_startUdpServer(JNIEnv* env, jobject /* this */, jint port)
{
    if(g_pServer != nullptr)
    {
        LOGE("Server already started");
        return JNI_FALSE;
    }
    
    // 创建监听器和服务器
    g_pListener = new CUdpServerListener();
    g_pServer = new CUdpServer(g_pListener);
    
    // 启动服务器
    if(g_pServer->Start("0.0.0.0", (USHORT)port))
    {
        LOGD("UDP Server started on port %d", port);
        return JNI_TRUE;
    }
    else
    {
        LOGE("Failed to start server: %d - %s", 
             g_pServer->GetLastError(), 
             g_pServer->GetLastErrorDesc());
        
        delete g_pServer;
        delete g_pListener;
        g_pServer = nullptr;
        g_pListener = nullptr;
        
        return JNI_FALSE;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myapp_MainActivity_stopUdpServer(JNIEnv* env, jobject /* this */)
{
    if(g_pServer != nullptr)
    {
        g_pServer->Stop();
        delete g_pServer;
        delete g_pListener;
        g_pServer = nullptr;
        g_pListener = nullptr;
        
        LOGD("UDP Server stopped");
    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_myapp_MainActivity_getHPSocketVersion(JNIEnv* env, jobject /* this */)
{
    // 返回HP-Socket版本信息
    return env->NewStringUTF("HP-Socket 5.9.1 for Android");
}
```

### 步骤6：Java层调用示例

在`MainActivity.java`中：

```java
package com.example.myapp;

import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    
    static {
        System.loadLibrary("native-lib");
    }
    
    private boolean isServerRunning = false;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        TextView versionText = findViewById(R.id.version_text);
        versionText.setText(getHPSocketVersion());
        
        Button startBtn = findViewById(R.id.start_button);
        Button stopBtn = findViewById(R.id.stop_button);
        
        startBtn.setOnClickListener(v -> {
            if (!isServerRunning) {
                if (startUdpServer(5555)) {
                    Log.d(TAG, "Server started successfully");
                    isServerRunning = true;
                    startBtn.setEnabled(false);
                    stopBtn.setEnabled(true);
                } else {
                    Log.e(TAG, "Failed to start server");
                }
            }
        });
        
        stopBtn.setOnClickListener(v -> {
            if (isServerRunning) {
                stopUdpServer();
                Log.d(TAG, "Server stopped");
                isServerRunning = false;
                startBtn.setEnabled(true);
                stopBtn.setEnabled(false);
            }
        });
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (isServerRunning) {
            stopUdpServer();
        }
    }
    
    // Native方法声明
    public native boolean startUdpServer(int port);
    public native void stopUdpServer();
    public native String getHPSocketVersion();
}
```

### 步骤7：配置权限

在`AndroidManifest.xml`中添加必要权限：

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.myapp">
    
    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
    <uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
    
    <application
        android:allowBackup="true"
        android:icon="@mipmap/ic_launcher"
        android:label="@string/app_name"
        android:theme="@style/AppTheme">
        <activity android:name=".MainActivity">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
    
</manifest>
```

---

## 方式二：作为预编译库引入

如果不想将HP-Socket作为独立模块，也可以直接在主CMakeLists.txt中引入：

### app/src/main/cpp/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.4.1)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置HP-Socket库和头文件路径
set(HPSOCKET_DIR ${CMAKE_SOURCE_DIR}/../../../hpsocket)
set(HPSOCKET_INCLUDE_DIR ${HPSOCKET_DIR}/include)

# 根据ABI选择库文件路径
if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(HPSOCKET_LIB_PATH ${HPSOCKET_DIR}/libs/arm64-v8a/libhpsocket.a)
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(HPSOCKET_LIB_PATH ${HPSOCKET_DIR}/libs/armeabi-v7a/libhpsocket.a)
elseif(ANDROID_ABI STREQUAL "x86_64")
    set(HPSOCKET_LIB_PATH ${HPSOCKET_DIR}/libs/x86_64/libhpsocket.a)
elseif(ANDROID_ABI STREQUAL "x86")
    set(HPSOCKET_LIB_PATH ${HPSOCKET_DIR}/libs/x86/libhpsocket.a)
else()
    message(FATAL_ERROR "Unsupported ABI: ${ANDROID_ABI}")
endif()

# 检查库文件是否存在
if(NOT EXISTS ${HPSOCKET_LIB_PATH})
    message(FATAL_ERROR "HP-Socket library not found: ${HPSOCKET_LIB_PATH}")
endif()

# 导入HP-Socket库
add_library(hpsocket STATIC IMPORTED)
set_target_properties(hpsocket PROPERTIES
    IMPORTED_LOCATION ${HPSOCKET_LIB_PATH}
    INTERFACE_INCLUDE_DIRECTORIES ${HPSOCKET_INCLUDE_DIR}
)

# 创建你的native库
add_library(native-lib SHARED
    native-lib.cpp
)

# 包含头文件目录
target_include_directories(native-lib PRIVATE
    ${HPSOCKET_INCLUDE_DIR}
)

# 查找系统库
find_library(log-lib log)
find_library(z-lib z)

# 链接库
target_link_libraries(native-lib
    hpsocket
    ${log-lib}
    ${z-lib}
    dl
)
```

---

## 方式三：源码编译方式

如果希望直接编译HP-Socket源码而不使用预编译库：

### 项目结构

```
MyAndroidApp/
├── app/
│   └── src/main/cpp/
│       ├── native-lib.cpp
│       └── CMakeLists.txt
├── hpsocket/                    # HP-Socket源码
│   ├── src/                     # 源文件
│   ├── include/                 # 头文件
│   └── CMakeLists.txt
└── build.gradle
```

### hpsocket/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.4.1)

project(hpsocket)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 定义宏（根据需要启用/禁用功能）
add_definitions(
    # -D_UDP_DISABLED        # 禁用UDP（不推荐）
    # -D_SSL_DISABLED        # 禁用SSL
    # -D_HTTP_DISABLED       # 禁用HTTP
    # -D_ZLIB_DISABLED       # 禁用ZLIB
    # -D_BROTLI_DISABLED     # 禁用BROTLI
    # -D_ICONV_DISABLED      # 禁用ICONV
    -D_MIMALLOC_DISABLED     # 禁用mimalloc（Android上推荐禁用）
)

# 源文件列表
set(HPSOCKET_SOURCES
    # 公共模块
    src/common/BufferPool.cpp
    src/common/Event.cpp
    src/common/FileHelper.cpp
    src/common/FuncHelper.cpp
    src/common/IODispatcher.cpp
    src/common/PollHelper.cpp
    src/common/RWLock.cpp
    src/common/SysHelper.cpp
    src/common/Thread.cpp
    
    # KCP协议支持（UDP ARQ需要）
    src/common/kcp/ikcp.c
    
    # HTTP支持
    src/common/http/llhttp_api.c
    src/common/http/llhttp_support.c
    src/common/http/llhttp_internal.c
    src/common/http/llhttp_url.c
    
    # 核心模块
    src/ArqHelper.cpp
    src/HPThreadPool.cpp
    src/HttpAgent.cpp
    src/HttpClient.cpp
    src/HttpCookie.cpp
    src/HttpHelper.cpp
    src/HttpServer.cpp
    src/MiscHelper.cpp
    src/SocketHelper.cpp
    
    # TCP模块
    src/TcpAgent.cpp
    src/TcpClient.cpp
    src/TcpPackAgent.cpp
    src/TcpPackClient.cpp
    src/TcpPackServer.cpp
    src/TcpPullAgent.cpp
    src/TcpPullClient.cpp
    src/TcpPullServer.cpp
    src/TcpServer.cpp
    
    # UDP模块
    src/UdpArqClient.cpp
    src/UdpArqServer.cpp
    src/UdpCast.cpp
    src/UdpNode.cpp
    src/UdpClient.cpp
    src/UdpServer.cpp
    
    # 主接口
    src/HPSocket.cpp
)

# 创建静态库
add_library(hpsocket STATIC ${HPSOCKET_SOURCES})

# 设置包含目录
target_include_directories(hpsocket PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 编译选项
target_compile_options(hpsocket PRIVATE
    -fPIC
    -fvisibility=hidden
    -fno-strict-aliasing
    -Wall
    -Wextra
    -Wno-deprecated-declarations
    -Wno-unused-parameter
    -fexceptions
    -frtti
    -fthreadsafe-statics
)

# 链接依赖库
target_link_libraries(hpsocket
    dl
    z
    log
)
```

### app/src/main/cpp/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.4.1)

# 添加hpsocket子目录
add_subdirectory(${CMAKE_SOURCE_DIR}/../../../hpsocket ${CMAKE_BINARY_DIR}/hpsocket)

# 创建你的native库
add_library(native-lib SHARED
    native-lib.cpp
)

# 链接库
target_link_libraries(native-lib
    hpsocket
    log
)
```

---

## 完整示例项目

### UDP客户端示例

```cpp
// UdpClientWrapper.cpp
#include <jni.h>
#include <android/log.h>
#include <hpsocket/HPSocket.h>
#include <thread>
#include <memory>

#define TAG "UdpClient"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

class CUdpClientListener : public CUdpClientListener
{
public:
    virtual EnHandleResult OnPrepareConnect(IUdpClient* pSender, CONNID dwConnID, SOCKET socket) override
    {
        LOGD("OnPrepareConnect");
        return HR_OK;
    }
    
    virtual EnHandleResult OnConnect(IUdpClient* pSender, CONNID dwConnID) override
    {
        LOGD("OnConnect: ConnID=%llu", (unsigned long long)dwConnID);
        return HR_OK;
    }
    
    virtual EnHandleResult OnReceive(IUdpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
    {
        LOGD("OnReceive: Length=%d, Data=%.*s", iLength, iLength, (char*)pData);
        return HR_OK;
    }
    
    virtual EnHandleResult OnSend(IUdpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
    {
        LOGD("OnSend: Length=%d", iLength);
        return HR_OK;
    }
    
    virtual EnHandleResult OnClose(IUdpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override
    {
        LOGD("OnClose: ErrorCode=%d", iErrorCode);
        return HR_OK;
    }
};

static std::unique_ptr<CUdpClientListener> g_pClientListener;
static std::unique_ptr<CUdpClient> g_pClient;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_myapp_UdpClient_connect(JNIEnv* env, jobject /* this */, 
                                          jstring host, jint port)
{
    const char* szHost = env->GetStringUTFChars(host, nullptr);
    
    if(!g_pClient)
    {
        g_pClientListener = std::make_unique<CUdpClientListener>();
        g_pClient = std::make_unique<CUdpClient>(g_pClientListener.get());
    }
    
    bool result = g_pClient->Start(szHost, (USHORT)port);
    
    env->ReleaseStringUTFChars(host, szHost);
    
    LOGD("Connect result: %d", result);
    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_myapp_UdpClient_send(JNIEnv* env, jobject /* this */, jstring message)
{
    if(!g_pClient)
    {
        LOGD("Client not initialized");
        return JNI_FALSE;
    }
    
    const char* szMessage = env->GetStringUTFChars(message, nullptr);
    int length = env->GetStringUTFLength(message);
    
    bool result = g_pClient->Send((const BYTE*)szMessage, length);
    
    env->ReleaseStringUTFChars(message, szMessage);
    
    return result ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myapp_UdpClient_disconnect(JNIEnv* env, jobject /* this */)
{
    if(g_pClient)
    {
        g_pClient->Stop();
        g_pClient.reset();
        g_pClientListener.reset();
        LOGD("Client disconnected");
    }
}
```

---

## 常见问题

### 1. 编译错误：找不到头文件

**问题：** `fatal error: hpsocket/HPSocket.h: No such file or directory`

**解决方案：**
- 确认头文件路径配置正确
- 检查`include_directories`或`target_include_directories`设置
- 验证头文件确实存在于指定路径

### 2. 链接错误：undefined reference

**问题：** `undefined reference to 'CUdpServer::Start'`

**解决方案：**
- 确保正确链接了`libhpsocket.a`
- 检查ABI是否匹配（例如不要在arm64设备上使用x86的库）
- 确认`target_link_libraries`包含了hpsocket

### 3. 运行时错误：库加载失败

**问题：** `java.lang.UnsatisfiedLinkError: dlopen failed`

**解决方案：**
- 检查`build.gradle`中的`abiFilters`配置
- 确保所有依赖库都已正确链接（dl, z, log等）
- 验证库文件的架构与设备匹配

### 4. 禁用某些功能后的编译配置

如果禁用了SSL、HTTP等功能，需要在CMakeLists.txt中添加对应的宏定义：

```cmake
add_definitions(
    -D_SSL_DISABLED
    -D_HTTP_DISABLED
    -D_ICONV_DISABLED
)
```

同时在Java层调用时注意这些功能不可用。

### 5. 多ABI支持优化

为了减小APK大小，可以只编译常用的ABI：

```gradle
android {
    defaultConfig {
        ndk {
            // 只支持主流架构
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
}
```

### 6. ProGuard配置

如果启用了代码混淆，需要保留JNI方法：

```proguard
# 保留native方法
-keepclasseswithmembernames class * {
    native <methods>;
}

# 保留HP-Socket相关类
-keep class com.example.myapp.** { *; }
```

---

## 总结

本指南提供了三种在Android项目中使用CMake集成HP-Socket的方法：

1. **单Module方式（推荐）**：将HP-Socket作为独立CMake模块，结构清晰，易于维护
2. **预编译库方式**：直接在主CMakeLists.txt中引入，配置简单
3. **源码编译方式**：从源码编译，灵活性最高，但配置复杂

推荐使用**方式一（单Module方式）**，这样可以：
- 保持项目结构清晰
- 便于版本管理和升级
- 支持多个模块共享HP-Socket库
- 易于调试和维护

所有方式都完全支持HP-Socket的UDP模块及其他所有功能（TCP、HTTP、SSL等）。
