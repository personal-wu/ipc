#include "stdafx.h"
#include "ipc_client.h"
#include <WS2tcpip.h>

ipc_client::ipc_client()
	: m_socket_io_obj(nullptr)
{
}

ipc_client::~ipc_client()
{
	stop();
}

bool ipc_client::disconnect(const int errorcode, const std::string reason, int sockectnum/* = 0*/)
{
	stop();
	close();
	return false;
}

bool ipc_client::send(const unsigned char* data, const int len, int sockectnum/* = 0*/)
{
	ipc_auto_lock ipc_lock(&m_cs);
	m_socket_io_obj->send(data, len);
	return false;
}

bool ipc_client::start()
{
	if (!connect())
	{
		return false;
	}

	ipc_auto_lock ipc_lock(&m_cs);
	if (m_socket_io_obj == nullptr)
	{
		m_socket_io_obj = new ipc_socket_io(m_socket, m_ipc_socket_callback);
	}
	return m_socket_io_obj->start_io();
}

bool ipc_client::connect()
{
	struct sockaddr_in addr;
	inet_pton(AF_INET, m_ip.c_str(), &addr.sin_addr);
	addr.sin_port = htons(m_port);
	addr.sin_family = AF_INET;
	bool result = false;
	while (m_socket != INVALID_SOCKET && m_ipc_socket_callback)
	{
		result = (::connect(m_socket, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR);
		int errorcode = WSAGetLastError();
		if (result || WSAEISCONN == errorcode || WSAEALREADY == errorcode)
		{
			result = true;
			m_ipc_socket_callback->on_connect(0, m_socket);
			break;
		}
		else if (WSAEWOULDBLOCK == errorcode || WSAEINVAL == errorcode)
		{
			Sleep(100);
			continue;
		}
		else
		{
			m_ipc_socket_callback->on_connect(errorcode, m_socket);
			break;
		}
	}	
	return result;
}

bool ipc_client::stop()
{
	ipc_auto_lock ipc_lock(&m_cs);
	bool ret = m_socket_io_obj->stop_io();
	delete m_socket_io_obj;
	m_socket_io_obj = nullptr;
	return ret;
}
