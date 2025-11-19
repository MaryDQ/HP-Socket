# HP-Socket Android CMake Integration Guide

This guide provides detailed instructions on how to integrate the HP-Socket library into Android projects using CMake, supporting all components including UDP, TCP, HTTP, etc.

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Method 1: Single Module Integration (Recommended)](#method-1-single-module-integration-recommended)
3. [Method 2: Prebuilt Library Integration](#method-2-prebuilt-library-integration)
4. [Method 3: Source Code Compilation](#method-3-source-code-compilation)
5. [Complete Example Project](#complete-example-project)
6. [FAQ](#faq)

---

## Prerequisites

### 1. Build HP-Socket Library

First, build the HP-Socket library for Android:

```bash
cd HP-Socket/Linux
./script/build-android-ndk.sh
```

After compilation, library files will be located in:
```
HP-Socket/Linux/lib/android-ndk/
├── arm64-v8a/
│   ├── libhpsocket.a      # C++ static library
│   ├── libhpsocket.so     # C++ dynamic library
│   ├── libhpsocket4c.a    # C interface static library
│   └── libhpsocket4c.so   # C interface dynamic library
├── armeabi-v7a/
│   └── ...
├── x86_64/
│   └── ...
└── x86/
    └── ...
```

### 2. Prepare Header Files

Copy the header file directory from source:
```
HP-Socket/Linux/include/hpsocket/
├── HPSocket.h          # C++ interface main header
├── HPSocket4C.h        # C interface main header
├── HPTypeDef.h         # Type definitions
├── SocketInterface.h   # Interface definitions
└── GlobalDef.h         # Global definitions
```

---

## Method 1: Single Module Integration (Recommended)

This is the recommended approach - integrate HP-Socket as an independent CMake module into your Android project.

### Project Structure

```
MyAndroidApp/
├── app/
│   ├── src/
│   │   └── main/
│   │       ├── cpp/
│   │       │   ├── native-lib.cpp       # Your JNI code
│   │       │   └── CMakeLists.txt       # Main CMakeLists
│   │       ├── java/
│   │       └── AndroidManifest.xml
│   └── build.gradle
├── hpsocket/                            # HP-Socket module directory
│   ├── libs/                            # Prebuilt library files
│   │   ├── arm64-v8a/
│   │   │   └── libhpsocket.a
│   │   ├── armeabi-v7a/
│   │   │   └── libhpsocket.a
│   │   ├── x86_64/
│   │   │   └── libhpsocket.a
│   │   └── x86/
│   │       └── libhpsocket.a
│   ├── include/                         # Header files directory
│   │   └── hpsocket/
│   │       ├── HPSocket.h
│   │       ├── HPSocket4C.h
│   │       ├── HPTypeDef.h
│   │       ├── SocketInterface.h
│   │       └── GlobalDef.h
│   └── CMakeLists.txt                   # HP-Socket module CMakeLists
├── build.gradle
└── settings.gradle
```

### Step 1: Create hpsocket Module Directory

Create `hpsocket` folder in project root and copy library files and headers:

```bash
mkdir -p hpsocket/libs/{arm64-v8a,armeabi-v7a,x86_64,x86}
mkdir -p hpsocket/include

# Copy library files
cp HP-Socket/Linux/lib/android-ndk/arm64-v8a/libhpsocket.a hpsocket/libs/arm64-v8a/
cp HP-Socket/Linux/lib/android-ndk/armeabi-v7a/libhpsocket.a hpsocket/libs/armeabi-v7a/
cp HP-Socket/Linux/lib/android-ndk/x86_64/libhpsocket.a hpsocket/libs/x86_64/
cp HP-Socket/Linux/lib/android-ndk/x86/libhpsocket.a hpsocket/libs/x86/

# Copy header files
cp -r HP-Socket/Linux/include/hpsocket hpsocket/include/
```

### Step 2: Create hpsocket/CMakeLists.txt

Create `CMakeLists.txt` in the `hpsocket` directory:

```cmake
cmake_minimum_required(VERSION 3.4.1)

project(hpsocket)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Select library based on ABI
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

# Import HP-Socket static library
add_library(hpsocket STATIC IMPORTED GLOBAL)
set_target_properties(hpsocket PROPERTIES
    IMPORTED_LOCATION "${HPSOCKET_LIB_DIR}/libhpsocket.a"
)

# Set include directories
target_include_directories(hpsocket INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set link libraries (system libraries that HP-Socket depends on)
target_link_libraries(hpsocket INTERFACE
    dl      # Dynamic linking library
    z       # zlib compression library
    log     # Android log library
)

# Export include directory for other modules
set(HPSOCKET_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
```

### Step 3: Modify app/src/main/cpp/CMakeLists.txt

Add hpsocket module in your main CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.4.1)

# Add hpsocket subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/../../../hpsocket ${CMAKE_BINARY_DIR}/hpsocket)

# Create your native library
add_library(native-lib SHARED
    native-lib.cpp
)

# Find Android log library
find_library(log-lib log)

# Link libraries
target_link_libraries(native-lib
    hpsocket          # HP-Socket library
    ${log-lib}        # Android log library
)
```

### Step 4: Modify app/build.gradle

Ensure correct ABI and CMake path configuration:

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
                // Specify ABIs to build
                abiFilters 'arm64-v8a', 'armeabi-v7a', 'x86_64', 'x86'
            }
        }
        
        ndk {
            // Can also specify ABIs here
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
    
    externalNativeBuild {
        cmake {
            path "src/main/cpp/CMakeLists.txt"
            version "3.18.1"  // Use newer CMake version
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

### Step 5: JNI Code Example

Use HP-Socket in `app/src/main/cpp/native-lib.cpp`:

```cpp
#include <jni.h>
#include <string>
#include <android/log.h>
#include <hpsocket/HPSocket.h>

#define TAG "HP-Socket-JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// UDP Server Listener
class CUdpServerListenerImpl : public CUdpServerListener
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
        
        // Echo data
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

// Global variables
static CUdpServerListenerImpl* g_pListener = nullptr;
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
    
    // Create listener and server
    g_pListener = new CUdpServerListenerImpl();
    g_pServer = new CUdpServer(g_pListener);
    
    // Start server
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
    // Return HP-Socket version info
    return env->NewStringUTF("HP-Socket 5.9.1 for Android");
}
```

### Step 6: Java Layer Example

In `MainActivity.java`:

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
    
    // Native method declarations
    public native boolean startUdpServer(int port);
    public native void stopUdpServer();
    public native String getHPSocketVersion();
}
```

### Step 7: Configure Permissions

Add necessary permissions in `AndroidManifest.xml`:

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

## Method 2: Prebuilt Library Integration

If you don't want HP-Socket as a separate module, import it directly in main CMakeLists.txt:

### app/src/main/cpp/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.4.1)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set HP-Socket library and header paths
set(HPSOCKET_DIR ${CMAKE_SOURCE_DIR}/../../../hpsocket)
set(HPSOCKET_INCLUDE_DIR ${HPSOCKET_DIR}/include)

# Select library path based on ABI
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

# Check if library exists
if(NOT EXISTS ${HPSOCKET_LIB_PATH})
    message(FATAL_ERROR "HP-Socket library not found: ${HPSOCKET_LIB_PATH}")
endif()

# Import HP-Socket library
add_library(hpsocket STATIC IMPORTED)
set_target_properties(hpsocket PROPERTIES
    IMPORTED_LOCATION ${HPSOCKET_LIB_PATH}
    INTERFACE_INCLUDE_DIRECTORIES ${HPSOCKET_INCLUDE_DIR}
)

# Create your native library
add_library(native-lib SHARED
    native-lib.cpp
)

# Include directories
target_include_directories(native-lib PRIVATE
    ${HPSOCKET_INCLUDE_DIR}
)

# Find system libraries
find_library(log-lib log)
find_library(z-lib z)

# Link libraries
target_link_libraries(native-lib
    hpsocket
    ${log-lib}
    ${z-lib}
    dl
)
```

---

## Method 3: Source Code Compilation

If you prefer to compile HP-Socket source directly instead of using prebuilt libraries:

### Project Structure

```
MyAndroidApp/
├── app/
│   └── src/main/cpp/
│       ├── native-lib.cpp
│       └── CMakeLists.txt
├── hpsocket/                    # HP-Socket source
│   ├── src/                     # Source files
│   ├── include/                 # Header files
│   └── CMakeLists.txt
└── build.gradle
```

### hpsocket/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.4.1)

project(hpsocket)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define macros (enable/disable features as needed)
add_definitions(
    # -D_UDP_DISABLED        # Disable UDP (not recommended)
    # -D_SSL_DISABLED        # Disable SSL
    # -D_HTTP_DISABLED       # Disable HTTP
    # -D_ZLIB_DISABLED       # Disable ZLIB
    # -D_BROTLI_DISABLED     # Disable BROTLI
    # -D_ICONV_DISABLED      # Disable ICONV
    -D_MIMALLOC_DISABLED     # Disable mimalloc (recommended on Android)
)

# Source file list
set(HPSOCKET_SOURCES
    # Common modules
    src/common/BufferPool.cpp
    src/common/Event.cpp
    src/common/FileHelper.cpp
    src/common/FuncHelper.cpp
    src/common/IODispatcher.cpp
    src/common/PollHelper.cpp
    src/common/RWLock.cpp
    src/common/SysHelper.cpp
    src/common/Thread.cpp
    
    # KCP protocol support (needed for UDP ARQ)
    src/common/kcp/ikcp.c
    
    # HTTP support
    src/common/http/llhttp_api.c
    src/common/http/llhttp_support.c
    src/common/http/llhttp_internal.c
    src/common/http/llhttp_url.c
    
    # Core modules
    src/ArqHelper.cpp
    src/HPThreadPool.cpp
    src/HttpAgent.cpp
    src/HttpClient.cpp
    src/HttpCookie.cpp
    src/HttpHelper.cpp
    src/HttpServer.cpp
    src/MiscHelper.cpp
    src/SocketHelper.cpp
    
    # TCP modules
    src/TcpAgent.cpp
    src/TcpClient.cpp
    src/TcpPackAgent.cpp
    src/TcpPackClient.cpp
    src/TcpPackServer.cpp
    src/TcpPullAgent.cpp
    src/TcpPullClient.cpp
    src/TcpPullServer.cpp
    src/TcpServer.cpp
    
    # UDP modules
    src/UdpArqClient.cpp
    src/UdpArqServer.cpp
    src/UdpCast.cpp
    src/UdpNode.cpp
    src/UdpClient.cpp
    src/UdpServer.cpp
    
    # Main interface
    src/HPSocket.cpp
)

# Create static library
add_library(hpsocket STATIC ${HPSOCKET_SOURCES})

# Set include directories
target_include_directories(hpsocket PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile options
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

# Link dependencies
target_link_libraries(hpsocket
    dl
    z
    log
)
```

---

## FAQ

### 1. Compilation Error: Header File Not Found

**Problem:** `fatal error: hpsocket/HPSocket.h: No such file or directory`

**Solution:**
- Verify header file path is configured correctly
- Check `include_directories` or `target_include_directories` settings
- Confirm header files exist at specified path

### 2. Linking Error: undefined reference

**Problem:** `undefined reference to 'CUdpServer::Start'`

**Solution:**
- Ensure `libhpsocket.a` is linked correctly
- Check ABI matches (don't use x86 library on arm64 device)
- Confirm `target_link_libraries` includes hpsocket

### 3. Runtime Error: Library Load Failed

**Problem:** `java.lang.UnsatisfiedLinkError: dlopen failed`

**Solution:**
- Check `abiFilters` configuration in `build.gradle`
- Ensure all dependent libraries are linked correctly (dl, z, log, etc.)
- Verify library architecture matches device

### 4. Compilation Configuration After Disabling Features

If you disabled SSL, HTTP, or other features, add corresponding macro definitions in CMakeLists.txt:

```cmake
add_definitions(
    -D_SSL_DISABLED
    -D_HTTP_DISABLED
    -D_ICONV_DISABLED
)
```

Also note these features are unavailable when calling from Java layer.

### 5. Multi-ABI Support Optimization

To reduce APK size, compile only for common ABIs:

```gradle
android {
    defaultConfig {
        ndk {
            // Only support mainstream architectures
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
}
```

### 6. ProGuard Configuration

If code obfuscation is enabled, preserve JNI methods:

```proguard
# Keep native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep HP-Socket related classes
-keep class com.example.myapp.** { *; }
```

---

## Summary

This guide provides three methods for integrating HP-Socket into Android projects using CMake:

1. **Single Module Method (Recommended)**: HP-Socket as independent CMake module - clear structure, easy to maintain
2. **Prebuilt Library Method**: Direct import in main CMakeLists.txt - simple configuration
3. **Source Compilation Method**: Compile from source - most flexible but complex configuration

**Method 1 (Single Module)** is recommended because:
- Maintains clear project structure
- Easy version management and upgrades
- Supports sharing HP-Socket library across multiple modules
- Easy to debug and maintain

All methods fully support HP-Socket's UDP module and all other features (TCP, HTTP, SSL, etc.).
