# 集成到Android Studio项目指南

本指南详细说明如何将HP-Socket JNI示例集成到现有的Android Studio项目中。

## 前提条件

- Android Studio 4.0或更高版本
- Android NDK已安装并配置
- HP-Socket源码

## 集成步骤

### 步骤1：准备HP-Socket源码

将HP-Socket源码克隆或下载到本地：

```bash
git clone https://github.com/ldcsaa/HP-Socket.git
# 或者从Gitee克隆
git clone https://gitee.com/ldcsaa/HP-Socket.git
```

记录HP-Socket的路径，例如：`/path/to/HP-Socket`

### 步骤2：复制示例文件到项目

假设你的Android项目结构如下：

```
MyAndroidApp/
├── app/
│   ├── src/
│   │   └── main/
│   │       ├── java/
│   │       │   └── com/
│   │       │       └── example/
│   │       │           └── myapp/
│   │       ├── res/
│   │       └── AndroidManifest.xml
│   └── build.gradle
├── build.gradle
└── settings.gradle
```

#### 2.1 创建cpp目录

```bash
cd MyAndroidApp/app/src/main
mkdir cpp
```

#### 2.2 复制CMakeLists.txt和JNI代码

```bash
# 复制CMakeLists.txt
cp /path/to/HP-Socket/Linux/demo/android-jni-example/CMakeLists.txt cpp/

# 复制JNI源码
cp -r /path/to/HP-Socket/Linux/demo/android-jni-example/jni cpp/
```

#### 2.3 复制Java接口文件

```bash
# 复制到你的包目录下（注意修改包名）
cp /path/to/HP-Socket/Linux/demo/android-jni-example/java/*.java \
   java/com/example/myapp/
```

### 步骤3：修改Java文件包名

编辑`UdpServer.java`和`UdpClient.java`，将包名修改为你的项目包名：

```java
// 原来的包名
package com.example.hpsocket;

// 修改为你的包名，例如：
package com.example.myapp;
```

### 步骤4：修改JNI函数签名

编辑`jni/udp_server_jni.cpp`和`jni/udp_client_jni.cpp`，将JNI函数名中的包名部分修改为你的包名。

**原来的函数名（示例）：**
```cpp
Java_com_example_hpsocket_UdpServer_nativeCreate
```

**修改为（假设你的包名是com.example.myapp）：**
```cpp
Java_com_example_myapp_UdpServer_nativeCreate
```

使用文本替换功能批量替换：
- 将`com_example_hpsocket`替换为`com_example_myapp`

### 步骤5：配置app/build.gradle

在`app/build.gradle`文件中添加CMake配置：

```gradle
android {
    compileSdkVersion 33
    
    defaultConfig {
        applicationId "com.example.myapp"
        minSdkVersion 21
        targetSdkVersion 33
        versionCode 1
        versionName "1.0"
        
        externalNativeBuild {
            cmake {
                cppFlags "-std=c++14 -frtti -fexceptions"
                
                // 设置HP-Socket源码路径
                // 方法1：使用绝对路径
                arguments "-DHPSOCKET_SRC_DIR=/path/to/HP-Socket/Linux/src",
                          "-DHPSOCKET_INCLUDE_DIR=/path/to/HP-Socket/Linux/include"
                
                // 方法2：使用相对路径（推荐）
                // 假设HP-Socket与你的项目在同一父目录下
                // arguments "-DHPSOCKET_SRC_DIR=${project.rootDir}/../../HP-Socket/Linux/src",
                //           "-DHPSOCKET_INCLUDE_DIR=${project.rootDir}/../../HP-Socket/Linux/include"
            }
        }
        
        ndk {
            // 指定要编译的ABI（根据需要选择）
            abiFilters 'arm64-v8a', 'armeabi-v7a'
            // 如果需要支持更多平台，可以添加 'x86_64', 'x86'
        }
    }
    
    externalNativeBuild {
        cmake {
            path "src/main/cpp/CMakeLists.txt"
            version "3.18.1"  // 使用你已安装的CMake版本
        }
    }
    
    buildTypes {
        release {
            minifyEnabled false
            proguardFiles getDefaultProguardFile('proguard-android-optimize.txt'), 'proguard-rules.pro'
        }
    }
}

dependencies {
    implementation 'androidx.appcompat:appcompat:1.6.1'
    // ... 其他依赖
}
```

### 步骤6：配置AndroidManifest.xml

添加必要的权限：

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.myapp">
    
    <!-- 网络权限 -->
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

### 步骤7：配置ProGuard（如果使用混淆）

在`proguard-rules.pro`中添加：

```proguard
# 保留native方法
-keepclasseswithmembernames class * {
    native <methods>;
}

# 保留HP-Socket相关类
-keep class com.example.myapp.UdpServer { *; }
-keep class com.example.myapp.UdpClient { *; }
-keep class com.example.myapp.UdpServer$* { *; }
-keep class com.example.myapp.UdpClient$* { *; }
```

### 步骤8：在代码中使用

