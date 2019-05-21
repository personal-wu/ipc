#ifndef _IPC_SERVER_
#define _IPC_SERVER_

#include <map>
#include "ipc_base.h"

class ipc_server: public ipc_socket
{
public:
	ipc_server();
	virtual ~ipc_server();

public:
	virtual bool disconnect(const int errorcode, const std::string reason, int sockectnum = 0);
	virtual bool send(const unsigned char* data, const int len, int sockectnum = 0);

protected:
	virtual bool start();
	bool accept();
	bool listen();
	bool bind();
	bool stop(int sockectnum);

private:
	std::map<int, ipc_socket_io*> m_client_socket_io_objs;
};

#endif //_IPC_SERVER_
