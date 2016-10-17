#include "../header/sock_server.h"



SockServer::SockServer()
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int on = 1, ret = -1;
	ret = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)  );
	bzero(&remoteaddr, sizeof(remoteaddr));
	bzero(&localaddr, sizeof(localaddr));
}

SockServer::SockServer(int local_port)
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int on = 1, ret = -1;
	ret = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)  );

	bzero(&remoteaddr, sizeof(remoteaddr));

	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(local_port);
	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);

}

sockaddr_in& SockServer::get_localaddr()
{
	return localaddr;
}

sockaddr_in& SockServer::get_remoteaddr()
{
	return remoteaddr;
}


void SockServer::set_localaddr(int local_port)
{
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(local_port);
	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
}



int SockServer::_bind()
{
	if(bind(sockfd, (struct sockaddr*)&localaddr, sizeof(localaddr)) < 0)
	{
		cout<< "bind socket failed!" <<endl;
		return -1;
	}
	return 0;
}

int SockServer::_listen()
{
	if(listen(sockfd, MAX_CLIENT_NUM) < 0)
	{
		cout << "listen failed!" << endl;
		return -1;
	}

	return 0;
}

int SockServer::_accept()
{
	socklen_t  clen = sizeof(remoteaddr);
	int cfd = accept(sockfd, (struct sockaddr *) &remoteaddr, &clen);

	if(cfd < 0)
	{
		cout << "accept failed!" << endl;
		return -1;
	}
	return cfd;
}





