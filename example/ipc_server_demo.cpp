// ipc_server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "../ipc/ipc_server.h"
#pragma comment(lib, "ws2_32.lib")

ipc_server server1;
class ipc_client_socket_callback : public ipc_socket_callback
{
public:
	ipc_client_socket_callback() {}
	virtual ~ipc_client_socket_callback() {}

	virtual void on_connect(const int errorcode, const int sockectnum) const { std::cout << "server accept connect, error code: " << errorcode << std::endl; };
	virtual void on_disconnect(const int errorcode, const std::string summary, const int sockectnum) const { std::cout << "client disconnect, error code: " << errorcode << " socket: "<< sockectnum << " summary: " << summary << std::endl; };
	virtual void on_send(const unsigned char* data, const int len, const int sockectnum) const { /*std::cout << "server send data: " << std::string((const char*)data, len) << std::endl;*/ };
	virtual void on_recv(const unsigned char* data, const int len, const int sockectnum) const 
	{ 
		std::cout << "\r\nserver recv data: " << std::string((const char*)data, len) << std::endl;
		
		std::cout << "sending: ";
		char sendBuf[1024];
		std::cin >> sendBuf;
		server1.send((unsigned char*)sendBuf, strlen(sendBuf), sockectnum);
	};
};

int main()
{
	server1.run("127.0.0.1", 9394, new ipc_client_socket_callback());

	while (true)
	{
		Sleep(1000);
	}
	system("pause");
}
