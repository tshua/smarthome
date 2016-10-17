#include "../header/protocol.h"
#include "../header/sock_client.h"
#include "../header/common.h"
#include "../header/socket_route.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <vector>
#include <list>
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <iostream>
using namespace std;

#define SER_INFO "./files/SER_IP_PORT"
#define PHONE1 "./files/MT/phone1"
#define SME_FILE "./files/semfile_phone"


char torken[20] = {0}; 	//记录本次会话的torken
phone_info phone1;

int msgid;
list<dev_info_e> dev_online;

int temprature = 0;
int light1 = 0;
int light2 = 0;
SockClient client;

int semid_phone_dev_online; //用于控制phone端设备信息读写同步的信号量
int count_phone_dev_online = 0;



//读与读同步,读与写互斥
int write_sync_lock(int semid)
{
	sem_p(semid, 1, 1);
}

int write_sync_unlock(int semid)
{
	sem_v(semid, 1, 1);
}


//读与读同步,读与写互斥
int read_sync_lock(int semid, int& count)
{
	sem_p(semid, 0, 1);//count 可操作
	if(count == 0) //阻止写进程
		sem_p(semid, 1, 1);
	count++;
	sem_v(semid, 0, 1);
}

int read_sync_unlock(int semid, int& count)
{
	sem_p(semid, 0, 1);
	count--;
	if(count == 0)
		sem_v(semid, 1, 1); //设置可写
	sem_v(semid, 0, 1);
}

int add_msg(unsigned char* buf, int size)
{
	Msgbuf msgbuf;
	bzero(&msgbuf, sizeof(Msgbuf));

	memcpy(msgbuf.mtext, buf, size);

	msgbuf.mtype = MSG_PHONETOQT; //设置发送消息的类型           
	
	cout << "msg" << buf << " "<<  buf+10 << endl;
	int ret = msgsnd(msgid, (void *)&msgbuf, size, 0); //阻塞发送消息

	if(ret < 0)
		return -1;
	return 1;
}

int search_dev_from_dev_online(dev_info_e& dev_e) //根据名称查询所有的设备信息
{

	read_sync_lock(semid_phone_dev_online, count_phone_dev_online);

	int find_ok = 0;
	list<dev_info_e>::iterator it = dev_online.begin();
	for(; it != dev_online.end(); it++)
	{
		if(strcmp((char*)dev_e.d.name, it->d.name) == 0)
		{
			memcpy(&dev_e, &(*it), sizeof(dev_info_e));
			find_ok = 1;
			break;
		}
	}


	read_sync_unlock(semid_phone_dev_online, count_phone_dev_online);
	return find_ok;
}

int regist_phone(phone_info& phone)
{
	unsigned char buf[MAX_PACKAGE_SIZE] = {0};

	Protocol p;
	p.package_header = 0x55;
	p.cmd_type = 0x0A;
	p.cmd = REGIST_CMD; 			//注册手机

	p.torken_len = 1; 			//无torken
	p.torken = new unsigned char[p.torken_len];
	p.torken[0] = -1;

	p.data = new unsigned char[sizeof(phone_info) - 1];
	memcpy(p.data, &phone, sizeof(phone_info)-1);

	p.package_tail = 0x55;

	int len = PACKAGE_LEN_EXCEPT_DATA + p.torken_len + sizeof(phone_info) - 1;
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
		if(strcmp((char*)p.data, "success") == 0)
			cout << "注册手机成功。" << endl;
		else
		{
			cout << "注册手机失败。" << endl;
			return -1;
		}
	}
	else
	{
		return -1;
	}

	return 1;
}


/*
 *读出手机相关信息
 **/
void phone_read_info(phone_info& phone)
{
	FILE *fp = fopen(PHONE1, "r");
	fscanf(fp, "%s %s %s %s %c", phone.phone_num, phone.phone_name,\
			phone.password, phone.mail, &phone.is_regist );
	fclose(fp);

	if(phone.is_regist == '0')
	{
		if(regist_phone(phone))
		{
			fp = fopen(PHONE1, "w");
			cout << "回写信息" << endl;
			phone.is_regist = '1';
			fprintf(fp, "%s %s %s %s %c", phone.phone_num, phone.phone_name,\
					phone.password, phone.mail, phone.is_regist );
			fclose(fp);
		}
		else
		{
			cout << "regist failed!" << endl;
			exit(-1);
		}
	}


}

