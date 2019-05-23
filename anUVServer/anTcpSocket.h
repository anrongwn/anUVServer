#pragma once
#include "uv.h"
#include <vector>
#include <string>
#include <mutex>
#include <thread>

#define AN_MAX_BUFFER_SIZE	(65536)


class anTcpServer;
class anTcpSocket :
	public uv_tcp_t
{
private:
	struct an_async_req : public uv_async_t {
		explicit an_async_req(anTcpSocket *p) {
			this->data = p;
		}
		an_async_req() = delete;
		~an_async_req() {

		}
	};
public:
	explicit anTcpSocket(const int sessionid);
	~anTcpSocket();

	anTcpSocket() = delete;
	anTcpSocket(const anTcpSocket&) = delete;
	anTcpSocket& operator=(const anTcpSocket&) = delete;

	anTcpSocket(anTcpSocket&&) = delete;
	anTcpSocket& operator=(anTcpSocket&&) = delete;

	inline anTcpServer* get_Server() {
		return reinterpret_cast<anTcpServer*>(this->data);
	}
	inline void set_Server(anTcpServer* obj) {
		this->data = obj;
	}

	void push_data(const char *data, const size_t len);
private:
	using raw_buffer = std::vector<char>;
	using mx_lock = std::lock_guard<std::mutex>;

	//°ü³¤¶È
	using u_len = union {
		char a[sizeof(size_t)];
		size_t x;
	};
public:
	int sessionID_;
	uv_buf_t read_buffer_;
	std::mutex mtx_;
	raw_buffer datas_;
};