#### 8.1 UDP Server示例

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
    private UdpServer udpServer;
    private TextView statusText;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        statusText = findViewById(R.id.status_text);
        Button startBtn = findViewById(R.id.start_button);
        Button stopBtn = findViewById(R.id.stop_button);
        
        // 创建UDP Server
        udpServer = new UdpServer();
        
        // 设置接收数据回调
        udpServer.setOnReceiveListener(new UdpServer.OnReceiveListener() {
            @Override
            public void onReceive(long connId, byte[] data) {
                String message = new String(data);
                Log.d(TAG, "Received from " + connId + ": " + message);
                
                runOnUiThread(() -> {
                    statusText.setText("Received: " + message);
                });
                
                // 回显数据
                udpServer.send(connId, data);
            }
        });
        
        // 设置其他回调
        udpServer.setOnAcceptListener(new UdpServer.OnAcceptListener() {
            @Override
            public void onAccept(long connId) {
                Log.d(TAG, "Client connected: " + connId);
            }
        });
        
        // 启动按钮
        startBtn.setOnClickListener(v -> {
            if (udpServer.start("0.0.0.0", 5555)) {
                Log.d(TAG, "UDP Server started on port 5555");
                statusText.setText("Server started on port 5555");
                startBtn.setEnabled(false);
                stopBtn.setEnabled(true);
            } else {
                Log.e(TAG, "Failed to start UDP Server");
                statusText.setText("Failed to start server");
            }
        });
        
        // 停止按钮
        stopBtn.setOnClickListener(v -> {
            if (udpServer.stop()) {
                Log.d(TAG, "UDP Server stopped");
                statusText.setText("Server stopped");
                startBtn.setEnabled(true);
                stopBtn.setEnabled(false);
            }
        });
        
        stopBtn.setEnabled(false);
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (udpServer != null) {
            udpServer.stop();
            udpServer.destroy();
        }
    }
}
```

#### 8.2 UDP Client示例

```java
package com.example.myapp;

import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;

public class ClientActivity extends AppCompatActivity {
    private static final String TAG = "ClientActivity";
    private UdpClient udpClient;
    private TextView statusText;
    private EditText messageInput;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_client);
        
        statusText = findViewById(R.id.status_text);
        messageInput = findViewById(R.id.message_input);
        Button connectBtn = findViewById(R.id.connect_button);
        Button sendBtn = findViewById(R.id.send_button);
        Button disconnectBtn = findViewById(R.id.disconnect_button);
        
        // 创建UDP Client
        udpClient = new UdpClient();
        
        // 设置接收数据回调
        udpClient.setOnReceiveListener(new UdpClient.OnReceiveListener() {
            @Override
            public void onReceive(byte[] data) {
                String message = new String(data);
                Log.d(TAG, "Received: " + message);
                
                runOnUiThread(() -> {
                    statusText.setText("Received: " + message);
                });
            }
        });
        
        // 连接按钮
        connectBtn.setOnClickListener(v -> {
            // 注意：替换为实际的服务器地址
            if (udpClient.connect("192.168.1.100", 5555)) {
                Log.d(TAG, "Connected to server");
                statusText.setText("Connected to server");
                connectBtn.setEnabled(false);
                sendBtn.setEnabled(true);
                disconnectBtn.setEnabled(true);
            } else {
                Log.e(TAG, "Failed to connect");
                statusText.setText("Failed to connect");
            }
        });
        
        // 发送按钮
        sendBtn.setOnClickListener(v -> {
            String message = messageInput.getText().toString();
            if (!message.isEmpty()) {
                if (udpClient.send(message.getBytes())) {
                    Log.d(TAG, "Message sent: " + message);
                } else {
                    Log.e(TAG, "Failed to send message");
                }
            }
        });
        
        // 断开按钮
        disconnectBtn.setOnClickListener(v -> {
            if (udpClient.disconnect()) {
                Log.d(TAG, "Disconnected from server");
                statusText.setText("Disconnected");
                connectBtn.setEnabled(true);
                sendBtn.setEnabled(false);
                disconnectBtn.setEnabled(false);
            }
        });
        
        sendBtn.setEnabled(false);
        disconnectBtn.setEnabled(false);
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (udpClient != null) {
            udpClient.disconnect();
            udpClient.destroy();
        }
    }
}
```

### 步骤9：同步和编译项目

1. 在Android Studio中，点击 **File > Sync Project with Gradle Files**
2. 等待同步完成
3. 点击 **Build > Make Project** 编译项目
4. 如果编译成功，运行应用

## 常见问题

### 问题1：CMake找不到HP-Socket源文件

**错误：** `CMake Error at CMakeLists.txt:XX (add_library): Cannot find source file`

**解决方案：**
- 检查`build.gradle`中的`HPSOCKET_SRC_DIR`路径是否正确
- 确保路径使用绝对路径或正确的相对路径
- 路径中不要包含空格和特殊字符

### 问题2：JNI函数未找到

**错误：** `java.lang.UnsatisfiedLinkError: No implementation found for ...`

**解决方案：**
- 确保JNI函数签名中的包名与Java类的包名一致
- 检查库是否正确加载（`System.loadLibrary("hpsocket-jni")`）
- 使用`javap -s`命令查看正确的JNI函数签名

### 问题3：编译错误 - undefined reference

**错误：** `undefined reference to 'CUdpServer::Start'`

**解决方案：**
- 确保CMakeLists.txt中包含了所有必要的源文件
- 检查是否正确链接了系统库（dl, log等）
- 清理项目后重新编译：**Build > Clean Project**

### 问题4：运行时权限问题

**错误：** 无法访问网络

**解决方案：**
- 检查AndroidManifest.xml中是否添加了网络权限
- 对于Android 6.0+，某些权限需要运行时请求

## 性能优化

1. **减小库体积**：在CMakeLists.txt中禁用不需要的功能
2. **仅编译必要的ABI**：在build.gradle中限制abiFilters
3. **使用Release构建**：启用优化选项

## 下一步

- 查看[README.md](README.md)了解更多使用示例
- 参考HP-Socket开发文档学习高级功能
- 根据需要扩展JNI封装以支持更多HP-Socket功能

## 技术支持

如有问题，请：
- 查看HP-Socket官方文档
- 访问HP-Socket GitHub Issues
- 加入QQ技术交流群（见主README）