/*
 * 模拟smartlink 手机将服务器及路由器信息写入文件，设备读文件
 * */
void phone_write_server_info()
{
	FILE *fp = fopen(SER_INFO, "w");
	fprintf(fp, "%s %d\n", SERADDR, SERPORT);
	fclose(fp);	
}

int phone_login(phone_info& phone)
{
	unsigned char buf[MAX_PACKAGE_SIZE] = {0};

	Protocol p;
	p.package_header = 0x55;
	p.cmd_type = 0x0A;
	p.cmd = LOGIN_CMD;			//手机登录

	p.torken_len = 1; 			//无torken
	p.torken = new unsigned char[p.torken_len];
	p.torken[0] = -1;

	p.data = new unsigned char[30];
	memcpy(p.data, phone.phone_name,10);
	memcpy(p.data + 10, phone.password,20);

	p.package_tail = 0x55;

	int len = PACKAGE_LEN_EXCEPT_DATA + p.torken_len + 30;
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
			memcpy(torken, p.data, 20);
		}
		else
		{
			cout << "login failed!" << endl;
		}
	}
	else
	{
		cout << "login failed!" << endl;
		return -1;
	}

	return 1;

}

int regist_device()
{
	unsigned char buf[MAX_PACKAGE_SIZE] = {0};
	char base[101] = {0};
	vector<string> devs;
	getcwd(base, 100);

	memcpy(base+strlen(base), "/files/DEV/", 11);
	DIR *dir = opendir(base);
	struct dirent *ptr;
	string b = base;
	while((ptr=readdir(dir)) != NULL)
	{
		if(ptr->d_type == 8) 	//file
		{
			devs.push_back(b + ptr->d_name);
		}
	}

	FILE *fp = NULL;
	dev_info dev;
	bzero(&dev, sizeof(dev_info));
	for(vector<string>::iterator it = devs.begin(); it != devs.end(); it++)
	{
		fp = fopen((*it).c_str(), "r");
		fscanf(fp, "%s %s %s", dev.mac, dev.type, dev.name);
		fclose(fp);
		Protocol p;
		p.package_header = 0x55;
		p.cmd_type = 0x0A;
		p.cmd = REGIST_DEV_CMD; 		//注册设备信息

		p.torken_len = 1; 			//无torken
		p.torken = new unsigned char[p.torken_len];
		p.torken[0] = -1;

		p.data = new unsigned char[sizeof(dev_info)];
		memcpy(p.data, &dev, sizeof(dev_info));

		p.package_tail = 0x55;

		int len = PACKAGE_LEN_EXCEPT_DATA + p.torken_len + sizeof(dev_info);
		p.len_low = len & 0x0ff;
		p.len_high = len >> 8;

		p.fill_buf(buf); 	//CRC16校验
		p.CRC_16(buf);
		p.fill_buf(buf);

		client._send(buf);

		bzero(buf, MAX_PACKAGE_SIZE);
		p.clean_data();
		if(RecvPacket(client.sockfd, buf))
		{
			p.parse_buf(buf);

			if(strcmp((char*)p.data, "success") == 0)
				cout << dev.name << "regist success" << endl;
			else
			{
				cout << "设备注册失败。" << endl;
				break;
			}
		}
		else
		{
			cout << "recv_pack error!" << endl;
			break;
		}

	}
}

