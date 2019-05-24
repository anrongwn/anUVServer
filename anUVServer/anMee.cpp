#include "pch.h"
#include "anMee.h"
#include "anTcpSocket.h"


anMee::anMee()
{
}


anMee::~anMee()
{
}

void anMee::thread_func(void * lp)
{
	anMee * that = static_cast<anMee*>(lp);

	if (std::atomic_exchange(&that->flag_, true)) return;

	//main run message loop
	int more = 0;
	while (that->flag_) {
		more = uv_run(that->loop_, UV_RUN_NOWAIT);
		if (more) {
			continue;
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));//防占cpu
		}
	}

	//fully close handle
	uv_walk(that->loop_, anMee::on_walk, nullptr);
	uv_run(that->loop_, UV_RUN_DEFAULT);
	do {
		more = uv_loop_close(that->loop_);
		if (UV_EBUSY == more) {
			uv_run(that->loop_, UV_RUN_NOWAIT);
		}
	} while (more);

	//
	delete that->loop_;
}

void anMee::close_cb(uv_handle_t * handle)
{
	if (handle->type == UV_WORK) {
		delete handle;
	}
}

void anMee::on_walk(uv_handle_t * handle, void * arg)
{
	if (!uv_is_closing(handle)) {
		uv_close(handle, nullptr);
	}
}

void anMee::on_work(uv_work_t * req)
{
	an_work_req * that = static_cast<an_work_req*>(req);
	std::string log = fmt::format("anMee::on_work({:#08x}), tid={:#08x}", (int)that, uv_thread_self());
	anTcpSocket *client = reinterpret_cast<anTcpSocket*>(that->data);


	anuv::getlogger()->info(log);
}

void anMee::on_after_work(uv_work_t * req, int status)
{
}

int anMee::start()
{
	int r = 0;
	if (engine_) return r;

	loop_ = new uv_loop_t;
	r = uv_loop_init(loop_);

	//自起线程处理
	r = uv_thread_create(&engine_, anMee::thread_func, this);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));

	return r;
}

int anMee::stop()
{
	int r = 0;
	if (!std::atomic_exchange(&this->flag_, false)) return r;

	//针对自启uv_run
	if ((loop_) && (engine_)) {
		uv_stop(loop_);
		//uv_loop_close(loop_);
		//delete loop_;

	}
	uv_thread_join(&engine_);
	loop_ = nullptr;
	return r;
}

int anMee::push_work(anTcpSocket * socket)
{
	int r = 0;
	std::string log = fmt::format("anMee::push_work({:#08x}), ", (int)socket);

	an_work_req *work = new an_work_req(socket);
	r = uv_queue_work(this->loop_, work, anMee::on_work, anMee::on_after_work);
	if (r) {
		log += fmt::format("uv_queue_work()={}, {}", r, anuv::getUVError_Info(r));
	}

	anuv::getlogger()->info(log);
	return r;
}
