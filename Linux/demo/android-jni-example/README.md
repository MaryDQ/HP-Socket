# Android JNI Example - HP-Socket源码编译集成示例

本示例演示如何在Android项目中通过JNI使用HP-Socket，采用源码直接编译的方式，无需预编译库。

## 示例特性

- ✅ 源码直接编译HP-Socket
- ✅ UDP Server和UDP Client完整实现
- ✅ JNI封装层
- ✅ 支持多ABI架构
- ✅ 完整的错误处理和日志
- ✅ 可直接集成到Android Studio项目

## 项目结构

```
android-jni-example/
├── README.md                          # 本文件
├── CMakeLists.txt                     # CMake构建文件（编译HP-Socket源码）
├── jni/
│   ├── udp_server_jni.cpp            # UDP Server JNI封装
│   ├── udp_client_jni.cpp            # UDP Client JNI封装
│   └── common.h                       # 公共头文件
├── java/
│   ├── UdpServer.java                # UDP Server Java接口
│   └── UdpClient.java                # UDP Client Java接口
└── integration-guide.md              # 集成到Android Studio指南
```

## 快速开始

### 1. 将示例集成到Android Studio项目

#### 步骤1：复制文件到项目

```bash
# 假设你的Android项目路径为 MyAndroidApp/
cd MyAndroidApp/app/src/main

# 创建cpp目录（如果不存在）
mkdir -p cpp

# 复制CMakeLists.txt和JNI代码
cp /path/to/HP-Socket/Linux/demo/android-jni-example/CMakeLists.txt cpp/
cp /path/to/HP-Socket/Linux/demo/android-jni-example/jni/* cpp/

# 复制Java接口文件到你的包路径
# 例如：com.example.myapp
cp /path/to/HP-Socket/Linux/demo/android-jni-example/java/*.java \
   java/com/example/myapp/
```

#### 步骤2：修改app/build.gradle

在`android`块中添加：

```gradle
android {
    ...
    
    defaultConfig {
        ...
        
        externalNativeBuild {
            cmake {
                cppFlags "-std=c++14 -frtti -fexceptions"
                arguments "-DHPSOCKET_SRC_DIR=${project.rootDir}/../HP-Socket/Linux/src",
                          "-DHPSOCKET_INCLUDE_DIR=${project.rootDir}/../HP-Socket/Linux/include"
            }
        }
        
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
    
    externalNativeBuild {
        cmake {
            path "src/main/cpp/CMakeLists.txt"
            version "3.18.1"
        }
    }
}
```

**注意：** 修改`HPSOCKET_SRC_DIR`和`HPSOCKET_INCLUDE_DIR`指向你的HP-Socket源码路径。

#### 步骤3：修改AndroidManifest.xml

添加网络权限：

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
```

#### 步骤4：在MainActivity中使用

```java
package com.example.myapp;

import android.os.Bundle;
import android.util.Log;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    private UdpServer udpServer;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        // 创建UDP Server
        udpServer = new UdpServer();
        
        // 启动服务器
        if (udpServer.start("0.0.0.0", 5555)) {
            Log.d(TAG, "UDP Server started on port 5555");
        } else {
            Log.e(TAG, "Failed to start UDP Server");
        }
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (udpServer != null) {
            udpServer.stop();
        }
    }
}
```

### 2. 独立使用（不集成到项目）

如果只是想测试示例，可以直接使用NDK编译：

```bash
cd /path/to/HP-Socket/Linux/demo/android-jni-example

# 使用ndk-build编译
ndk-build NDK_PROJECT_PATH=. \
          APP_BUILD_SCRIPT=./Android.mk \
          NDK_APPLICATION_MK=./Application.mk
```

## 使用示例

### UDP Server示例

```java
// 创建UDP Server
UdpServer server = new UdpServer();

// 设置接收回调
server.setOnReceiveListener(new UdpServer.OnReceiveListener() {
    @Override
    public void onReceive(long connId, byte[] data) {
        Log.d(TAG, "Received: " + new String(data));
        // 回显数据
        server.send(connId, data);
    }
});

// 启动服务器
if (server.start("0.0.0.0", 5555)) {
    Log.d(TAG, "Server started");
}

// 停止服务器
server.stop();
```

### UDP Client示例

```java
// 创建UDP Client
UdpClient client = new UdpClient();

// 设置接收回调
client.setOnReceiveListener(new UdpClient.OnReceiveListener() {
    @Override
    public void onReceive(byte[] data) {
        Log.d(TAG, "Received: " + new String(data));
    }
});

// 连接到服务器
if (client.connect("192.168.1.100", 5555)) {
    Log.d(TAG, "Connected to server");
    
    // 发送数据
    String message = "Hello from Android!";
    client.send(message.getBytes());
}

// 断开连接
client.disconnect();
```

## 高级配置

### 禁用不需要的功能

如果不需要SSL、HTTP等功能，可以在CMakeLists.txt中添加相应的定义来减小库体积：

```cmake
add_definitions(
    -D_SSL_DISABLED
    -D_HTTP_DISABLED
    -D_ICONV_DISABLED
    -D_BROTLI_DISABLED
    -D_MIMALLOC_DISABLED
)
```

### 自定义编译选项

在build.gradle的cmake块中添加：

```gradle
cmake {
    cppFlags "-std=c++14 -frtti -fexceptions -O2"
    arguments "-D_SSL_DISABLED=ON",
              "-D_HTTP_DISABLED=ON"
}
```

### 支持更多ABI

在build.gradle中修改：

```gradle
ndk {
    abiFilters 'arm64-v8a', 'armeabi-v7a', 'x86_64', 'x86'
}
```

## 故障排除

### 问题1：找不到HP-Socket源文件

**错误信息：** `CMake Error: Cannot find source file`

**解决方案：**
- 检查build.gradle中的`HPSOCKET_SRC_DIR`和`HPSOCKET_INCLUDE_DIR`路径是否正确
- 确保HP-Socket源码已经下载到指定位置

### 问题2：编译错误 - undefined reference

**错误信息：** `undefined reference to 'CUdpServer::Start'`

**解决方案：**
- 确保CMakeLists.txt中包含了所有必要的源文件
- 检查链接库是否完整（dl, z, log等）

### 问题3：运行时错误 - UnsatisfiedLinkError

**错误信息：** `java.lang.UnsatisfiedLinkError`

**解决方案：**
- 检查Java类的包名和JNI函数名是否匹配
- 确保native库已正确编译并打包到APK中
- 检查ABI是否与设备匹配

### 问题4：接收不到数据

**解决方案：**
- 检查AndroidManifest.xml中是否添加了网络权限
- 检查防火墙设置
- 使用`adb logcat`查看详细日志

## 性能优化建议

1. **使用Release构建**：在build.gradle中配置release版本
2. **启用编译优化**：在cmake中添加`-O2`或`-O3`优化标志
3. **减小库体积**：禁用不需要的功能（SSL、HTTP等）
4. **仅编译需要的ABI**：通过abiFilters限制架构数量

## 参考资源

- [HP-Socket开发指南](../../Doc/HP-Socket%20Development%20Guide.pdf)
- [Android NDK官方文档](https://developer.android.com/ndk)
- [CMake官方文档](https://cmake.org/documentation/)
- [HP-Socket GitHub主页](https://github.com/ldcsaa/HP-Socket)

## 许可证

本示例遵循HP-Socket的Apache License 2.0许可证。

## 技术支持

如有问题，请访问：
- HP-Socket GitHub Issues
- QQ交流群（见主README）
