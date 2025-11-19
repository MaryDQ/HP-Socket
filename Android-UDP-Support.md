# HP-Socket UDP模块Android支持说明

## 结论

**HP-Socket的UDP模块完全支持Android平台使用。**

## 支持的UDP组件

HP-Socket为Android平台提供以下UDP组件：

1. **UdpServer** - UDP服务器组件
2. **UdpClient** - UDP客户端组件
3. **UdpCast** - UDP组播组件
4. **UdpNode** - UDP节点组件（可同时作为服务器和客户端）
5. **UdpArqServer** - 基于ARQ协议的可靠UDP服务器
6. **UdpArqClient** - 基于ARQ协议的可靠UDP客户端

## Android平台实现方式

### 方式一：编译HP-Socket库（推荐）

#### 1. 环境准备

确保已安装Android NDK，并配置好环境变量。

#### 2. 编译HP-Socket库

进入Linux目录，执行Android NDK构建脚本：

```bash
cd HP-Socket/Linux
./script/build-android-ndk.sh
```

#### 3. 构建选项

**默认编译所有ABI架构：**
- arm64-v8a
- armeabi-v7a
- x86_64
- x86

**指定特定ABI架构：**
```bash
./script/build-android-ndk.sh APP_ABI=armeabi-v7a,arm64-v8a
```

**禁用特定功能（如果不需要）：**
```bash
# 仅编译UDP，禁用SSL和HTTP
./script/build-android-ndk.sh _SSL_DISABLED=true _HTTP_DISABLED=true
```

**注意：** 默认情况下UDP功能是启用的。如果要禁用UDP（不推荐），可以使用：
```bash
./script/build-android-ndk.sh _UDP_DISABLED=true
```

#### 4. 输出文件

编译完成后，库文件将输出到：
```
HP-Socket/Linux/lib/android-ndk/
├── arm64-v8a/
│   ├── libhpsocket.a      # C++静态库
│   ├── libhpsocket.so     # C++动态库
│   ├── libhpsocket4c.a    # C接口静态库
│   └── libhpsocket4c.so   # C接口动态库
├── armeabi-v7a/
├── x86_64/
└── x86/
```

### 方式二：编译UDP示例程序

HP-Socket提供了完整的UDP示例程序，可以直接在Android上编译运行。

#### 1. 编译所有示例

```bash
cd HP-Socket/Linux/demo
./build-android-ndk-demo.sh
```

#### 2. 编译特定UDP示例

UDP示例程序包括：
- **testecho-udp/server** - UDP回显服务器
- **testecho-udp/client** - UDP回显客户端
- **testecho-udp/cast** - UDP组播示例
- **testecho-udp/node** - UDP节点示例

所有这些示例都有对应的Android.mk文件，支持Android NDK编译。

#### 3. 输出可执行文件

编译后的可执行文件位于：
```
HP-Socket/Linux/demo/Release/android-ndk/<ABI>/
├── hp-testecho-udp-server
├── hp-testecho-udp-client
├── hp-testecho-udp-cast
└── hp-testecho-udp-node
```

## 在Android项目中集成

### 使用C++接口

```cpp
#include <hpsocket/HPSocket.h>

// 创建UDP服务器监听器
class CListenerImpl : public CUdpServerListener
{
public:
    virtual EnHandleResult OnPrepareListen(IUdpServer* pSender, SOCKET soListen) override
    {
        // 准备监听
        return HR_OK;
    }

    virtual EnHandleResult OnAccept(IUdpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override
    {
        // 接受连接
        return HR_OK;
    }

    virtual EnHandleResult OnReceive(IUdpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
    {
        // 接收数据
        pSender->Send(dwConnID, pData, iLength);  // 回显数据
        return HR_OK;
    }

    virtual EnHandleResult OnSend(IUdpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
    {
        // 发送完成
        return HR_OK;
    }

    virtual EnHandleResult OnClose(IUdpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override
    {
        // 连接关闭
        return HR_OK;
    }

    virtual EnHandleResult OnShutdown(IUdpServer* pSender) override
    {
        // 服务器关闭
        return HR_OK;
    }
};

// 使用示例
void StartUdpServer()
{
    CListenerImpl listener;
    CUdpServer server(&listener);
    
    // 启动服务器
    if(server.Start("0.0.0.0", 5555))
    {
        // 服务器启动成功
    }
    
    // 停止服务器
    server.Stop();
}
```