void* thread_sync_dev_online(void* arg) //10s同步一次
{

	unsigned char buf[MAX_PACKAGE_SIZE] = {0};
	unsigned char msg_data[20] = {0};

	while(1)
	{

		Protocol p;
		p.package_header = 0x55;
		p.cmd_type = 0x0B;
		p.cmd = GET_DEV_SENCEINFO_CMD; 		//同步登录的设备信息

		p.torken_len = 1;                       //无torken
		p.torken = new unsigned char[p.torken_len];
		p.torken[0] = -1; 

		p.data = new unsigned char[20];
		memset(p.data, 0, 20);
		memcpy(p.data, phone1.phone_num, 20);

		p.package_tail = 0x55;

		int len = PACKAGE_LEN_EXCEPT_DATA + p.torken_len + 20; 
		p.len_low = len & 0x0ff;
		p.len_high = len >> 8;


		p.fill_buf(buf);        //CRC16校验
		p.CRC_16(buf);
		p.fill_buf(buf);

		client._send(buf);

		bzero(buf, MAX_PACKAGE_SIZE);
		if(RecvPacket(client.sockfd, buf))
		{
			write_sync_lock(semid_phone_dev_online);

			dev_online.clear();
			p.clean_data();
			p.parse_buf(buf);

			int dev_num = p.extend_info[0];

			dev_info_e dev_e;
			unsigned char* pos = p.data;
			for(int i = 0;i < dev_num; i++)
			{
				bzero(&dev_e, sizeof(dev_info_e));
				bzero(msg_data, 20);

				memcpy(&dev_e, pos, sizeof(dev_info_e));
				pos += sizeof(dev_info_e);
				dev_online.push_back(dev_e);

				memcpy(msg_data, dev_e.d.name, 10);
				memcpy(msg_data + 10, dev_e.status?"on\0":"off", 3);
				add_msg(msg_data, 20);
				if(strncmp(dev_e.d.type , "lamp", 4) == 0) 	//如果是灯 发送自动手动信息
				{
					bzero(msg_data, 20);
					memcpy(msg_data, dev_e.d.name, 10);
					memcpy(msg_data + 10, dev_e.lamp_auto?"auto\0\0":"manual", 6);
					add_msg(msg_data, 20);
				}
			}

			light1 = atoi((char*)pos);
			bzero(msg_data, 20);
			memcpy(msg_data, "lamp1", 5);
			memcpy(msg_data+10, pos, 10);
			add_msg(msg_data, 20);

			light2 = atoi((char*)(pos + 10));
			bzero(msg_data, 20);
			memcpy(msg_data, "lamp2", 5);
			memcpy(msg_data+10, pos+10, 10);
			add_msg(msg_data, 20);

			temprature = atoi((char*)(pos + 20));
			bzero(msg_data, 20);
			memcpy(msg_data, "fan1", 10);
			memcpy(msg_data+10, pos+20, 10);
			add_msg(msg_data, 20);
			
			write_sync_unlock(semid_phone_dev_online);
		}
		else
		{
			cout << "sync dev_online error!" << endl;
		}

		//add_msg(p.data, p.extend_info[0]*sizeof(dev_info_e));

		sleep(1);
	}
}

void output_select()
{
	cout << "p/print the devs." << endl;
}


void output_select_lamp()
{
	cout << "on/for open." << endl;
	cout << "off/for close." << endl;
	cout << "auto/for auto control." <<endl;
	cout << "manual/for manual control." <<endl;
}
void output_select_fan_swutch()
{
	cout << "on/for open." << endl;
	cout << "off/for close." << endl;
}

