#ifndef _IPC_BASE_
#define _IPC_BASE_

#include <list>
#include <string>
#include <WinSock2.h>

#define buff_size 4096
#define keep_alive_span	10
#define ipc_packet_header	"ipc_packet_length:"
#define ipc_packet_keep_alive		"keep_alive_packet"
#define ipc_error_not_connect_0	"socket has disconnect while sending data"
#define ipc_error_not_connect_1	"socket has disconnect while receiving data"

class ipc_socket_callback
{
public:
	ipc_socket_callback() {};
	virtual ~ipc_socket_callback() {};

public:
	virtual void on_connect(const int errorcode, const int sockectnum) = 0;
	virtual void on_disconnect(const int errorcode, const std::string summary, const int sockectnum) = 0;
	virtual void on_send(const unsigned char* data, const int len, const int sockectnum) = 0;
	virtual void on_recv(const unsigned char* data, const int len, const int sockectnum) = 0;
};

class ipc_socket
{
public:
	ipc_socket(bool async = true);
	virtual ~ipc_socket();

public:
	int port();
	std::string ip();	
	bool run(const std::string& ip, const int port, ipc_socket_callback* callback);

public:
	virtual bool disconnect(const int errorcode, const std::string reason, int sockectnum = 0) = 0;
	virtual bool send(const unsigned char* data, const int len, int sockectnum = 0) = 0;

protected:
	virtual bool start() = 0;
	void close();

private:
	bool initialize();
	bool create(bool async);
	void clear();

private:
	static unsigned __stdcall run_ipc_socket(void * p);

protected:
	std::string m_ip;
	int	m_port;
	SOCKET m_socket;
	HANDLE m_handle_run;
	ipc_socket_callback* m_ipc_socket_callback;
	CRITICAL_SECTION m_cs;
};

class ipc_packet
{
public:
	ipc_packet();
	ipc_packet(const ipc_packet& packet);
	ipc_packet(const unsigned char* data, int length);
	~ipc_packet();

	ipc_packet& operator=(const ipc_packet& packet);
	void copy(const unsigned char* data, int length, bool header = false);

	unsigned char* m_data;
	int m_length;
};

class ipc_socket_io
{
public:
	ipc_socket_io(const SOCKET& s, ipc_socket_callback* callback);
	virtual ~ipc_socket_io();

public:
	bool valid();
	bool start_io();
	bool stop_io();
	bool send(const unsigned char* data, const int len);

private:
	static unsigned __stdcall run_ipc_socket_io_send(void * p);
	static unsigned __stdcall run_ipc_socket_io_recv(void * p);

	bool heartbeat();
	bool is_send_packets_empty();
	void pop_front_send_packets();
	ipc_packet send_packets_front();

protected:
	bool m_stop;
	SOCKET m_io_socket;
	HANDLE m_handle_send;
	HANDLE m_handle_recv;
	ipc_socket_callback* m_callback;
	std::list<ipc_packet> m_list_send_packets;
	CRITICAL_SECTION m_cs;
};

class ipc_auto_lock
{
public:
	ipc_auto_lock(CRITICAL_SECTION* p_cs);
	~ipc_auto_lock();

private:
	CRITICAL_SECTION * m_pcs;
};

#endif //_IPC_BASE_
