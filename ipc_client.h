#ifndef _IPC_CLIENT_
#define _IPC_CLIENT_

#include "ipc_base.h"

class ipc_client : public ipc_socket
{
public:
	ipc_client();
	virtual ~ipc_client();

public:
	virtual bool disconnect(const int errorcode, const std::string reason, int sockectnum = 0);
	virtual bool send(const unsigned char* data, const int len, int sockectnum = 0);

protected:
	virtual bool start();
	bool connect();
	bool stop();

private:
	ipc_socket_io* m_socket_io_obj;
};

#endif //_IPC_CLIENT_
