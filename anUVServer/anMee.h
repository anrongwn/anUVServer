#pragma once
#include "uv.h"
#include <mutex>
#include <thread>
#include <atomic>

class anTcpSocket;
class anMee
{
private:
	struct an_work_req :public uv_work_t {
		explicit an_work_req(anTcpSocket *socket) {
			this->data = socket;
		}

		an_work_req() = delete;
	};
public:
	anMee();
	~anMee();

	anMee(const anMee&) = delete;
	anMee& operator=(const anMee&) = delete;
	anMee(anMee&&) = delete;
	anMee& operator=(anMee&&) = delete;
public:
	int start();
	int stop();

	int push_work(anTcpSocket* socket);
private:
	static void thread_func(void * lp);
	static void close_cb(uv_handle_t* handle);
	static void on_walk(uv_handle_t* handle, void* arg);

	static void on_work(uv_work_t* req);
	static void on_after_work(uv_work_t* req, int status);

private:
	using mx_lock = std::lock_guard<std::mutex>;

	//°ü³¤¶È
	using u_len = union {
		char a[sizeof(size_t)];
		size_t x;
	};
private:
	std::mutex mtx_;
	std::atomic_bool flag_ = { ATOMIC_FLAG_INIT };
	uv_thread_t engine_ = { nullptr };
	uv_loop_t * loop_ = {nullptr};
};

