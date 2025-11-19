package com.example.hpsocket;

/**
 * HP-Socket UDP Client Java封装
 * 
 * 使用示例：
 * <pre>
 * UdpClient client = new UdpClient();
 * client.setOnReceiveListener(new UdpClient.OnReceiveListener() {
 *     public void onReceive(byte[] data) {
 *         // 处理接收到的数据
 *         String message = new String(data);
 *         Log.d(TAG, "Received: " + message);
 *     }
 * });
 * 
 * if (client.connect("192.168.1.100", 5555)) {
 *     Log.d(TAG, "Connected to server");
 *     
 *     // 发送数据
 *     String message = "Hello from Android!";
 *     client.send(message.getBytes());
 * }
 * 
 * // 使用完毕后断开
 * client.disconnect();
 * client.destroy();
 * </pre>
 */
public class UdpClient {
    
    static {
        // 加载native库
        // 注意：库名为libhpsocket-jni.so，这里只需要写hpsocket-jni
        System.loadLibrary("hpsocket-jni");
    }
    
    private long nativeHandle = 0;
    private OnConnectListener connectListener;
    private OnReceiveListener receiveListener;
    private OnSendListener sendListener;
    private OnCloseListener closeListener;
    
    /**
     * 连接成功回调接口
     */
    public interface OnConnectListener {
        void onConnect();
    }
    
    /**
     * 接收数据回调接口
     */
    public interface OnReceiveListener {
        void onReceive(byte[] data);
    }
    
    /**
     * 发送数据回调接口
     */
    public interface OnSendListener {
        void onSend(int length);
    }
    
    /**
     * 连接关闭回调接口
     */
    public interface OnCloseListener {
        void onClose(int errorCode);
    }
    
    /**
     * 构造函数
     */
    public UdpClient() {
        nativeHandle = nativeCreate(this);
        if (nativeHandle == 0) {
            throw new RuntimeException("Failed to create native UDP Client");
        }
    }
    
    /**
     * 连接到服务器
     * 
     * @param host 服务器地址
     * @param port 服务器端口
     * @return true表示成功，false表示失败
     */
    public boolean connect(String host, int port) {
        if (nativeHandle == 0) {
            return false;
        }
        return nativeConnect(nativeHandle, host, port);
    }
    
    /**
     * 断开连接
     * 
     * @return true表示成功，false表示失败
     */
    public boolean disconnect() {
        if (nativeHandle == 0) {
            return false;
        }
        return nativeDisconnect(nativeHandle);
    }
    
    /**
     * 发送数据
     * 
     * @param data 要发送的数据
     * @return true表示成功，false表示失败
     */
    public boolean send(byte[] data) {
        if (nativeHandle == 0 || data == null) {
            return false;
        }
        return nativeSend(nativeHandle, data);
    }
    
    /**
     * 检查是否已连接
     * 
     * @return true表示已连接，false表示未连接
     */
    public boolean isConnected() {
        if (nativeHandle == 0) {
            return false;
        }
        return nativeIsConnected(nativeHandle);
    }
    
    /**
     * 销毁客户端（释放资源）
     * 调用后此对象不能再使用
     */
    public void destroy() {
        if (nativeHandle != 0) {
            nativeDestroy(nativeHandle);
            nativeHandle = 0;
        }
    }
    
    /**
     * 设置连接成功监听器
     */
    public void setOnConnectListener(OnConnectListener listener) {
        this.connectListener = listener;
    }
    
    /**
     * 设置接收数据监听器
     */
    public void setOnReceiveListener(OnReceiveListener listener) {
        this.receiveListener = listener;
    }
    
    /**
     * 设置发送数据监听器
     */
    public void setOnSendListener(OnSendListener listener) {
        this.sendListener = listener;
    }
    
    /**
     * 设置连接关闭监听器
     */
    public void setOnCloseListener(OnCloseListener listener) {
        this.closeListener = listener;
    }
    
    // ========== Native回调方法（由C++层调用） ==========
    
    @SuppressWarnings("unused")
    private void onConnect() {
        if (connectListener != null) {
            connectListener.onConnect();
        }
    }
    
    @SuppressWarnings("unused")
    private void onReceive(byte[] data) {
        if (receiveListener != null) {
            receiveListener.onReceive(data);
        }
    }
    
    @SuppressWarnings("unused")
    private void onSend(int length) {
        if (sendListener != null) {
            sendListener.onSend(length);
        }
    }
    
    @SuppressWarnings("unused")
    private void onClose(int errorCode) {
        if (closeListener != null) {
            closeListener.onClose(errorCode);
        }
    }
    
    // ========== Native方法声明 ==========
    
    private native long nativeCreate(Object callback);
    private native boolean nativeConnect(long handle, String host, int port);
    private native boolean nativeDisconnect(long handle);
    private native boolean nativeSend(long handle, byte[] data);
    private native void nativeDestroy(long handle);
    private native boolean nativeIsConnected(long handle);
}
