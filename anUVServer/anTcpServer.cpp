#include "pch.h"
#include "anTcpServer.h"
#include "anTcpSocket.h"

anTcpServer::anTcpServer(uv_loop_t *loop) : uv_loop_(loop)
{
	
}


anTcpServer::~anTcpServer()
{
}

int anTcpServer::start(const char * addr, const unsigned short port)
{
	int r = 0;

	r = init();
	if (r) return r;

	struct sockaddr_in bind_addr;
	r = uv_ip4_addr(addr, port, &bind_addr);
	if (r) {
		anuv::getlogger()->error("anTcpServer::start({}, {})--uv_ip4_addr={}, {}", \
			addr, port,r,anuv::getUVError_Info(r));
		return r;
	}

	r = uv_tcp_bind(&uv_server_, reinterpret_cast<struct sockaddr*>(&bind_addr), 0);
	if (r) {
		anuv::getlogger()->error("anTcpServer::start({}, {})--uv_tcp_bind={}, {}", \
			addr, port, r, anuv::getUVError_Info(r));
		return r;
	}
	
	r = uv_listen(reinterpret_cast<uv_stream_t*>(&uv_server_), 1024, anTcpServer::on_new_connection);
	if (r) {
		anuv::getlogger()->error("anTcpServer::start({}, {})--uv_listen={}, {}", \
			addr, port, r, anuv::getUVError_Info(r));
		return r;
	}

	return r;
}

int anTcpServer::run()
{
	int r = 0;

	r = uv_run(uv_loop_, UV_RUN_DEFAULT);

	return r;
}

int anTcpServer::init()
{
	int r = 0;

	if (std::atomic_exchange(&init_, true)) return r;

	r = uv_tcp_init(uv_loop_, &uv_server_);
	if (r) {
		anuv::getlogger()->error("anTcpServer::init()--uv_tcp_init={}, {}", r, anuv::getUVError_Info(r));
		return r;
	}
	uv_handle_set_data(reinterpret_cast<uv_handle_t*>(&uv_server_), this);
	//uv_server_.data = this;
	
	r = uv_tcp_nodelay(&uv_server_, 0);
	if (r) {
		anuv::getlogger()->error("anTcpServer::init()--uv_tcp_nodelay={}, {}", r, anuv::getUVError_Info(r));
		//return r;
	}

	return r;
}

void anTcpServer::on_new_connection(uv_stream_t * server, int status)
{
	std::string log = fmt::format("anTcpServer::on_new_connection({:#08x}, {}), ", (int)server, status);
	if (status < 0) {
		if (UV_EOF == status) {
			uv_close(reinterpret_cast<uv_handle_t*>(server), nullptr);
		}
		
		log += anuv::getUVError_Info(status);
		anuv::getlogger()->error(log);

		return;
	}

	int r = 0;
	anTcpServer * that = reinterpret_cast<anTcpServer*>(server->data);
	int sessionid = anTcpServer::get_sessionID();
	anTcpSocket *client = new anTcpSocket(sessionid);

	r = uv_tcp_init(that->uv_loop_, client);
	if (r) {
		log += fmt::format("uv_tcp_init={}, {}", r, anuv::getUVError_Info(r));
		anuv::getlogger()->error(log);

		delete client;
		return;
	}

	client->set_Server(that);
	r = uv_accept(reinterpret_cast<uv_stream_t*>(&that->uv_server_), reinterpret_cast<uv_stream_t*>(client));
	if (r) {
		log += fmt::format("uv_accept={}, {}", r, anuv::getUVError_Info(r));
		anuv::getlogger()->error(log);

		uv_close(reinterpret_cast<uv_handle_t*>(client), nullptr);
		delete client;
		return;
	}

	that->client_lists_.insert(std::make_pair(sessionid, client));

	r = uv_read_start(reinterpret_cast<uv_stream_t*>(client), anTcpServer::alloc_buffer, anTcpServer::on_read);
	if (r) {
		log += fmt::format("uv_read_start={}, {}", r, anuv::getUVError_Info(r));
		anuv::getlogger()->error(log);

		uv_close(reinterpret_cast<uv_handle_t*>(client), nullptr);
		delete client;
		return;
	}

	log += fmt::format("sessionid={}, client={:#08x}", sessionid, (int)client);
	anuv::getlogger()->error(log);

}

void anTcpServer::alloc_buffer(uv_handle_t * handle, size_t suggested_size, uv_buf_t * buf)
{
	
	anTcpSocket * client = reinterpret_cast<anTcpSocket*>(handle);
	if (client) {
		*buf = client->read_buffer_;
	}

}

void anTcpServer::on_read(uv_stream_t * socket, ssize_t nread, const uv_buf_t * buf)
{
	std::string log = fmt::format("anTcpServer::on_read({:#08x}, {})", (int)socket, nread);
	
	anTcpSocket * client = reinterpret_cast<anTcpSocket*>(socket);
	if (nread < 0) {
		switch (nread) {
		case UV_EOF:	//主动断开
			if (!uv_is_closing((uv_handle_t*)socket)) {
				uv_close(reinterpret_cast<uv_handle_t*>(socket), anTcpServer::on_close);
			}
			break;
		case UV_ECONNRESET:	//异常断开
			break;
		default:
			break;
		}

	}
	else if (0 == nread) {

	}
	else if (nread > 0) {

	}

	anuv::getlogger()->info(log);
}

void anTcpServer::on_close(uv_handle_t * handle)
{
	std::string log = fmt::format("anTcpServer::on_close({:#08x})", (int)handle);

	if (UV_ASYNC == handle->type) {

	}
	else if (UV_TCP == handle->type) {
		//从连接中清除
		anTcpServer * that = reinterpret_cast<anTcpServer *>(handle->data);
		

		//通知退出
		
	}

	anuv::getlogger()->info(log);
}
