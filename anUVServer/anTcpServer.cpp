#include "pch.h"
#include "anTcpServer.h"
#include "anTcpSocket.h"
#include "anMee.h"
#include "anMee2.h"

anTcpServer::anTcpServer(uv_loop_t *loop) : uv_loop_(loop)
{
	//message_handler_ = std::make_unique<anMee>();

	message_handler2_ = std::make_unique<anMee2>(uv_loop_);
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

	//r = message_handler_->start();
	r = message_handler2_->start();
	if (r) {
		anuv::getlogger()->error("anTcpServer::start({}, {})--message_handler_->start={}, {}", \
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

int anTcpServer::wait_exit()
{
	int r = 0;

	//
	message_handler2_->stop();

	uv_walk(uv_loop_, anTcpServer::on_walk, nullptr);
	uv_run(uv_loop_, UV_RUN_DEFAULT);
	do {
		r = uv_loop_close(uv_loop_);
		if (UV_EBUSY == r) {
			uv_run(uv_loop_, UV_RUN_NOWAIT);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));//防占cpu
		//anuv::getlogger()->debug("anTcpServer::wait_exit(). uv_loop_close()={}", r);
	} while (r);

	uv_loop_ = nullptr;

	return r;
}

int anTcpServer::push_work(anTcpSocket * socket)
{
	//return message_handler_->push_work(socket);
	return message_handler2_->push_work(socket);
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
	
	r = uv_tcp_nodelay(&uv_server_, 1);
	if (r) {
		anuv::getlogger()->error("anTcpServer::init()--uv_tcp_nodelay(1)={}, {}", r, anuv::getUVError_Info(r));
		//return r;
	}

	r = uv_tcp_keepalive(&uv_server_, 0, 0);
	if (r) {
		anuv::getlogger()->error("anTcpServer::init()--uv_tcp_keepalive(0, 0)={}, {}", r, anuv::getUVError_Info(r));
		//return r;
	}

	r = uv_signal_init(uv_loop_, &sig_);
	if (r) {
		anuv::getlogger()->error("anTcpServer::init()--uv_signal_init={}, {}", r, anuv::getUVError_Info(r));
		//return r;
	}

	sig_.data = this;
	r = uv_signal_start(&sig_, anTcpServer::on_signal, SIGINT);
	if (r) {
		anuv::getlogger()->error("anTcpServer::init()--uv_signal_start={}, {}", r, anuv::getUVError_Info(r));
		//return r;
	}

	return r;
}

/*
int anTcpServer::shutdown()
{
	std::string log = fmt::format("anTcpServer::shutdown(), ");
	int r = 0;

	if (!uv_is_active((uv_handle_t*)&this->uv_server_)) {
		log+= fmt::format("uv_is_active({:#08x}) unactive, ", (int)&this->uv_server_);
		anuv::getlogger()->error(log);
		return r;
	}

	
	uv_shutdown_t * req = new uv_shutdown_t;
	req->data = this;
	r = uv_shutdown(req, (uv_stream_t*)&this->uv_server_, anTcpServer::on_shutdown);
	if (r) {
		log += fmt::format("uv_shutdown(req={:#08x})={},{}, ", (int)req, r, anuv::getUVError_Info(r));

		delete req;
		anuv::getlogger()->error(log);
	}

	return r;
}
*/

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
	anuv::getlogger()->info(log);

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
			if (!uv_is_closing((uv_handle_t*)socket)) {
				uv_close(reinterpret_cast<uv_handle_t*>(socket), anTcpServer::on_close);
			}
			break;
		default:
			break;
		}

	}
	else if (0 == nread) {

	}
	else if (nread > 0) {
		client->push_data(buf->base, nread);

		anTcpServer *server = client->get_Server();
		server->push_work(client);
	}

	anuv::getlogger()->info(log);
}

void anTcpServer::on_close(uv_handle_t * handle)
{
	std::string log = fmt::format("anTcpServer::on_close({:#08x}), ", (int)handle);

	if (UV_ASYNC == handle->type) {

	}
	else if (UV_TCP == handle->type) {
		//从连接中清除
		anTcpSocket * socket = reinterpret_cast<anTcpSocket *>(handle);
		//anTcpServer * server = reinterpret_cast<anTcpServer *>(handle->data);
		
		log += fmt::format("clear_session(sessionid={}, client={:#08x})", socket->sessionID_, (int)handle);

		//通知退出
		socket->get_Server()->clear_session(socket->sessionID_);
	}

	anuv::getlogger()->info(log);
}

void anTcpServer::on_walk(uv_handle_t * handle, void * arg)
{
	if (!uv_is_closing(handle)) {
		uv_close(handle, nullptr);
	}
}

void anTcpServer::on_signal(uv_signal_t * handle, int signum)
{
	std::string log = fmt::format("anTcpServer::on_signal({:#08x}, {}), ", (int)handle, signum);
	int r = 0;
	anTcpServer * that = reinterpret_cast<anTcpServer *>(handle->data);

	//shutdown
	//r = that->shutdown();
	//
	uv_stop(that->uv_loop_);

	uv_close((uv_handle_t*)&that->uv_server_, nullptr);

	uv_signal_stop(handle);

	anuv::getlogger()->info(log);
}

/*
void anTcpServer::on_shutdown(uv_shutdown_t * req, int status)
{
	std::string log = fmt::format("anTcpServer::on_shutdown({:#08x}, {}), ", (int)req, status);
	int r = 0;
	anTcpServer * that = reinterpret_cast<anTcpServer *>(req->data);

	//
	uv_stop(that->uv_loop_);

	delete req;
	
	anuv::getlogger()->info(log);

}
*/

void anTcpServer::clear_session(const int serssionid)
{
	auto it = client_lists_.find(serssionid);
	if (it != client_lists_.end()) {
		delete it->second;

		client_lists_.erase(it);
	}
}
