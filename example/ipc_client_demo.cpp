// ipc_client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "../ipc/ipc_client.h"
#pragma comment(lib, "ws2_32.lib")

ipc_client client1;

class ipc_client_socket_callback : public ipc_socket_callback
{
public:
	ipc_client_socket_callback() {}
	virtual ~ipc_client_socket_callback() {}

	virtual void on_connect(const int errorcode, const int sockectnum) { std::cout << "client connect, error code: " << errorcode << std::endl; };
	virtual void on_disconnect(const int errorcode, const std::string summary, const int sockectnum) { std::cout << "client disconnect, error code: " << errorcode << " socket: " << sockectnum << " summary: " << summary << std::endl; };
	virtual void on_send(const unsigned char* data, const int len, const int sockectnum) { /*std::cout << "client send data: " << std::string((const char*)data, len) << std::endl;*/ };
	virtual void on_recv(const unsigned char* data, const int len, const int sockectnum) 
	{ 
		std::cout << "\r\nclient recv data: " << std::string((const char*)data, len) << std::endl;
		std::cout << "sending: ";
		char sendBuf[1024];
		std::cin >> sendBuf;
		client1.send((unsigned char*)sendBuf, strlen(sendBuf));
	};
};

int main()
{
	client1.run("127.0.0.1", 9394, new ipc_client_socket_callback());

	char sendBuf[1024];
	std::cin >> sendBuf;
	client1.send((unsigned char*)sendBuf, strlen(sendBuf));


	while (true)
	{
		Sleep(1000);
	}
	system("pause");
}
