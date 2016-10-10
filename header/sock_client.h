#ifndef H_SOCK_CLIENT_H
#define H_SOCK_CLIENT_H


#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <arpa/inet.h>
#include <string>
#include <iostream>
using namespace std;

class SockClient
{
	sockaddr_in remoteaddr;

	public:
	int sockfd;

	SockClient();

	SockClient(int remote_port, string remote_ip);

	sockaddr_in& get_remoteaddr();
	
	void set_remoteaddr(int remote_port, string remote_ip);

	void set_remoteaddr(sockaddr_in remoteaddr);

	int _connect();

	int _send(unsigned char *buf);

};



#endif
