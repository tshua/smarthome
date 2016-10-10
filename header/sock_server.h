#ifndef H_SOCK_H
#define H_SOCK_H


#define MAX_CLIENT_NUM 100

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <arpa/inet.h>

#include <iostream>
using namespace std;

class SockServer
{
	sockaddr_in localaddr;
	sockaddr_in remoteaddr;

	public:
	int sockfd;

	SockServer();

	SockServer(int local_port);

	sockaddr_in& get_localaddr();

	sockaddr_in& get_remoteaddr();
	
	void set_localaddr(int local_port);
	
	int _bind();
	
	int _listen();

	int _accept();

};



#endif
