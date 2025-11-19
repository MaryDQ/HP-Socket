# HP-Socket UDP Module Android Support Guide

## Conclusion

**HP-Socket's UDP module fully supports the Android platform.**

## Supported UDP Components

HP-Socket provides the following UDP components for Android platform:

1. **UdpServer** - UDP server component
2. **UdpClient** - UDP client component
3. **UdpCast** - UDP multicast component
4. **UdpNode** - UDP node component (can act as both server and client)
5. **UdpArqServer** - Reliable UDP server based on ARQ protocol
6. **UdpArqClient** - Reliable UDP client based on ARQ protocol

## Implementation Methods for Android Platform

### Method 1: Build HP-Socket Library (Recommended)

#### 1. Environment Setup

Ensure Android NDK is installed and environment variables are configured.

#### 2. Build HP-Socket Library

Navigate to the Linux directory and execute the Android NDK build script:

```bash
cd HP-Socket/Linux
./script/build-android-ndk.sh
```

#### 3. Build Options

**Build all ABI architectures by default:**
- arm64-v8a
- armeabi-v7a
- x86_64
- x86

**Specify specific ABI architectures:**
```bash
./script/build-android-ndk.sh APP_ABI=armeabi-v7a,arm64-v8a
```

**Disable specific features (if not needed):**
```bash
# Build only UDP, disable SSL and HTTP
./script/build-android-ndk.sh _SSL_DISABLED=true _HTTP_DISABLED=true
```

**Note:** UDP functionality is enabled by default. To disable UDP (not recommended), use:
```bash
./script/build-android-ndk.sh _UDP_DISABLED=true
```

#### 4. Output Files

After compilation, library files will be output to:
```
HP-Socket/Linux/lib/android-ndk/
├── arm64-v8a/
│   ├── libhpsocket.a      # C++ static library
│   ├── libhpsocket.so     # C++ dynamic library
│   ├── libhpsocket4c.a    # C interface static library
│   └── libhpsocket4c.so   # C interface dynamic library
├── armeabi-v7a/
├── x86_64/
└── x86/
```

### Method 2: Build UDP Sample Programs

HP-Socket provides complete UDP sample programs that can be compiled and run directly on Android.

#### 1. Build All Samples

```bash
cd HP-Socket/Linux/demo
./build-android-ndk-demo.sh
```

#### 2. Build Specific UDP Samples

UDP sample programs include:
- **testecho-udp/server** - UDP echo server
- **testecho-udp/client** - UDP echo client
- **testecho-udp/cast** - UDP multicast sample
- **testecho-udp/node** - UDP node sample

All these samples have corresponding Android.mk files and support Android NDK compilation.

#### 3. Output Executables

Compiled executables are located in:
```
HP-Socket/Linux/demo/Release/android-ndk/<ABI>/
├── hp-testecho-udp-server
├── hp-testecho-udp-client
├── hp-testecho-udp-cast
└── hp-testecho-udp-node
```

## Integration in Android Projects

### Using C++ Interface

```cpp
#include <hpsocket/HPSocket.h>

// Create UDP server listener
class CListenerImpl : public CUdpServerListener
{
public:
    virtual EnHandleResult OnPrepareListen(IUdpServer* pSender, SOCKET soListen) override
    {
        // Prepare to listen
        return HR_OK;
    }

    virtual EnHandleResult OnAccept(IUdpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override
    {
        // Accept connection
        return HR_OK;
    }

    virtual EnHandleResult OnReceive(IUdpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
    {
        // Receive data
        pSender->Send(dwConnID, pData, iLength);  // Echo data
        return HR_OK;
    }

    virtual EnHandleResult OnSend(IUdpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
    {
        // Send completed
        return HR_OK;
    }

    virtual EnHandleResult OnClose(IUdpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override
    {
        // Connection closed
        return HR_OK;
    }

    virtual EnHandleResult OnShutdown(IUdpServer* pSender) override
    {
        // Server shutdown
        return HR_OK;
    }
};

// Usage example
void StartUdpServer()
{
    CListenerImpl listener;
    CUdpServer server(&listener);
    
    // Start server
    if(server.Start("0.0.0.0", 5555))
    {
        // Server started successfully
    }
    
    // Stop server
    server.Stop();
}
```