void *thread_input(void *arg){
	unsigned char buf[MAX_PACKAGE_SIZE];
	char c;
	string option;
	int i;
	while(1)
	{
		cin >> c;
		if(c == 'p')
		{
			i = 0;
			read_sync_lock(semid_phone_dev_online, count_phone_dev_online);

			list<dev_info_e>::iterator it = dev_online.begin();
			for(; it != dev_online.end(); it++)
			{
				cout << i << ". "<< it->d.name << endl;
				i++;
			}
			cout << "input the device num." << endl;
			cin >> i;
			if(i >= dev_online.size() || i < 0)
			{
				cout << "device num not exists!" << endl;
				continue;
			}
			it = dev_online.begin();
			while(i > 0 && it != dev_online.end())
			{
				it++;
				i--;
			}

			if(strcmp(it->d.type, "lamp") == 0)
			{

				output_select_lamp();
				cout << "input control info:" << endl;
				cin >> option;
				if(option == "on")
				{

					dev_info_e dev = *it;
					cout << dev.d.mac << endl;
					cout << "torken" << dev.torken <<endl;
					cout << "send on lamp!" << endl;
					Protocol p1;

					p1.package_header = 0x55;
					p1.cmd_type = 0x0B;
					p1.cmd = CONTRL_DEV_CMD;                 //控制设备
					memcpy(p1.device_id, it->d.mac, 8);


					p1.torken_len = 20;
					p1.torken = new unsigned char[p1.torken_len];
					memcpy(p1.torken, dev.torken, 20);

					p1.data = new unsigned char[10];
					memset(p1.data, 0, 10);
					memcpy(p1.data, "on", 2); 

					p1.package_tail = 0x55;

					int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
					p1.len_low = len & 0x0ff;
					p1.len_high = len >> 8;

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.fill_buf(buf);        //CRC16校验
					p1.CRC_16(buf);
					p1.fill_buf(buf);
					client._send(buf);

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.clean_data();
					if(RecvPacket(client.sockfd, buf))
					{
						p1.parse_buf(buf);
						if(strcmp((char*)p1.data, "success") == 0)
						{
							cout << "recv ack!" << endl;
							it->status = 1;
						}

					}
					else
					{
						cout << "set lamp error!" << endl;
					}

				}
				if(option == "off")
				{

					dev_info_e dev = *it;
					cout << dev.d.mac << endl;
					cout << "torken" << dev.torken <<endl;
					cout << "send off lamp!" << endl;
					Protocol p1;

					p1.package_header = 0x55;
					p1.cmd_type = 0x0B;
					p1.cmd = CONTRL_DEV_CMD;                 //控制设备
					memcpy(p1.device_id, it->d.mac, 8);

					p1.torken_len = 20;                       //无torken
					p1.torken = new unsigned char[p1.torken_len];
					memcpy(p1.torken, dev.torken, 20);

					p1.data = new unsigned char[10];
					memset(p1.data, 0, 10);
					memcpy(p1.data, "off", 3); 

					p1.package_tail = 0x55;

					int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
					p1.len_low = len & 0x0ff;
					p1.len_high = len >> 8;

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.fill_buf(buf);        //CRC16校验
					p1.CRC_16(buf);
					p1.fill_buf(buf);
					client._send(buf);

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.clean_data();
					if(RecvPacket(client.sockfd, buf))
					{
						p1.parse_buf(buf);
						if(strcmp((char*)p1.data, "success") == 0)
						{
							cout << "recv ack!" << endl;
							it->status = 0;
						}

					}
					else
					{
						cout << "set lamp error!" << endl;
					}
				}
				if(option == "auto")
				{

					dev_info_e dev = *it;
					cout << dev.d.mac << endl;
					cout << "torken" << dev.torken <<endl;
					cout << "auto lamp!" << endl;
					Protocol p1;

					p1.package_header = 0x55;
					p1.cmd_type = 0x0B;
					p1.cmd = CONTRL_DEV_CMD;                 //控制设备
					memcpy(p1.device_id, it->d.mac, 8);

					p1.torken_len = 20;                       //无torken
					p1.torken = new unsigned char[p1.torken_len];
					memcpy(p1.torken, dev.torken, 20);

					p1.data = new unsigned char[10];
					memset(p1.data, 0, 10);
					memcpy(p1.data, "auto", 4); 

					p1.package_tail = 0x55;

					int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
					p1.len_low = len & 0x0ff;
					p1.len_high = len >> 8;

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.fill_buf(buf);        //CRC16校验
					p1.CRC_16(buf);
					p1.fill_buf(buf);
					client._send(buf);

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.clean_data();
					if(RecvPacket(client.sockfd, buf))
					{
						p1.parse_buf(buf);
						if(strcmp((char*)p1.data, "success") == 0)
						{
							cout << "recv ack!" << endl;
							it->lamp_auto = 1;
						}

					}
					else
					{
						cout << "set auto error!" << endl;
					}
				}
				if(option == "manual")
				{

					dev_info_e dev = *it;
					cout << dev.d.mac << endl;
					cout << "torken" << dev.torken <<endl;
					cout << "send manual lamp!" << endl;
					Protocol p1;

					p1.package_header = 0x55;
					p1.cmd_type = 0x0B;
					p1.cmd = CONTRL_DEV_CMD;                 //控制设备
					memcpy(p1.device_id, it->d.mac, 8);

					p1.torken_len = 20;                       //无torken
					p1.torken = new unsigned char[p1.torken_len];
					memcpy(p1.torken, dev.torken, 20);

					p1.data = new unsigned char[10];
					memset(p1.data, 0, 10);
					memcpy(p1.data, "manual", 6); 

					p1.package_tail = 0x55;

					int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
					p1.len_low = len & 0x0ff;
					p1.len_high = len >> 8;

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.fill_buf(buf);        //CRC16校验
					p1.CRC_16(buf);
					p1.fill_buf(buf);
					client._send(buf);

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.clean_data();
					if(RecvPacket(client.sockfd, buf))
					{
						p1.parse_buf(buf);
						if(strcmp((char*)p1.data, "success") == 0)
						{
							cout << "recv ack!" << endl;
							it->lamp_auto = 0;
						}

					}
					else
					{
						cout << "set manual error!" << endl;
					}
				}
			}
			else if(strcmp(it->d.type, "fan") == 0 || strcmp(it->d.type, "switch") == 0)
			{
				output_select_fan_swutch();
				cout << "input control info:" << endl;
				cin >> option;
				if(option == "on")
				{

					dev_info_e dev = *it;
					cout << dev.d.mac << endl;
					cout << "torken" << dev.torken <<endl;
					cout << "send on fan/switch!" << endl;
					Protocol p1;

					p1.package_header = 0x55;
					p1.cmd_type = 0x0B;
					p1.cmd = CONTRL_DEV_CMD;                 //控制设备

					memcpy(p1.device_id, it->d.mac, 8);

					p1.torken_len = 20;
					p1.torken = new unsigned char[p1.torken_len];
					memcpy(p1.torken, dev.torken, 20);

					p1.data = new unsigned char[10];
					memset(p1.data, 0, 10);
					memcpy(p1.data, "on", 2); 

					p1.package_tail = 0x55;

					int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
					p1.len_low = len & 0x0ff;
					p1.len_high = len >> 8;

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.fill_buf(buf);        //CRC16校验
					p1.CRC_16(buf);
					p1.fill_buf(buf);
					client._send(buf);

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.clean_data();
					if(RecvPacket(client.sockfd, buf))
					{
						p1.parse_buf(buf);
						if(strcmp((char*)p1.data, "success") == 0)
						{
							cout << "recv ack!" << endl;
							it->status = 1;
						}

					}
					else
					{
						cout << "set fan/switch error!" << endl;
					}

				}
				if(option == "off")
				{

					dev_info_e dev = *it;
					cout << dev.d.mac << endl;
					cout << "torken" << dev.torken <<endl;
					cout << "send off fan/switch!" << endl;
					Protocol p1;

					p1.package_header = 0x55;
					p1.cmd_type = 0x0B;
					p1.cmd = CONTRL_DEV_CMD;                 //控制设备

					memcpy(p1.device_id, it->d.mac, 8);

					p1.torken_len = 20;                       
					p1.torken = new unsigned char[p1.torken_len];
					memcpy(p1.torken, dev.torken, 20);

					p1.data = new unsigned char[10];
					memset(p1.data, 0, 10);
					memcpy(p1.data, "off", 3); 

					p1.package_tail = 0x55;

					int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
					p1.len_low = len & 0x0ff;
					p1.len_high = len >> 8;

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.fill_buf(buf);        //CRC16校验
					p1.CRC_16(buf);
					p1.fill_buf(buf);
					client._send(buf);

					bzero(buf, MAX_PACKAGE_SIZE);
					p1.clean_data();
					if(RecvPacket(client.sockfd, buf))
					{
						p1.parse_buf(buf);
						if(strcmp((char*)p1.data, "success") == 0)
						{
							cout << "recv ack!" << endl;
							it->status = 0;
						}

					}
					else
					{
						cout << "set fan/switch error!" << endl;
					}
				}
			}
			read_sync_unlock(semid_phone_dev_online, count_phone_dev_online);
		}
	}
}



