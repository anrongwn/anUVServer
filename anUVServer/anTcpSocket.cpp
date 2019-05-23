#include "pch.h"
#include "anTcpSocket.h"


anTcpSocket::anTcpSocket(const int sessionid) : sessionID_(sessionid)
{
	read_buffer_ = uv_buf_init((char*)malloc(AN_MAX_BUFFER_SIZE), AN_MAX_BUFFER_SIZE);
}


anTcpSocket::~anTcpSocket()
{
	if (read_buffer_.base) {
		free(read_buffer_.base);
		read_buffer_.len = 0;
		read_buffer_.base = nullptr;
	}
}

void anTcpSocket::push_data(const char * data, const size_t len)
{
	mx_lock lk(mtx_);
	datas_.insert(datas_.end(), data, data + len);

}