### Using C Interface

```c
#include <hpsocket/HPSocket4C.h>

// Callback function
EnHandleResult __HP_CALL OnReceive(HP_UdpServer pSender, HP_CONNID dwConnID, const BYTE* pData, int iLength)
{
    HP_Server_Send(pSender, dwConnID, pData, iLength);
    return HR_OK;
}

// Usage example
void StartUdpServer()
{
    HP_UdpServerListener listener = Create_HP_UdpServerListener();
    HP_UdpServer server = Create_HP_UdpServer(listener);
    
    // Set callback
    HP_Set_FN_Server_OnReceive(listener, OnReceive);
    
    // Start server
    if(HP_Server_Start(server, "0.0.0.0", 5555))
    {
        // Server started successfully
    }
    
    // Stop and cleanup
    HP_Server_Stop(server);
    Destroy_HP_UdpServer(server);
    Destroy_HP_UdpServerListener(listener);
}
```

## Android.mk Configuration Example

To use HP-Socket UDP module in your own Android project:

```makefile
LOCAL_PATH := $(call my-dir)

# Import HP-Socket static library
include $(CLEAR_VARS)
LOCAL_MODULE := hpsocket
LOCAL_SRC_FILES := path/to/libhpsocket.a
LOCAL_EXPORT_C_INCLUDES := path/to/hpsocket/include
include $(PREBUILT_STATIC_LIBRARY)

# Your application
include $(CLEAR_VARS)
LOCAL_MODULE := myapp
LOCAL_SRC_FILES := main.cpp
LOCAL_STATIC_LIBRARIES := hpsocket
LOCAL_LDLIBS := -ldl -lz
include $(BUILD_EXECUTABLE)
```

## CMakeLists.txt Configuration Example

If using CMake build system:

```cmake
cmake_minimum_required(VERSION 3.4.1)

# Add HP-Socket library
add_library(hpsocket STATIC IMPORTED)
set_target_properties(hpsocket PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}/libhpsocket.a
)

# Include headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Your application
add_executable(myapp main.cpp)
target_link_libraries(myapp hpsocket dl z)
```

## Important Notes

1. **KCP Protocol Support**: The UDP module includes KCP (Fast Reliable Protocol) support for ARQ components
2. **Compilation Dependencies**: If certain features are disabled (such as SSL, ICONV, etc.), you need to define corresponding macros when compiling applications using HP-Socket:
   ```
   -D_SSL_DISABLED -D_ICONV_DISABLED
   ```
3. **Multi-ABI Support**: It's recommended to compile library files for different Android device architectures
4. **Permission Requirements**: Android applications need to add network permissions in AndroidManifest.xml:
   ```xml
   <uses-permission android:name="android.permission.INTERNET" />
   <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
   ```

## Verification Method

You can verify UDP module functionality on Android by compiling and running sample programs:

```bash
# Compile samples
cd HP-Socket/Linux/demo
./build-android-ndk-demo.sh APP_ABI=arm64-v8a

# Push compiled programs to Android device
adb push Release/android-ndk/arm64-v8a/hp-testecho-udp-server /data/local/tmp/
adb push Release/android-ndk/arm64-v8a/hp-testecho-udp-client /data/local/tmp/

# Run on device
adb shell
cd /data/local/tmp
chmod +x hp-testecho-udp-server
chmod +x hp-testecho-udp-client
./hp-testecho-udp-server -b 0.0.0.0 -p 5555
```

## Reference Resources

- [HP-Socket Development Guide](Doc/HP-Socket%20Development%20Guide.pdf)
- [Linux Version README](Linux/README.md)
- [UDP Sample Code](Linux/demo/testecho-udp/)
- [Android NDK Build Script](Linux/script/build-android-ndk.sh)

## Summary

HP-Socket's UDP module provides complete support for the Android platform, including:
- ✅ Complete UDP component implementation (Server/Client/Cast/Node/ARQ)
- ✅ Android NDK build scripts
- ✅ Complete sample programs
- ✅ Support for multiple ABI architectures
- ✅ Support for both static and dynamic libraries
- ✅ Both C and C++ interfaces available

Developers can directly use the provided build scripts to compile library files and then integrate them into Android projects.
