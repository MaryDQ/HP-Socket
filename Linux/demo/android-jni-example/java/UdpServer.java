package com.example.hpsocket;

/**
 * HP-Socket UDP Server Java封装
 * 
 * 使用示例：
 * <pre>
 * UdpServer server = new UdpServer();
 * server.setOnReceiveListener(new UdpServer.OnReceiveListener() {
 *     public void onReceive(long connId, byte[] data) {
 *         // 处理接收到的数据
 *         String message = new String(data);
 *         Log.d(TAG, "Received: " + message);
 *         
 *         // 回显数据
 *         server.send(connId, data);
 *     }
 * });
 * 
 * if (server.start("0.0.0.0", 5555)) {
 *     Log.d(TAG, "Server started successfully");
 * }
 * 
 * // 使用完毕后停止
 * server.stop();
 * server.destroy();
 * </pre>
 */
public class UdpServer {
    
    static {
        // 加载native库
        // 注意：库名为libhpsocket-jni.so，这里只需要写hpsocket-jni
        System.loadLibrary("hpsocket-jni");
    }
    
    private long nativeHandle = 0;
    private OnReceiveListener receiveListener;
    private OnAcceptListener acceptListener;
    private OnSendListener sendListener;
    private OnCloseListener closeListener;
    private OnShutdownListener shutdownListener;
    
    /**
     * 接收数据回调接口
     */
    public interface OnReceiveListener {
        void onReceive(long connId, byte[] data);
    }
    
    /**
     * 接受连接回调接口
     */
    public interface OnAcceptListener {
        void onAccept(long connId);
    }
    
    /**
     * 发送数据回调接口
     */
    public interface OnSendListener {
        void onSend(long connId, int length);
    }
    
    /**
     * 连接关闭回调接口
     */
    public interface OnCloseListener {
        void onClose(long connId, int errorCode);
    }
    
    /**
     * 服务器关闭回调接口
     */
    public interface OnShutdownListener {
        void onShutdown();
    }
    
    /**
     * 构造函数
     */
    public UdpServer() {
        nativeHandle = nativeCreate(this);
        if (nativeHandle == 0) {
            throw new RuntimeException("Failed to create native UDP Server");
        }
    }
    
    /**
     * 启动服务器
     * 
     * @param bindAddr 绑定地址，例如 "0.0.0.0" 表示监听所有网卡
     * @param port 端口号
     * @return true表示成功，false表示失败
     */
    public boolean start(String bindAddr, int port) {
        if (nativeHandle == 0) {
            return false;
        }
        return nativeStart(nativeHandle, bindAddr, port);
    }
    
    /**
     * 停止服务器
     * 
     * @return true表示成功，false表示失败
     */
    public boolean stop() {
        if (nativeHandle == 0) {
            return false;
        }
        return nativeStop(nativeHandle);
    }
    
    /**
     * 发送数据到指定连接
     * 
     * @param connId 连接ID
     * @param data 要发送的数据
     * @return true表示成功，false表示失败
     */
    public boolean send(long connId, byte[] data) {
        if (nativeHandle == 0 || data == null) {
            return false;
        }
        return nativeSend(nativeHandle, connId, data);
    }
    
    /**
     * 获取当前连接数
     * 
     * @return 连接数
     */
    public int getConnectionCount() {
        if (nativeHandle == 0) {
            return 0;
        }
        return nativeGetConnectionCount(nativeHandle);
    }
    
    /**
     * 销毁服务器（释放资源）
     * 调用后此对象不能再使用
     */
    public void destroy() {
        if (nativeHandle != 0) {
            nativeDestroy(nativeHandle);
            nativeHandle = 0;
        }
    }
    
    /**
     * 设置接收数据监听器
     */
    public void setOnReceiveListener(OnReceiveListener listener) {
        this.receiveListener = listener;
    }
    
    /**
     * 设置接受连接监听器
     */
    public void setOnAcceptListener(OnAcceptListener listener) {
        this.acceptListener = listener;
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
    
    /**
     * 设置服务器关闭监听器
     */
    public void setOnShutdownListener(OnShutdownListener listener) {
        this.shutdownListener = listener;
    }
    
    // ========== Native回调方法（由C++层调用） ==========
    
    @SuppressWarnings("unused")
    private void onAccept(long connId) {
        if (acceptListener != null) {
            acceptListener.onAccept(connId);
        }
    }
    
    @SuppressWarnings("unused")
    private void onReceive(long connId, byte[] data) {
        if (receiveListener != null) {
            receiveListener.onReceive(connId, data);
        }
    }
    
    @SuppressWarnings("unused")
    private void onSend(long connId, int length) {
        if (sendListener != null) {
            sendListener.onSend(connId, length);
        }
    }
    
    @SuppressWarnings("unused")
    private void onClose(long connId, int errorCode) {
        if (closeListener != null) {
            closeListener.onClose(connId, errorCode);
        }
    }
    
    @SuppressWarnings("unused")
    private void onShutdown() {
        if (shutdownListener != null) {
            shutdownListener.onShutdown();
        }
    }
    
    // ========== Native方法声明 ==========
    
    private native long nativeCreate(Object callback);
    private native boolean nativeStart(long handle, String bindAddr, int port);
    private native boolean nativeStop(long handle);
    private native boolean nativeSend(long handle, long connId, byte[] data);
    private native void nativeDestroy(long handle);
    private native int nativeGetConnectionCount(long handle);
}