### 使用C接口

```c
#include <hpsocket/HPSocket4C.h>

// 回调函数
EnHandleResult __HP_CALL OnReceive(HP_UdpServer pSender, HP_CONNID dwConnID, const BYTE* pData, int iLength)
{
    HP_Server_Send(pSender, dwConnID, pData, iLength);
    return HR_OK;
}

// 使用示例
void StartUdpServer()
{
    HP_UdpServerListener listener = Create_HP_UdpServerListener();
    HP_UdpServer server = Create_HP_UdpServer(listener);
    
    // 设置回调
    HP_Set_FN_Server_OnReceive(listener, OnReceive);
    
    // 启动服务器
    if(HP_Server_Start(server, "0.0.0.0", 5555))
    {
        // 服务器启动成功
    }
    
    // 停止和清理
    HP_Server_Stop(server);
    Destroy_HP_UdpServer(server);
    Destroy_HP_UdpServerListener(listener);
}
```

## Android.mk配置示例

如果要在自己的Android项目中使用HP-Socket UDP模块：

```makefile
LOCAL_PATH := $(call my-dir)

# 导入HP-Socket静态库
include $(CLEAR_VARS)
LOCAL_MODULE := hpsocket
LOCAL_SRC_FILES := path/to/libhpsocket.a
LOCAL_EXPORT_C_INCLUDES := path/to/hpsocket/include
include $(PREBUILT_STATIC_LIBRARY)

# 你的应用
include $(CLEAR_VARS)
LOCAL_MODULE := myapp
LOCAL_SRC_FILES := main.cpp
LOCAL_STATIC_LIBRARIES := hpsocket
LOCAL_LDLIBS := -ldl -lz
include $(BUILD_EXECUTABLE)
```

## CMakeLists.txt配置示例

如果使用CMake构建系统：

```cmake
cmake_minimum_required(VERSION 3.4.1)

# 添加HP-Socket库
add_library(hpsocket STATIC IMPORTED)
set_target_properties(hpsocket PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}/libhpsocket.a
)

# 包含头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 你的应用
add_executable(myapp main.cpp)
target_link_libraries(myapp hpsocket dl z)
```

## 注意事项

1. **KCP协议支持**: UDP模块包含KCP（快速可靠协议）支持，用于ARQ组件
2. **编译依赖**: 如果禁用了某些功能（如SSL、ICONV等），在编译使用HP-Socket的应用时需要定义相应的宏：
   ```
   -D_SSL_DISABLED -D_ICONV_DISABLED
   ```
3. **多ABI支持**: 建议为不同的Android设备架构编译对应的库文件
4. **权限要求**: Android应用需要在AndroidManifest.xml中添加网络权限：
   ```xml
   <uses-permission android:name="android.permission.INTERNET" />
   <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
   ```

## 验证方法

可以通过编译和运行示例程序来验证UDP模块在Android上的工作情况：

```bash
# 编译示例
cd HP-Socket/Linux/demo
./build-android-ndk-demo.sh APP_ABI=arm64-v8a

# 将编译好的程序推送到Android设备
adb push Release/android-ndk/arm64-v8a/hp-testecho-udp-server /data/local/tmp/
adb push Release/android-ndk/arm64-v8a/hp-testecho-udp-client /data/local/tmp/

# 在设备上运行
adb shell
cd /data/local/tmp
chmod +x hp-testecho-udp-server
chmod +x hp-testecho-udp-client
./hp-testecho-udp-server -b 0.0.0.0 -p 5555
```

## 参考资源

- [HP-Socket开发指南](Doc/HP-Socket%20Development%20Guide.pdf)
- [Linux版本README](Linux/README.md)
- [UDP示例代码](Linux/demo/testecho-udp/)
- [Android NDK构建脚本](Linux/script/build-android-ndk.sh)

## 总结

HP-Socket的UDP模块为Android平台提供了完整的支持，包括：
- ✅ 完整的UDP组件实现（Server/Client/Cast/Node/ARQ）
- ✅ Android NDK构建脚本
- ✅ 完整的示例程序
- ✅ 支持多种ABI架构
- ✅ 支持静态库和动态库
- ✅ 提供C和C++两种接口

开发者可以直接使用提供的构建脚本编译库文件，然后在Android项目中集成使用。
