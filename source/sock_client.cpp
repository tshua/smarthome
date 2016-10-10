#include "../header/sock_client.h"
#include "../header/protocol.h"


SockClient::SockClient()
{
	sockfd = socket(PF_INET, SOCK_STREAM, 0);

	bzero(&remoteaddr, sizeof(remoteaddr));
}

SockClient::SockClient(int remote_port, string remote_ip)
{
	sockfd = socket(PF_INET, SOCK_STREAM, 0);

	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_port = htons(remote_port);
	remoteaddr.sin_addr.s_addr = inet_addr(remote_ip.c_str());


}

sockaddr_in& SockClient::get_remoteaddr()
{
	return remoteaddr;
}

void SockClient::set_remoteaddr(int remote_port, string remote_ip)
{
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_port = htons(remote_port);
	remoteaddr.sin_addr.s_addr = inet_addr(remote_ip.c_str());

}

void SockClient::set_remoteaddr(sockaddr_in remoteaddr)
{
	this->remoteaddr = remoteaddr;
}

int SockClient::_connect()
{
	int ret = connect(sockfd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr));
	if(ret < 0)
	{
		cout << "connect failed!" <<endl;
		return -1; 

	}

}

int SockClient::_send(unsigned char *buf)
{
	Protocol p;
	int length = p.transferEncode(buf);
	
	buf[2] = length & 0x0ff; 	//buf[2]  buf[3] 不转义，固定存长度
	buf[3] = length >> 8;
	return send(sockfd, buf, length, 0);
}






