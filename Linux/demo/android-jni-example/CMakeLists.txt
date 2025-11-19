cmake_minimum_required(VERSION 3.4.1)

project(hpsocket-jni-example)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# HP-Socket源码路径（可通过命令行参数传入）
# 示例：-DHPSOCKET_SRC_DIR=/path/to/HP-Socket/Linux/src
if(NOT DEFINED HPSOCKET_SRC_DIR)
    # 默认路径：假设在HP-Socket仓库的demo目录下
    set(HPSOCKET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src")
endif()

if(NOT DEFINED HPSOCKET_INCLUDE_DIR)
    set(HPSOCKET_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
endif()

message(STATUS "HP-Socket Source Dir: ${HPSOCKET_SRC_DIR}")
message(STATUS "HP-Socket Include Dir: ${HPSOCKET_INCLUDE_DIR}")

# 定义编译选项（可根据需要启用/禁用功能）
# 禁用不需要的功能可以减小库体积
add_definitions(
    # -D_UDP_DISABLED        # 禁用UDP（不推荐）
    -D_SSL_DISABLED          # 禁用SSL（减小体积）
    -D_HTTP_DISABLED         # 禁用HTTP（减小体积）
    -D_ZLIB_DISABLED         # 禁用ZLIB
    -D_BROTLI_DISABLED       # 禁用BROTLI
    -D_ICONV_DISABLED        # 禁用ICONV
    -D_MIMALLOC_DISABLED     # 禁用mimalloc（Android上推荐禁用）
)

# HP-Socket核心源文件
set(HPSOCKET_COMMON_SOURCES
    ${HPSOCKET_SRC_DIR}/common/BufferPool.cpp
    ${HPSOCKET_SRC_DIR}/common/Event.cpp
    ${HPSOCKET_SRC_DIR}/common/FileHelper.cpp
    ${HPSOCKET_SRC_DIR}/common/FuncHelper.cpp
    ${HPSOCKET_SRC_DIR}/common/IODispatcher.cpp
    ${HPSOCKET_SRC_DIR}/common/PollHelper.cpp
    ${HPSOCKET_SRC_DIR}/common/RWLock.cpp
    ${HPSOCKET_SRC_DIR}/common/SysHelper.cpp
    ${HPSOCKET_SRC_DIR}/common/Thread.cpp
)

# KCP协议支持（UDP ARQ需要）
set(HPSOCKET_KCP_SOURCES
    ${HPSOCKET_SRC_DIR}/common/kcp/ikcp.c
)

# HP-Socket主要模块源文件
set(HPSOCKET_CORE_SOURCES
    ${HPSOCKET_SRC_DIR}/ArqHelper.cpp
    ${HPSOCKET_SRC_DIR}/HPThreadPool.cpp
    ${HPSOCKET_SRC_DIR}/MiscHelper.cpp
    ${HPSOCKET_SRC_DIR}/SocketHelper.cpp
)

# TCP模块（如果需要可以注释掉以减小体积）
set(HPSOCKET_TCP_SOURCES
    ${HPSOCKET_SRC_DIR}/TcpAgent.cpp
    ${HPSOCKET_SRC_DIR}/TcpClient.cpp
    ${HPSOCKET_SRC_DIR}/TcpPackAgent.cpp
    ${HPSOCKET_SRC_DIR}/TcpPackClient.cpp
    ${HPSOCKET_SRC_DIR}/TcpPackServer.cpp
    ${HPSOCKET_SRC_DIR}/TcpPullAgent.cpp
    ${HPSOCKET_SRC_DIR}/TcpPullClient.cpp
    ${HPSOCKET_SRC_DIR}/TcpPullServer.cpp
    ${HPSOCKET_SRC_DIR}/TcpServer.cpp
)

# UDP模块
set(HPSOCKET_UDP_SOURCES
    ${HPSOCKET_SRC_DIR}/UdpArqClient.cpp
    ${HPSOCKET_SRC_DIR}/UdpArqServer.cpp
    ${HPSOCKET_SRC_DIR}/UdpCast.cpp
    ${HPSOCKET_SRC_DIR}/UdpNode.cpp
    ${HPSOCKET_SRC_DIR}/UdpClient.cpp
    ${HPSOCKET_SRC_DIR}/UdpServer.cpp
)

# HP-Socket接口实现
set(HPSOCKET_INTERFACE_SOURCES
    ${HPSOCKET_SRC_DIR}/HPSocket.cpp
)

# 创建HP-Socket静态库
add_library(hpsocket STATIC
    ${HPSOCKET_COMMON_SOURCES}
    ${HPSOCKET_KCP_SOURCES}
    ${HPSOCKET_CORE_SOURCES}
    ${HPSOCKET_TCP_SOURCES}
    ${HPSOCKET_UDP_SOURCES}
    ${HPSOCKET_INTERFACE_SOURCES}
)

# 设置HP-Socket的包含目录
target_include_directories(hpsocket PUBLIC
    ${HPSOCKET_INCLUDE_DIR}
    ${HPSOCKET_SRC_DIR}
)

# 设置HP-Socket编译选项
target_compile_options(hpsocket PRIVATE
    -fPIC
    -fvisibility=hidden
    -fno-strict-aliasing
    -Wall
    -Wextra
    -Wno-deprecated-declarations
    -Wno-unused-parameter
    -Wno-pessimizing-move
    $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-frtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fthreadsafe-statics>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-inconsistent-missing-override>
)

# HP-Socket依赖的系统库
target_link_libraries(hpsocket
    dl
    log
)

# 创建JNI共享库
add_library(hpsocket-jni SHARED
    jni/udp_server_jni.cpp
    jni/udp_client_jni.cpp
)

# JNI库包含目录
target_include_directories(hpsocket-jni PRIVATE
    ${HPSOCKET_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
)

# 链接HP-Socket库
target_link_libraries(hpsocket-jni
    hpsocket
    log
)

# 设置JNI库输出名称
set_target_properties(hpsocket-jni PROPERTIES
    OUTPUT_NAME "hpsocket-jni"
)
