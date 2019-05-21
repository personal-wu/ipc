#include "stdafx.h"
#include "ipc_base.h"
#include <process.h>

ipc_socket::ipc_socket(bool async/* = true*/)
	: m_socket(INVALID_SOCKET)
	, m_ipc_socket_callback(nullptr)
	, m_handle_run(nullptr)
	, m_port(0)
	, m_ip("")
{
	InitializeCriticalSection(&m_cs);
	initialize();
	create(async);	
}

ipc_socket::~ipc_socket()
{
	clear();
	InitializeCriticalSection(&m_cs);
}

int ipc_socket::port()
{
	return m_port;
}

std::string ipc_socket::ip()
{
	return m_ip;
}

void ipc_socket::close()
{
	if (INVALID_SOCKET != m_socket)
	{
		::closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		m_ip.clear();
		m_port = 0;
	}
	if (0 != m_handle_run)
	{
		CloseHandle(m_handle_run);
	}
}

bool ipc_socket::run(const std::string& ip, const int port, ipc_socket_callback* callback)
{
	m_ip = ip;
	m_port = port;
	m_ipc_socket_callback = callback;

	unsigned  u_run_thread_id;
	m_handle_run = (HANDLE)::_beginthreadex(NULL, 0, run_ipc_socket, this, 0, &u_run_thread_id);
	return (0 != m_handle_run);
}

bool ipc_socket::initialize()
{
	WSADATA wsd;
	int ret = ::WSAStartup(MAKEWORD(2, 2), &wsd);
	if (ret != 0)
	{
		// TO DO
	}
	return ret == 0;
}

bool ipc_socket::create(bool async)
{
	m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == m_socket)
	{
		return false;
	}

	u_long nonblock = 1;
	if (async && SOCKET_ERROR == ioctlsocket(m_socket, FIONBIO, &nonblock))
	{
		return false;
	}
	return true;
}

void ipc_socket::clear()
{
	close();
	WSACleanup();
}

unsigned __stdcall ipc_socket::run_ipc_socket(void * p)
{
	ipc_socket* p_ipc_socket = (ipc_socket*)p;
	if (nullptr == p_ipc_socket)
	{
		return 0;
	}
	p_ipc_socket->start();
	return 1;
}


ipc_packet::ipc_packet()
	: m_data(nullptr)
	, m_length(0)
{
}

ipc_packet::ipc_packet(const ipc_packet& packet)
	: m_data(nullptr)
	, m_length(0)
{
	copy(packet.m_data, packet.m_length);
}

ipc_packet::ipc_packet(const unsigned char* data, int length)
	: m_data(nullptr)
	, m_length(0)
{
	copy(data, length, true);
}

ipc_packet::~ipc_packet()
{
	m_length = 0;
	if (m_data)
	{
		delete[] m_data;
	}
}

ipc_packet& ipc_packet::operator=(const ipc_packet& packet)
{
	if (this != &packet)
	{
		copy(packet.m_data, packet.m_length);
	}
	return *this;
}

void ipc_packet::copy(const unsigned char* data, int length, bool header/* = false*/)
{
	std::string str_header;
	int packet_total_length = length;
	int header_length = 0;
	if (header)
	{
		char ch[4];
		memcpy(ch, &length, 4);
		std::string value(ch, 4);
		str_header = ipc_packet_header + value;
		header_length = str_header.length();
		packet_total_length += header_length;
	}	
	m_data = new unsigned char[packet_total_length];
	m_length = packet_total_length;
	if (header)
	{
		memcpy(m_data, str_header.c_str(), header_length);
		if (length > 0)
		{
			unsigned char* body = &m_data[header_length];
			memcpy(body, data, length);
		}
	}
	else
	{
		memcpy(m_data, data, packet_total_length);
	}	
}

ipc_socket_io::ipc_socket_io(const SOCKET& s, const ipc_socket_callback* callback)
	: m_io_socket(s)
	, m_callback(callback)
	, m_handle_send(nullptr)
	, m_handle_recv(nullptr)
{
	InitializeCriticalSection(&m_cs);
}

ipc_socket_io::~ipc_socket_io()
{
	CloseHandle(m_handle_send);
	CloseHandle(m_handle_recv);
	DeleteCriticalSection(&m_cs);
}

bool ipc_socket_io::valid()
{
	return (m_callback && m_io_socket != INVALID_SOCKET && !m_stop);
}

bool ipc_socket_io::start_io()
{
	unsigned  u_send_thread_id;
	m_handle_send = (HANDLE)::_beginthreadex(NULL, 0, run_ipc_socket_io_send, this, 0, &u_send_thread_id);
	if (0 == m_handle_send)
	{
		return false;
	}

	unsigned  u_recv_thread_id;
	m_handle_recv = (HANDLE)::_beginthreadex(NULL, 0, run_ipc_socket_io_recv, this, 0, &u_recv_thread_id);
	if (0 == m_handle_recv)
	{
		return false;
	}
	return true;
}

bool ipc_socket_io::stop_io()
{
	if (nullptr == this)
	{
		return false;
	}

	m_stop = true;
	HANDLE handles[2];
	handles[0] = m_handle_send;
	handles[1] = m_handle_recv;
	DWORD ret = WaitForMultipleObjects(2, handles, true, INFINITE);
	return ret != WAIT_FAILED;
}

bool ipc_socket_io::send(const unsigned char* data, const int len)
{
	if (nullptr == this)
	{
		return false;
	}
	ipc_auto_lock ipc_lock(&m_cs);
	m_list_send_packets.push_back(ipc_packet(data, len));
	return true;
}