void* thread_msg_input(void* arg)
{
	unsigned char buf[MAX_PACKAGE_SIZE];
	Msgbuf msg;
	while(1)
	{
		bzero(&msg, sizeof(Msgbuf));

		int ret = msgrcv(msgid, (void *)&msg, MAX_MSG_SIZE, MSG_QTTOPHONE, 0); //阻塞接收消息
		if(ret < 0)
			err_fun(__FILE__, __LINE__, "msgrcv", errno);
		cout << "recv a msg from qt phone " << msg.mtext << endl;
		cout << "recv a msg from qt phone " << msg.mtext +10 << endl;

		dev_info_e dev;
		memcpy(dev.d.name, msg.mtext, 10);
		if(!search_dev_from_dev_online(dev))
			continue;
		if(strncmp(msg.mtext, "lamp", 4) == 0)
		{

			if(strcmp(msg.mtext+10, "on") == 0)
			{

				cout << dev.d.mac << endl;
				cout << "torken" << dev.torken <<endl;
				cout << "send on lamp!" << endl;
				Protocol p1;

				p1.package_header = 0x55;
				p1.cmd_type = 0x0B;
				p1.cmd = CONTRL_DEV_CMD;                 //控制设备
				memcpy(p1.device_id, dev.d.mac, 8);


				p1.torken_len = 20;
				p1.torken = new unsigned char[p1.torken_len];
				memcpy(p1.torken, dev.torken, 20);

				p1.data = new unsigned char[10];
				memset(p1.data, 0, 10);
				memcpy(p1.data, "on", 2); 

				p1.package_tail = 0x55;

				int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
				p1.len_low = len & 0x0ff;
				p1.len_high = len >> 8;

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.fill_buf(buf);        //CRC16校验
				p1.CRC_16(buf);
				p1.fill_buf(buf);
				client._send(buf);

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.clean_data();
				if(RecvPacket(client.sockfd, buf))
				{
					p1.parse_buf(buf);
					if(strcmp((char*)p1.data, "success") == 0)
					{
						cout << "recv ack!" << endl;
						//it->status = 1;
					}

				}
				else
				{
					cout << "set lamp error!" << endl;
				}

			}
			if(strcmp(msg.mtext+10, "off") == 0)
			{

				cout << dev.d.mac << endl;
				cout << "torken" << dev.torken <<endl;
				cout << "send off lamp!" << endl;
				Protocol p1;

				p1.package_header = 0x55;
				p1.cmd_type = 0x0B;
				p1.cmd = CONTRL_DEV_CMD;                 //控制设备
				memcpy(p1.device_id, dev.d.mac, 8);

				p1.torken_len = 20;                       //无torken
				p1.torken = new unsigned char[p1.torken_len];
				memcpy(p1.torken, dev.torken, 20);

				p1.data = new unsigned char[10];
				memset(p1.data, 0, 10);
				memcpy(p1.data, "off", 3); 

				p1.package_tail = 0x55;

				int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
				p1.len_low = len & 0x0ff;
				p1.len_high = len >> 8;

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.fill_buf(buf);        //CRC16校验
				p1.CRC_16(buf);
				p1.fill_buf(buf);
				client._send(buf);

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.clean_data();
				if(RecvPacket(client.sockfd, buf))
				{
					p1.parse_buf(buf);
					if(strcmp((char*)p1.data, "success") == 0)
					{
						cout << "recv ack!" << endl;
						//it->status = 0;
					}

				}
				else
				{
					cout << "set lamp error!" << endl;
				}
			}
			if(strcmp(msg.mtext+10, "auto") == 0)
			{

				cout << dev.d.mac << endl;
				cout << "torken" << dev.torken <<endl;
				cout << "auto lamp!" << endl;
				Protocol p1;

				p1.package_header = 0x55;
				p1.cmd_type = 0x0B;
				p1.cmd = CONTRL_DEV_CMD;                 //控制设备
				memcpy(p1.device_id, dev.d.mac, 8);

				p1.torken_len = 20;                       //无torken
				p1.torken = new unsigned char[p1.torken_len];
				memcpy(p1.torken, dev.torken, 20);

				p1.data = new unsigned char[10];
				memset(p1.data, 0, 10);
				memcpy(p1.data, "auto", 4); 

				p1.package_tail = 0x55;

				int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
				p1.len_low = len & 0x0ff;
				p1.len_high = len >> 8;

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.fill_buf(buf);        //CRC16校验
				p1.CRC_16(buf);
				p1.fill_buf(buf);
				client._send(buf);

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.clean_data();
				if(RecvPacket(client.sockfd, buf))
				{
					p1.parse_buf(buf);
					if(strcmp((char*)p1.data, "success") == 0)
					{
						cout << "recv ack!" << endl;
						//it->lamp_auto = 1;
					}

				}
				else
				{
					cout << "set auto error!" << endl;
				}
			}
			if(strcmp(msg.mtext+10, "manual") == 0)
			{

				cout << dev.d.mac << endl;
				cout << "torken" << dev.torken <<endl;
				cout << "send manual lamp!" << endl;
				Protocol p1;

				p1.package_header = 0x55;
				p1.cmd_type = 0x0B;
				p1.cmd = CONTRL_DEV_CMD;                 //控制设备
				memcpy(p1.device_id, dev.d.mac, 8);

				p1.torken_len = 20;                       //无torken
				p1.torken = new unsigned char[p1.torken_len];
				memcpy(p1.torken, dev.torken, 20);

				p1.data = new unsigned char[10];
				memset(p1.data, 0, 10);
				memcpy(p1.data, "manual", 6); 

				p1.package_tail = 0x55;

				int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
				p1.len_low = len & 0x0ff;
				p1.len_high = len >> 8;

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.fill_buf(buf);        //CRC16校验
				p1.CRC_16(buf);
				p1.fill_buf(buf);
				client._send(buf);

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.clean_data();
				if(RecvPacket(client.sockfd, buf))
				{
					p1.parse_buf(buf);
					if(strcmp((char*)p1.data, "success") == 0)
					{
						cout << "recv ack!" << endl;
						//it->lamp_auto = 0;
					}

				}
				else
				{
					cout << "set manual error!" << endl;
				}
			}
		}
		else if(strncmp(msg.mtext, "fan", 3) == 0 || strncmp(msg.mtext, "switch", 6) == 0)
		{
			if(strcmp(msg.mtext+10, "on") == 0)
			{

				cout << dev.d.mac << endl;
				cout << "torken" << dev.torken <<endl;
				cout << "send on fan/switch!" << endl;
				Protocol p1;

				p1.package_header = 0x55;
				p1.cmd_type = 0x0B;
				p1.cmd = CONTRL_DEV_CMD;                 //控制设备

				memcpy(p1.device_id, dev.d.mac, 8);

				p1.torken_len = 20;
				p1.torken = new unsigned char[p1.torken_len];
				memcpy(p1.torken, dev.torken, 20);

				p1.data = new unsigned char[10];
				memset(p1.data, 0, 10);
				memcpy(p1.data, "on", 2); 

				p1.package_tail = 0x55;

				int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
				p1.len_low = len & 0x0ff;
				p1.len_high = len >> 8;

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.fill_buf(buf);        //CRC16校验
				p1.CRC_16(buf);
				p1.fill_buf(buf);
				client._send(buf);

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.clean_data();
				if(RecvPacket(client.sockfd, buf))
				{
					p1.parse_buf(buf);
					if(strcmp((char*)p1.data, "success") == 0)
					{
						cout << "recv ack!" << endl;
						//it->status = 1;
					}

				}
				else
				{
					cout << "set fan/switch error!" << endl;
				}

			}
			else if(strcmp(msg.mtext+10, "off") == 0)
			{

				cout << dev.d.mac << endl;
				cout << "torken" << dev.torken <<endl;
				cout << "send off fan/switch!" << endl;
				Protocol p1;

				p1.package_header = 0x55;
				p1.cmd_type = 0x0B;
				p1.cmd = CONTRL_DEV_CMD;                 //控制设备

				memcpy(p1.device_id, dev.d.mac, 8);

				p1.torken_len = 20;                       
				p1.torken = new unsigned char[p1.torken_len];
				memcpy(p1.torken, dev.torken, 20);

				p1.data = new unsigned char[10];
				memset(p1.data, 0, 10);
				memcpy(p1.data, "off", 3); 

				p1.package_tail = 0x55;

				int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 10; 
				p1.len_low = len & 0x0ff;
				p1.len_high = len >> 8;

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.fill_buf(buf);        //CRC16校验
				p1.CRC_16(buf);
				p1.fill_buf(buf);
				client._send(buf);

				bzero(buf, MAX_PACKAGE_SIZE);
				p1.clean_data();
				if(RecvPacket(client.sockfd, buf))
				{
					p1.parse_buf(buf);
					if(strcmp((char*)p1.data, "success") == 0)
					{
						cout << "recv ack!" << endl;
						//it->status = 0;
					}

				}
				else
				{
					cout << "set fan/switch error!" << endl;
				}
			}
		}
	}
}




int main(void)
{	
	mk_get_msg(&msgid, MSG_FILE_PHONE, 0644, 'a');
	cout << "msgid" << msgid << endl;

	get_sem(&semid_phone_dev_online, SEM_FILE, NSEMS, 'a', 0664);
	for(int i = 0;i<NSEMS; i++)
		init_sem(semid_phone_dev_online, i, 1);

	bzero(&phone1, sizeof(phone1));

	client.set_remoteaddr(SERPORT, SERADDR);
	client._connect();

	phone_read_info(phone1);

	phone_write_server_info();

	phone_login(phone1);

	regist_device();


	//开启接收输入线程
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, thread_input, NULL);

	pthread_create(&thread_id, NULL, thread_msg_input, NULL);


	while(1) //与界面通信，接收消息队列的信息
	{	
		thread_sync_dev_online(NULL);
	}


	return 0;
}	


