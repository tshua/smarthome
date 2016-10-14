#include "../header/protocol.h"
#include "../header/sock_client.h"
#include "../header/common.h"
#include "../header/socket_route.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <iostream>
using namespace std;

#define SER_INFO "./files/SER_IP_PORT"
#define SWITCH1 "./files/DEV/dev4"

int switch_status = 0; //0 关 1开
int msgid;

short ser_port = 0;
char ser_ip[20] = {0};
unsigned char torken[20];
SockClient client;
dev_info dev;

int add_msg(unsigned char* buf)
{
	Msgbuf msgbuf;
	bzero(&msgbuf, sizeof(Msgbuf));

	memcpy(msgbuf.mtext, "switch1", 7);
	memcpy(msgbuf.mtext+10, buf, 10);

	msgbuf.mtype = MSG_DEVTOQT; //设置发送消息的类型           

	int ret = msgsnd(msgid, (void *)&msgbuf, 20, 0); //阻塞发送消息

	if(ret < 0)
		return -1;
	return 1;
}

void read_server_info()
{
	FILE* fp = fopen(SER_INFO, "r");
	fscanf(fp, "%s %d", ser_ip, &ser_port);
	fclose(fp);
}

void read_dev_info()
{
	FILE* fp = fopen(SWITCH1, "r");
	fscanf(fp, "%s %s %s", dev.mac, dev.type, dev.name);
	fclose(fp);
}

void dev_login()
{
	unsigned char buf[MAX_PACKAGE_SIZE] = {0};

	Protocol p;
	p.package_header = 0x55;
	p.cmd_type = 0x0A;
	p.cmd = DEV_LOGIN;			//设备登录

	p.torken_len = 1; 			//无torken
	p.torken = new unsigned char[p.torken_len];
	p.torken[0] = -1;

	p.data = new unsigned char[8];
	memcpy(p.data, dev.mac, 8);
	p.package_tail = 0x55;

	int len = PACKAGE_LEN_EXCEPT_DATA + p.torken_len + 8;
	p.len_low = len & 0x0ff;
	p.len_high = len >> 8;


	p.fill_buf(buf); 	//CRC16校验
	p.CRC_16(buf);
	p.fill_buf(buf);

	client._send(buf);

	bzero(buf, MAX_PACKAGE_SIZE);
	if(RecvPacket(client.sockfd, buf))
	{
		p.parse_buf(buf);
		if(p.torken_len > 1)
		{
			cout << "longin success!" << endl;
			memcpy(torken, p.torken, 20);
		}
		else
		{
			cout << "login failed!" << endl;
		}	

	}
}


void deal_recv_message()
{
	unsigned char buf[MAX_PACKAGE_SIZE];
	Protocol p;
	Protocol p1;

	while(1)
	{
		bzero(buf, MAX_PACKAGE_SIZE);
		bool isRecved = WaitData(client.sockfd, 100000);
		if(isRecved)
		{
			if(RecvPacket(client.sockfd, buf))
			{
				p.clean_data();
				p1.clean_data();
				p.parse_buf(buf);
				int len;
				cout << p.cmd << endl;
				switch(p.cmd)
				{
					
					case CONTRL_DEV_CMD:

						if(strcmp((char*)p.torken, (char*)torken) != 0)//判断torken
						{
							break;
						}

						if(strcmp((char*)p.data, "off") == 0)
						{
							switch_status = 0;
							cout << "switch off" <<endl;
						}
						if(strcmp((char*)p.data, "on") == 0)
						{
							switch_status = 1;
							cout << "switch open" <<endl;
						}

						add_msg(p.data);

						p1.package_header = 0x55;
						p1.cmd_type = 0x0B;
						p1.cmd = RES;                     //应答

						p1.torken_len = 1;                       //无torken
						p1.torken = new unsigned char[p.torken_len];
						p1.torken[0] = -1;

						p1.data = new unsigned char[10];
						memset(p1.data, 0, 10);
						memcpy(p1.data, "success", 8); 

						p1.package_tail = 0x55;

						len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10;
						p1.len_low = len & 0x0ff;
						p1.len_high = len >> 8;

						bzero(buf, MAX_PACKAGE_SIZE);
						p1.fill_buf(buf);        //CRC16校验
						p1.CRC_16(buf);
						p1.fill_buf(buf);
						client._send(buf);


						break;
				}
			}
		}
	}
}


int main()
{
	mk_get_msg(&msgid, MSG_FILE_DEV, 0644, 'a');

	read_server_info();

	read_dev_info();

	client.set_remoteaddr(ser_port, ser_ip);
	client._connect();

	dev_login();
	
	while(1)
	{
		deal_recv_message();
	}
}
