#include "stdafx.h"
#include "ipc_server.h"
#include <WS2tcpip.h>

ipc_server::ipc_server()
{
	m_client_socket_io_objs.clear();
}

ipc_server::~ipc_server()
{
	// TO DO 
}

bool ipc_server::disconnect(const int errorcode, const std::string reason, int sockectnum/* = 0*/)
{
	stop(sockectnum);
	return false;
}

bool ipc_server::send(const unsigned char* data, const int len, int sockectnum/* = 0*/)
{
	ipc_auto_lock ipc_lock(&m_cs);
	return m_client_socket_io_objs[sockectnum]->send(data, len);
}

bool ipc_server::start()
{
	if (!bind())
	{
		return false;
	}
	if (!listen())
	{
		return false;
	}
	if (!accept())
	{
		return false;
	}
	return true;
}

bool ipc_server::accept()
{
	while (m_socket != INVALID_SOCKET && m_ipc_socket_callback)
	{
		sockaddr_in addr_client;
		int addr_client_len = sizeof(addr_client);
		SOCKET client_socket = ::accept(m_socket, (sockaddr FAR*)&addr_client, &addr_client_len);
		if (client_socket == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK || error == WSAEINVAL)
			{
				Sleep(100);
				continue;
			}
			else
			{
				return false;
			}
		}
		else
		{
			ipc_socket_io* socket_io = new ipc_socket_io(client_socket, m_ipc_socket_callback);
			socket_io->start_io();
			m_ipc_socket_callback->on_connect(0, (int)client_socket);

			ipc_auto_lock ipc_lock(&m_cs);
			m_client_socket_io_objs[(int)client_socket] = socket_io;
		}
	}
	return true;
}

bool ipc_server::listen()
{
	int ret = ::listen(m_socket, SOMAXCONN);
	if (SOCKET_ERROR == ret)
	{
		// TO DO
	}
	return (SOCKET_ERROR != ret);
}

bool ipc_server::bind()
{
	struct sockaddr_in addr;
	inet_pton(AF_INET, m_ip.c_str(), &addr.sin_addr);
	addr.sin_port = htons(m_port);
	addr.sin_family = AF_INET;
	int ret = ::bind(m_socket, (LPSOCKADDR)&addr, sizeof(addr));
	int errNo = WSAGetLastError();
	if (SOCKET_ERROR == ret)
	{
		// TO DO
	}
	else if (m_port == 0)
	{
		struct sockaddr_in conn_addr;
		int len = sizeof(conn_addr);
		memset(&conn_addr, 0, len);
		if (SOCKET_ERROR == ::getsockname(m_socket, (sockaddr*)&conn_addr, &len))
		{
			return false;
		}

		m_port = ntohs(conn_addr.sin_port); //  port that randomly allocated by system
	}
	return (SOCKET_ERROR != ret);
}

bool ipc_server::stop(int sockectnum)
{
	ipc_auto_lock ipc_lock(&m_cs);
	ipc_socket_io* socket_io = m_client_socket_io_objs[sockectnum];
	if (!socket_io)
	{
		return false;
	}
	bool ret = socket_io->stop_io();
	m_client_socket_io_objs.erase(sockectnum);
	::closesocket((SOCKET)sockectnum);
	delete socket_io;
	socket_io = nullptr;	
	return ret;
}

