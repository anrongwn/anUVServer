#pragma once
#include "uv.h"
#define AN_MAX_BUFFER_SIZE	(65536)

class anTcpSocket :
	public uv_tcp_t
{
public:
	explicit anTcpSocket(const int sessionid);
	~anTcpSocket();

	anTcpSocket() = delete;
	anTcpSocket(const anTcpSocket&) = delete;
	anTcpSocket& operator=(const anTcpSocket&) = delete;

	anTcpSocket(anTcpSocket&&) = delete;
	anTcpSocket& operator=(anTcpSocket&&) = delete;

public:
	int sessionID_;
	uv_buf_t read_buffer_;
};

