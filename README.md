# anUVServer
使用libuv uv_tcp_* 开发多线路、高并发的tcp server 


#anTcpClient 采用 QTcpSocket ，QTimer 定时轮巡 同步收发机制。

#anUVServer 采用libuv uv_run(uv_loop_, UV_RUN_DEFAULT)  异步事件机制处理所有 new socket 的连接，所有IO 的读写。另使用uv_queue_work 线程池，处理每个socket read到数据。
#测试说明 客户端测试： 局域内 同一PC上开启 10 个客户端，每客户端隔10ms 发送 1093 byte 数据，并同步接收1093 byte 数据。 配置：4G，i5

服务端表现： CPU占用： 约3~11% 内存占用：15.46M 服务线程：7个 配置：4G，i5