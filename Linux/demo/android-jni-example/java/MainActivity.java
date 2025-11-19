package com.example.hpsocket;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;

/**
 * Android JNI示例 - MainActivity
 * 
 * 展示如何使用HP-Socket UDP Server和Client
 * 
 * 使用说明：
 * 1. 将此文件复制到你的项目中
 * 2. 修改包名为你的项目包名
 * 3. 创建对应的布局文件 activity_main.xml
 * 4. 在AndroidManifest.xml中添加INTERNET权限
 */
public class MainActivity extends AppCompatActivity {
    private static final String TAG = "HPSocket-Example";
    
    // UI组件
    private TextView statusText;
    private EditText portEdit;
    private EditText hostEdit;
    private EditText messageEdit;
    private Button startServerBtn;
    private Button stopServerBtn;
    private Button connectBtn;
    private Button sendBtn;
    private Button disconnectBtn;
    
    // HP-Socket组件
    private UdpServer udpServer;
    private UdpClient udpClient;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        // 初始化UI
        initViews();
        
        // 初始化HP-Socket组件
        initHPSocket();
        
        // 设置按钮监听
        setupListeners();
    }
    
    private void initViews() {
        statusText = findViewById(R.id.status_text);
        portEdit = findViewById(R.id.port_edit);
        hostEdit = findViewById(R.id.host_edit);
        messageEdit = findViewById(R.id.message_edit);
        startServerBtn = findViewById(R.id.start_server_btn);
        stopServerBtn = findViewById(R.id.stop_server_btn);
        connectBtn = findViewById(R.id.connect_btn);
        sendBtn = findViewById(R.id.send_btn);
        disconnectBtn = findViewById(R.id.disconnect_btn);
        
        // 设置默认值
        portEdit.setText("5555");
        hostEdit.setText("127.0.0.1");
        
        // 初始状态
        stopServerBtn.setEnabled(false);
        sendBtn.setEnabled(false);
        disconnectBtn.setEnabled(false);
    }
    
    private void initHPSocket() {
        // 创建UDP Server
        udpServer = new UdpServer();
        
        // 设置Server回调
        udpServer.setOnAcceptListener(new UdpServer.OnAcceptListener() {
            @Override
            public void onAccept(long connId) {
                updateStatus("Client connected: " + connId);
                Log.i(TAG, "Client accepted: " + connId);
            }
        });
        
        udpServer.setOnReceiveListener(new UdpServer.OnReceiveListener() {
            @Override
            public void onReceive(long connId, byte[] data) {
                String message = new String(data);
                updateStatus("Server received from " + connId + ": " + message);
                Log.i(TAG, "Server received: " + message);
                
                // 自动回显
                udpServer.send(connId, data);
            }
        });
        
        udpServer.setOnCloseListener(new UdpServer.OnCloseListener() {
            @Override
            public void onClose(long connId, int errorCode) {
                updateStatus("Client disconnected: " + connId);
                Log.i(TAG, "Client closed: " + connId);
            }
        });
        
        // 创建UDP Client
        udpClient = new UdpClient();
        
        // 设置Client回调
        udpClient.setOnConnectListener(new UdpClient.OnConnectListener() {
            @Override
            public void onConnect() {
                updateStatus("Connected to server");
                Log.i(TAG, "Client connected");
            }
        });
        
        udpClient.setOnReceiveListener(new UdpClient.OnReceiveListener() {
            @Override
            public void onReceive(byte[] data) {
                String message = new String(data);
                updateStatus("Client received: " + message);
                Log.i(TAG, "Client received: " + message);
            }
        });
        
        udpClient.setOnCloseListener(new UdpClient.OnCloseListener() {
            @Override
            public void onClose(int errorCode) {
                updateStatus("Disconnected from server");
                Log.i(TAG, "Client disconnected");
            }
        });
    }
    
    private void setupListeners() {
        // 启动Server
        startServerBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startServer();
            }
        });
        
        // 停止Server
        stopServerBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                stopServer();
            }
        });
        
        // 连接到Server
        connectBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                connectToServer();
            }
        });
        
        // 发送消息
        sendBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                sendMessage();
            }
        });
        
        // 断开连接
        disconnectBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                disconnectFromServer();
            }
        });
    }
    
    private void startServer() {
        try {
            int port = Integer.parseInt(portEdit.getText().toString());
            
            if (udpServer.start("0.0.0.0", port)) {
                updateStatus("Server started on port " + port);
                showToast("Server started successfully");
                startServerBtn.setEnabled(false);
                stopServerBtn.setEnabled(true);
                Log.i(TAG, "Server started on port: " + port);
            } else {
                updateStatus("Failed to start server");
                showToast("Failed to start server");
                Log.e(TAG, "Failed to start server");
            }
        } catch (NumberFormatException e) {
            showToast("Invalid port number");
        }
    }
    
    private void stopServer() {
        if (udpServer.stop()) {
            updateStatus("Server stopped");
            showToast("Server stopped");
            startServerBtn.setEnabled(true);
            stopServerBtn.setEnabled(false);
            Log.i(TAG, "Server stopped");
        } else {
            updateStatus("Failed to stop server");
            showToast("Failed to stop server");
        }
    }
    
    private void connectToServer() {
        try {
            String host = hostEdit.getText().toString();
            int port = Integer.parseInt(portEdit.getText().toString());
            
            if (udpClient.connect(host, port)) {
                updateStatus("Connecting to " + host + ":" + port);
                showToast("Connected to server");
                connectBtn.setEnabled(false);
                sendBtn.setEnabled(true);
                disconnectBtn.setEnabled(true);
                Log.i(TAG, "Connected to: " + host + ":" + port);
            } else {
                updateStatus("Failed to connect");
                showToast("Failed to connect");
                Log.e(TAG, "Failed to connect");
            }
        } catch (NumberFormatException e) {
            showToast("Invalid port number");
        }
    }
    
    private void sendMessage() {
        String message = messageEdit.getText().toString();
        if (message.isEmpty()) {
            showToast("Please enter a message");
            return;
        }
        
        if (udpClient.send(message.getBytes())) {
            updateStatus("Sent: " + message);
            Log.i(TAG, "Message sent: " + message);
            messageEdit.setText("");  // 清空输入框
        } else {
            updateStatus("Failed to send message");
            showToast("Failed to send message");
            Log.e(TAG, "Failed to send message");
        }
    }
    
    private void disconnectFromServer() {
        if (udpClient.disconnect()) {
            updateStatus("Disconnected");
            showToast("Disconnected from server");
            connectBtn.setEnabled(true);
            sendBtn.setEnabled(false);
            disconnectBtn.setEnabled(false);
            Log.i(TAG, "Disconnected from server");
        } else {
            updateStatus("Failed to disconnect");
            showToast("Failed to disconnect");
        }
    }
    
    private void updateStatus(final String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                statusText.setText(message);
            }
        });
    }
    
    private void showToast(final String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(MainActivity.this, message, Toast.LENGTH_SHORT).show();
            }
        });
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        // 清理资源
        if (udpServer != null) {
            udpServer.stop();
            udpServer.destroy();
        }
        
        if (udpClient != null) {
            udpClient.disconnect();
            udpClient.destroy();
        }
        
        Log.i(TAG, "Activity destroyed, resources cleaned up");
    }
}