unsigned __stdcall ipc_socket_io::run_ipc_socket_io_send(void * p)
{
	ipc_socket_io* p_socket_io = (ipc_socket_io*)p;
	if (nullptr == p_socket_io)
	{
		return 0;
	}
	unsigned long keep_alive_begin_tickcout = ::GetTickCount();
	while (p_socket_io->valid())
	{
		while (!p_socket_io->is_send_packets_empty() && !p_socket_io->m_stop)
		{
			ipc_packet packet = p_socket_io->send_packets_front();
			char* data = (char*)packet.m_data;
			int len = packet.m_length;
			while (len > 0)
			{
				data = &data[packet.m_length - len];
				int ret = ::send(p_socket_io->m_io_socket, data, len, 0);
				if (SOCKET_ERROR == ret)
				{
					int error = WSAGetLastError();
					if ((WSAENOTCONN == error || WSAESHUTDOWN == error || WSAECONNRESET == error) && !p_socket_io->m_stop)
					{
						p_socket_io->m_callback->on_disconnect(error, ipc_error_not_connect_0, (int)p_socket_io->m_io_socket);
						p_socket_io->m_stop = true;
					}
					break;
				}
				else
				{
					len -= ret;
					p_socket_io->m_callback->on_send((unsigned char*)data, ret, (int)p_socket_io->m_io_socket);
				}
			}
			p_socket_io->pop_front_send_packets();
			keep_alive_begin_tickcout = ::GetTickCount();
		}

		unsigned long keep_alive_end_tickcout = ::GetTickCount();
		int n_keep_alive_span = (keep_alive_end_tickcout - keep_alive_begin_tickcout) / 1000;
		if (p_socket_io->is_send_packets_empty() && n_keep_alive_span >= keep_alive_span)
		{
			p_socket_io->heartbeat();
			keep_alive_begin_tickcout = keep_alive_end_tickcout;
		}
		Sleep(100);
	}

	return 1;
}

unsigned __stdcall ipc_socket_io::run_ipc_socket_io_recv(void * p)
{
	ipc_socket_io* p_socket_io = (ipc_socket_io*)p;
	if (nullptr == p_socket_io)
	{
		return 0;
	}

	while (p_socket_io->valid())
	{
		bool recv_new_packet = true;
		unsigned char* recv_packet = nullptr;
		int rest_length = 0;
		int packet_length = 0;
		int ret = 0;
		while (true)
		{
			char recv_buff[buff_size];
			::memset(recv_buff, 0, buff_size);
			int length = recv_new_packet ? buff_size : rest_length;
			ret = ::recv(p_socket_io->m_io_socket, recv_buff, length, 0);
			if (SOCKET_ERROR == ret)
			{
				int error = WSAGetLastError();
				if ((WSAENOTCONN == error || WSAESHUTDOWN == error || WSAECONNRESET == error) && !p_socket_io->m_stop)
				{
					p_socket_io->m_callback->on_disconnect(error, ipc_error_not_connect_1, (int)p_socket_io->m_io_socket);
					p_socket_io->m_stop = true;
				}
				rest_length = 0;
				break;
			}
			else
			{
				std::string recv_data(recv_buff, ret);
				if (recv_data.find(ipc_packet_header) == 0)
				{
					if (ret > 22)		// header length
					{
						packet_length = *(int*)recv_data.substr(18).c_str();
						recv_packet = new unsigned char[packet_length];
						recv_new_packet = false;
						rest_length = packet_length - (ret - 22);
						memcpy(recv_packet, recv_data.substr(22).c_str(), ret - 22);
					}
					else
					{
						// TO DO...
					}
				}
				else if (recv_packet != nullptr)
				{
					unsigned char* body = &recv_packet[packet_length - rest_length];
					memcpy(body, recv_buff, ret);
					rest_length -= ret;
				}
			}
			if (rest_length <= 0)
			{
				break;
			}
		}
		if (recv_packet)
		{
			std::string packet_data((char*)recv_packet, packet_length);
			if (SOCKET_ERROR != ret && packet_data != ipc_packet_keep_alive)
			{
				p_socket_io->m_callback->on_recv(recv_packet, packet_length, (int)p_socket_io->m_io_socket);
			}			
			delete[] recv_packet;
			recv_packet = nullptr;
		}
		if (SOCKET_ERROR == ret)
		{
			Sleep(100);
		}
	}

	return 1;
}

bool ipc_socket_io::heartbeat()
{
	std::string keep_alive(ipc_packet_keep_alive);
	return send((unsigned char*)keep_alive.c_str(), keep_alive.length());
}

bool ipc_socket_io::is_send_packets_empty()
{
	ipc_auto_lock ipc_lock(&m_cs);
	return m_list_send_packets.empty();
}

void ipc_socket_io::pop_front_send_packets()
{
	ipc_auto_lock ipc_lock(&m_cs);
	m_list_send_packets.pop_front();
}

ipc_packet ipc_socket_io::send_packets_front()
{
	ipc_auto_lock ipc_lock(&m_cs);
	return m_list_send_packets.front();
}

ipc_auto_lock::ipc_auto_lock(CRITICAL_SECTION* p_cs)
	: m_pcs(p_cs)
{
	if (m_pcs != NULL)
	{
		EnterCriticalSection(m_pcs);
	}
}

ipc_auto_lock::~ipc_auto_lock()
{
	if (m_pcs)
	{
		LeaveCriticalSection(m_pcs);
	}
}
