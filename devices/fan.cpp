#include "../header/protocol.h"
#include "../header/sock_client.h"
#include "../header/common.h"
#include "../header/socket_route.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <iostream>
using namespace std;

#define SER_INFO "./files/SER_IP_PORT"
#define FAN "./files/DEV/dev3"

int fan_status = 0; //0 关 1开
int temprature = 10; //温度

int msgid;


int ser_port = 0;
char ser_ip[20] = {0};
unsigned char torken[20];
SockClient client;
dev_info dev;

int add_msg(unsigned char* buf)
{
	Msgbuf msgbuf;
	bzero(&msgbuf, sizeof(Msgbuf));

	memcpy(msgbuf.mtext, "fan1", 4);
	memcpy(msgbuf.mtext+10, buf, 10);

	msgbuf.mtype = MSG_DEVTOQT; //设置发送消息的类型           

	int ret = msgsnd(msgid, (void *)&msgbuf, 20, 0); //阻塞发送消息

	if(ret < 0)
		return -1;
	return 1;
}

void sig_fun(int signo) {

	unsigned char content[10] = "logout";
	add_msg(content);

	exit(0);
}

void read_server_info()
{
	FILE* fp = fopen(SER_INFO, "r");
	fscanf(fp, "%s %d", ser_ip, &ser_port);
	fclose(fp);
}

void read_dev_info()
{
	FILE* fp = fopen(FAN, "r");
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
			
			unsigned char content[10] = "login";
			add_msg(content);
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

						if(strncmp((char*)p.torken, (char*)torken, 20) != 0)//判断torken
						{
							break;
						}

						if(strcmp((char*)p.data, "off") == 0)
						{
							fan_status = 0;
							cout << "fan off" <<endl;
						}
						if(strcmp((char*)p.data, "on") == 0)
						{
							fan_status = 1;
							cout << "fan open" <<endl;
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
					case HEARTBEAT_CMD:
						p1.package_header = 0x55;
						p1.cmd_type = 0x0B;
						p1.cmd = RES;                     //应答

						p1.torken_len = 1;                //无torken
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



void send_temprature()
{

	unsigned char buf[MAX_PACKAGE_SIZE] = {0};


	srand(time(NULL));
	temprature = rand()%20 + 10; //温度范围  10到30

	Protocol p;
	p.package_header = 0x55;
	p.cmd_type = 0x0B;
	p.cmd = TEMPRATURE; 		//温度信息

	p.torken_len = 1; 			//无torken
	p.torken = new unsigned char[p.torken_len];
	p.torken[0] = -1;

	p.data = new unsigned char[10];
	//itoa(light, p.data, 10);
	sprintf((char*)p.data, "%d", temprature);
	p.package_tail = 0x55;

	int len = PACKAGE_LEN_EXCEPT_DATA + p.torken_len + 10;
	p.len_low = len & 0x0ff;
	p.len_high = len >> 8;

	bzero(buf, MAX_PACKAGE_SIZE);
	p.fill_buf(buf); 	//CRC16校验
	p.CRC_16(buf);
	p.fill_buf(buf);

	client._send(buf);
	
	add_msg(p.data);

}

void* thread_temprature_produce(void *arg)
{
	while(1)
	{
		send_temprature();
		sleep(20);
	}
}

int main()
{
	signal(SIGINT, sig_fun);

	mk_get_msg(&msgid, MSG_FILE_DEV, 0644, 'a');

	read_server_info();

	read_dev_info();

	client.set_remoteaddr(ser_port, ser_ip);
	client._connect();

	dev_login();
	
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, thread_temprature_produce, NULL);
	while(1)
	{
		deal_recv_message();
	}
}
