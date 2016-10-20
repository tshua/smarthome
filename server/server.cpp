#include "../header/protocol.h"
#include "../header/sock_client.h"
#include "../header/sock_server.h"
#include "../header/socket_route.h"
#include "../header/protocol.h"
#include "../header/common.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <list>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>


#define MT_REGIST "./files/MT_REGIST"
#define DEV_REGIST "./files/DEV_REGIST"


#define SEM_FILE1 "./files/semfile1"
#define SEM_FILE2 "./files/semfile2"
#define SEM_FILE3 "./files/semfile3"
#define SEM_FILE4 "./files/semfile4"

//struct phone_info_e
//{
//	phone_info p;
//	unsigned char torken[20];
//};
//
//struct dev_info_e
//{
//	dev_info d;
//	unsigned char torken[20];
//	int status; //0 关  1 开
//	int lamp_auto; //只和灯有关 0 手动 1自动
//	int sockfd;
//};


list<phone_info_e> phone_online;
list<dev_info_e> dev_online;

int temprature = 0;
int light1 = 0;
int light2 = 0;

int semid_devfile;
int semid_mtfile;
int semid_devonline;
int semid_phoneonline;
int msgid;


int count_devfile = 0;
int count_mtfile = 0;
int count_devonline = 0;
int count_phoneonline = 0;

//初始化信号量
static int init_sem()
{
	/*get_sem(&semid, SEM_FILE, NSEMS, 'a', 0664);*/
	get_sem(&semid_devfile, SEM_FILE1, NSEMS, 'a', 0664); //2个双态信号量, 0代表count可操作  1代表可以写  设备文件读写同步使用
	for(int i=0; i<NSEMS; i++) init_sem(semid_devfile, i, 1);

	get_sem(&semid_mtfile, SEM_FILE2, NSEMS, 'a', 0664); //APP文件读写同步使用
	for(int i=0; i<NSEMS; i++) init_sem(semid_mtfile, i, 1);

	get_sem(&semid_devonline, SEM_FILE3, NSEMS, 'a', 0664); // 在线设备列表读写同步使用
	for(int i=0; i<NSEMS; i++) init_sem(semid_devonline, i, 1);

	get_sem(&semid_phoneonline, SEM_FILE4, NSEMS, 'a', 0664); // 在线手机设备读写同步使用
	for(int i=0; i<NSEMS; i++) init_sem(semid_phoneonline, i, 1);

	return 1;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//读操作加锁
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int write_sync_lock(int semid)
{
	sem_p(semid, 1, 1);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//读操作解锁
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int write_sync_unlock(int semid)
{
	sem_v(semid, 1, 1);
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//写操作加锁
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int read_sync_lock(int semid, int& count)
{
	sem_p(semid, 0, 1);//count 可操作
	if(count == 0) //阻止写进程
		sem_p(semid, 1, 1);
	count++;
	sem_v(semid, 0, 1);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//写操作解锁
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int read_sync_unlock(int semid, int& count)
int read_sync_unlock(int semid, int& count)
{
	sem_p(semid, 0, 1);
	count--;
	if(count == 0)
		sem_v(semid, 1, 1); //设置可写
	sem_v(semid, 0, 1);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：捕获退出信号，释放资源
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void signal_fun(int signo) //信号捕获函数
{
	if(SIGINT == signo)
	{

		del_sem(semid_devfile, NSEMS, SEM_FILE1); //删除信号量

		del_sem(semid_mtfile, NSEMS, SEM_FILE2); 

		del_sem(semid_devonline, NSEMS, SEM_FILE3);

		del_sem(semid_phoneonline, NSEMS, SEM_FILE4);

		rm_msg(msgid, MSG_FILE_SERVER); //删除消息队列

		
		mk_get_msg(&msgid, MSG_FILE_DEV, 0644, 'a'); //释放设备的消息队列 ,因为有多个设备，所以在服务器这边释放设备的消息队列
		rm_msg(msgid, MSG_FILE_DEV);

		exit(0);

	}
	else if(SIGPIPE == signo)
	{
		cout << "sigpipe" << endl;
	}
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：把缓冲区内容放到消息队列
//参数：buf 缓冲区指针 size 缓冲区大小
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int add_msg(unsigned char* buf, int size)
{
	Msgbuf msgbuf;
	bzero(&msgbuf, sizeof(Msgbuf));

	memcpy(msgbuf.mtext, buf, size);

	msgbuf.mtype = MSG_SERVERTOQT; //设置发送消息的类型           

	cout << "msg" << buf << " "<<  buf+10 << endl;
	int ret = msgsnd(msgid, (void *)&msgbuf, size, 0); //阻塞发送消息

	if(ret < 0)
		return -1;
	return 1;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：向qt界面发送已经注册的设备和手机信息
//      用于界面显示
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void send_regist_msg()
{
	unsigned char msg_data[sizeof(phone_info) + 10] = {0};

	read_sync_lock(semid_mtfile, count_mtfile);             //设置读mt文件
	FILE *fp = fopen(MT_REGIST, "r");

	phone_info phone; //读注册的手机信息文件
	while(fscanf(fp, "%s %s %s %s\n", phone.phone_num, phone.phone_name,\
				phone.password, phone.mail) != EOF) 
	{
		{
			bzero(msg_data, sizeof(phone_info) + 10);
			memcpy(msg_data, "phone", 5);
			memcpy(msg_data + 10, &phone, sizeof(phone));

			add_msg(msg_data, sizeof(phone) + 10);
		}
	}

	fclose(fp);
	read_sync_unlock(semid_mtfile, count_mtfile);

	read_sync_lock(semid_devfile, count_devfile);
	fp = fopen(DEV_REGIST, "r");

	dev_info dev; //读已经注册的设备信息文件
	while(fscanf(fp, "%s %s %s\n", dev.mac, dev.type, dev.name) != EOF) 
	{
		bzero(msg_data, sizeof(phone_info) + 10);
		memcpy(msg_data, "dev", 3);
		memcpy(msg_data + 10, &dev, sizeof(dev));

		add_msg(msg_data, sizeof(dev_info) + 10);
	}

	fclose(fp);
	read_sync_unlock(semid_devfile, count_devfile);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：把设备从在线设备列表中移除
//参数：clnfd 对应设备的socket fd
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int rmdev_from_list(int clnfd)
{
	int rm_ok = 0;
	write_sync_lock(semid_devonline);

	list<dev_info_e>::iterator it = dev_online.begin();
	for(; it != dev_online.end(); it++)
	{
		if(clnfd == it->sockfd)
		{
			dev_online.erase(it);
			rm_ok = 1;
			break;
		}
	}

	write_sync_unlock(semid_devonline);

	return rm_ok;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：把手机从在线设备列表中移除
//参数：clnfd 对应设备的socket fd
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int rmphone_from_list(int clnfd)//从在线设备列表中移除该设备
{
	int rm_ok = 0; 
	write_sync_lock(semid_phoneonline);

	list<phone_info_e>::iterator it = phone_online.begin();
	for(; it != phone_online.end(); it++)
	{
		if(clnfd == it->p.is_regist)
		{
			phone_online.erase(it);
			rm_ok = 1; 
			break;
		}
	}
	write_sync_unlock(semid_phoneonline);

	return rm_ok;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：查找数据库（文件）
//参数：phone_info 返回查找到手机的信息
//返回值： 1 查找成功 0查找失败
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int search_phone(phone_info& p)
{
	read_sync_lock(semid_mtfile, count_mtfile); 		//设置读mt文件
	FILE *fp = fopen(MT_REGIST, "r");

	int find_ok = 0;
	phone_info phone;
	while(fscanf(fp, "%s %s %s %s\n", phone.phone_num, phone.phone_name,\
				phone.password, phone.mail) != EOF)
	{
		if(strcmp(p.phone_name, phone.phone_name)==0 && strcmp(p.password, phone.password) == 0)
		{
			memcpy(p.phone_num, phone.phone_num, 20);
			memcpy(p.mail, phone.mail, 20);
			find_ok = 1;
			break;
		}
	}

	fclose(fp);
	read_sync_unlock(semid_mtfile, count_mtfile);
	return find_ok;

}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：查找数据库（文件）
//参数：dev_info 返回查找到设备的信息
//返回值： 1 查找成功 0查找失败
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int search_dev(dev_info& d) //from file
{
	read_sync_lock(semid_devfile, count_devfile);
	FILE *fp = fopen(DEV_REGIST, "r");

	int find_ok = 0;
	dev_info dev;
	while(fscanf(fp, "%s %s %s\n", dev.mac, dev.type, dev.name) != EOF)
	{
		if(strcmp(dev.mac, d.mac)==0)
		{
			memcpy(d.type, dev.type, 10);
			memcpy(d.name, dev.name, 10);
			find_ok = 1;
			break;
		}
	}

	fclose(fp);
	read_sync_unlock(semid_devfile, count_devfile);
	return find_ok;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：查找在线设备列表
//参数：mac 设备的mac地址
//返回值： 返回对应设备的描述符
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int search_dev_from_dev_online(unsigned char *mac) //返回对应的套接字描述符
{

	read_sync_lock(semid_devonline, count_devonline);

	list<dev_info_e>::iterator it = dev_online.begin();
	for(; it != dev_online.end(); it++)
	{
		cout << "step in" << endl;
		if(strncmp((char*)mac, it->d.mac, 8) == 0)
		{
			read_sync_unlock(semid_devonline, count_devonline);
			return it->sockfd;
		}
	}

	read_sync_unlock(semid_devonline, count_devonline);
	return -1;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：查找在线手机列表
//参数： phone_info_e phone_e 手机号码，返回torken
//返回值： 1 查找成功 0查找失败
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int search_phone_from_phone_online(phone_info_e& phone_e) 
{
	read_sync_lock(semid_phoneonline, count_phoneonline);

	int find_ok = 0;
	list<phone_info_e>::iterator it = phone_online.begin();
	for(; it != phone_online.end(); it++)
	{
		if(strcmp((char*)phone_e.p.phone_num, it->p.phone_num) == 0)
		{
			memcpy(phone_e.torken, it->torken, 20);
			find_ok = 1;
		}
	}

	read_sync_unlock(semid_phoneonline, count_phoneonline);

	return find_ok;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：生成torken
//参数： 返回生成的torken
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void make_torken(unsigned char* torken)
{
	/* initialize random seed: */
	srand ( time(NULL)  );

	/* generate secret number: */
	unsigned char iSecret;
	for(int i = 0;i<20;i++)
	{
		iSecret = rand() % 74 + 48;  //0~z
		torken[i] = iSecret;
	}
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：插入新的手机信息到在线手机列表
//参数： phone_info_e 手机信息
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void insert_online_mt(phone_info_e phone_e)
{
	write_sync_lock(semid_phoneonline);
	phone_online.push_back(phone_e);
	write_sync_unlock(semid_phoneonline);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：插入新的设备信息到在线设备列表
//参数： dev_info_e 设备信息
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void insert_online_dev(dev_info_e dev_e)
{
	write_sync_lock(semid_devonline);
	dev_online.push_back(dev_e);
	write_sync_unlock(semid_devonline);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：写入新的手机信息到数据库（文件）
//参数： phone_info 手机信息
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void write_regist_info_to_file(phone_info& phone)
{
	if(!search_phone(phone))
	{
		write_sync_lock(semid_mtfile);
		FILE *fp = fopen(MT_REGIST, "a");
		fprintf(fp, "%s %s %s %s\n", phone.phone_num, phone.phone_name,\
				phone.password, phone.mail);
		fclose(fp);
		write_sync_unlock(semid_mtfile);
	}
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：写入新的设备信息到数据库（文件）
//参数： dev_info 设备信息
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void write_dev_info_to_file(dev_info& dev)
{
	if(!search_dev(dev))
	{
		write_sync_lock(semid_devfile);
		FILE *fp = fopen(DEV_REGIST, "a");
		fprintf(fp, "%s %s %s\n", dev.mac, dev.type, dev.name); 
		fclose(fp);
		write_sync_unlock(semid_devfile);
	}
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能：更改设备状态
//参数： mac 设备地址  status 设备状态
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void set_lamp_status(unsigned char* mac, int status)
{
	write_sync_lock(semid_devonline);

	list<dev_info_e>::iterator it;

	for ( it = dev_online.begin() ; it != dev_online.end(); it++  )
	{
		if(strncmp( (char*)it->d.mac, (char*)mac, 8) == 0)
		{
			it->status = status;
		}
	}
	write_sync_unlock(semid_devonline);

}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能： 心跳检测、接收手机、设备的注册、登录等信息
// 	 转发手机端给设备的控制信息
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void* thread_recv(void *arg)
{
	int clnfd = *((int*)arg);
	SockClient client;
	client.sockfd = clnfd;

	unsigned char buf[MAX_PACKAGE_SIZE];
	unsigned char msg_data[20] = {0};
	Protocol p1;
	Protocol p;
	int link_data_count = 0; //心跳检测计数，到达多长时间发送一次心跳检测
	while(1)
	{
		bzero(buf, MAX_PACKAGE_SIZE);
		int isRecved = WaitData(clnfd, 100000);  //延时100ms
		link_data_count ++;
		//cout << link_data_count << endl;
		if(isRecved)
		{
			if(RecvPacket(clnfd, buf))
			{
				link_data_count = 0;
				p.clean_data();
				p1.clean_data();
				p.parse_buf(buf);
				int len;

				switch(p.cmd)
				{

					case LOGIN_CMD: //手机登录信息
						phone_info_e phone_e;
						memcpy(phone_e.p.phone_name, p.data, 10);
						memcpy(phone_e.p.password, p.data + 10, 20);
						if(search_phone(phone_e.p)) //查找成功
						{
							make_torken(phone_e.torken);
							phone_e.p.is_regist = clnfd; //这一个标志位在服务器没有意义，用来记录手机的socket fd
							insert_online_mt(phone_e);

							bzero(buf, MAX_PACKAGE_SIZE);


							p1.package_header = 0x55;
							p1.cmd_type = 0x0A;
							p1.cmd = RES;                     //登录成功应答

							p1.torken_len = 20;                       //torken
							p1.torken = new unsigned char[p1.torken_len];
							memcpy(p1.torken, phone_e.torken, 20);


							p1.data = new unsigned char[1]; 
							p1.data[0] = -1; 

							p1.package_tail = 0x55;

							len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 1;
							p1.len_low = len & 0x0ff;
							p1.len_high = len >> 8;

							bzero(buf, MAX_PACKAGE_SIZE);
							p1.fill_buf(buf);        //CRC16校验
							p1.CRC_16(buf);
							p1.fill_buf(buf);

							client._send(buf);

						}
						else //登录失败
						{


							p1.package_header = 0x55;
							p1.cmd_type = 0x0A;
							p1.cmd = RES;                     //应答

							p1.torken_len = 1;                       //无torken
							p1.torken = new unsigned char[p1.torken_len];
							p1.torken[0] = -1; 

							p1.data = new unsigned char[1]; 
							p1.data[0] = -1;

							p1.package_tail = 0x55;

							len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 1;
							p1.len_low = len & 0x0ff;
							p1.len_high = len >> 8;

							bzero(buf, MAX_PACKAGE_SIZE);
							p1.fill_buf(buf);        //CRC16校验
							p1.CRC_16(buf);
							p1.fill_buf(buf);

							client._send(buf);
						}

						break;

					case REGIST_CMD: // 手机注册信息
						bzero(msg_data, 20);
						memcpy(msg_data, "cleandata", 9);
						add_msg(msg_data, 20);

						phone_info phone;
						memcpy(&phone, p.data, sizeof(phone_info) - 1);
						write_regist_info_to_file(phone);

						p1.package_header = 0x55;
						p1.cmd_type = 0x0A;
						p1.cmd = RES;                     //应答

						p1.torken_len = 1;                       //无torken
						p1.torken = new unsigned char[p1.torken_len];
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
						send_regist_msg();

						break;
					case REGIST_DEV_CMD: // 设备注册信息

						bzero(msg_data, 20); //先把显示的内容清空
						memcpy(msg_data, "cleandata", 9);
						add_msg(msg_data, 20);

						dev_info dev;
						memcpy(&dev, p.data, sizeof(dev_info));
						write_dev_info_to_file(dev);

						p1.package_header = 0x55;
						p1.cmd_type = 0x0A;
						p1.cmd = RES;                     //应答

						p1.torken_len = 1;                       //无torken
						p1.torken = new unsigned char[p1.torken_len];
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

						send_regist_msg();
						break;

					case DEV_LOGIN: //设备登录信息
						dev_info_e dev_e;
						memcpy(dev_e.d.mac, p.data, 8);
						dev_e.sockfd = client.sockfd;
						dev_e.status = 0; 	// 默认状态 关闭
						dev_e.lamp_auto = 0; 	//手动
						if(search_dev(dev_e.d)) //查找成功
						{
							make_torken(dev_e.torken);
							insert_online_dev(dev_e);

							bzero(buf, MAX_PACKAGE_SIZE);


							p1.package_header = 0x55;
							p1.cmd_type = 0x0A;
							p1.cmd = RES;                     //登录成功应答

							p1.torken_len = 20;                       //torken
							p1.torken = new unsigned char[p1.torken_len];
							memcpy(p1.torken, dev_e.torken, 20);

							p1.data = new unsigned char[1]; 
							p1.data[0] = -1; 

							p1.package_tail = 0x55;

							len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 1;
							p1.len_low = len & 0x0ff;
							p1.len_high = len >> 8;

							bzero(buf, MAX_PACKAGE_SIZE);
							p1.fill_buf(buf);        //CRC16校验
							p1.CRC_16(buf);
							p1.fill_buf(buf);

							client._send(buf);

							bzero(msg_data, 20);
							memcpy(msg_data, dev_e.d.name, 10);
							memcpy(msg_data + 10, "login", 5);
							add_msg(msg_data, 20);

						}

						break;

					case STATUS_LIGHT: //光强 on/off device_id
						{
							read_sync_lock(semid_devonline, count_devonline);

							list<dev_info_e>::iterator it = dev_online.begin();
							for(; it != dev_online.end(); it++) //发送消息到QT界面
							{
								if(strncmp((char*)p.device_id, it->d.mac, 8) == 0)
								{
									if(strcmp(it->d.name, "lamp1") == 0)
									{
										light1 = atoi((char*)p.data);
										cout << "new light data:"<< light1 << endl;
										bzero(msg_data, 20);
										memcpy(msg_data, it->d.name, 10);
										memcpy(msg_data + 10, p.data, 10);
										add_msg(msg_data, 20);
									}
									else if(strcmp(it->d.name, "lamp2") == 0)
									{
										light2 = atoi((char*)p.data);
										cout << "new light data:"<< light2 << endl;
										bzero(msg_data, 20);
										memcpy(msg_data, it->d.name, 10);
										memcpy(msg_data + 10, p.data, 10);
										add_msg(msg_data, 20);
									}
									break;
								}

							}
							read_sync_unlock(semid_devonline, count_devonline);
						}

						//light = atoi((char*)p.data);
						//cout << "new light data:"<< light << endl;
						if(strcmp((char*)(p.data+10), "on") == 0)
						{
							set_lamp_status(p.device_id, 1);
						}
						else if(strcmp((char*)(p.data+10),"off") == 0)
						{
							set_lamp_status(p.device_id, 0);
						}
						break;
					case LIGHT: //光照数据
						{
							read_sync_lock(semid_devonline, count_devonline);

							list<dev_info_e>::iterator it = dev_online.begin();
							for(; it != dev_online.end(); it++)
							{
								if(strncmp((char*)p.device_id, it->d.mac, 8) == 0)
								{
									if(strcmp(it->d.name, "lamp1") == 0)
									{
										light1 = atoi((char*)p.data);
										cout << "new light data:"<< light1 << endl;

										bzero(msg_data, 20);
										memcpy(msg_data, it->d.name, 10);
										memcpy(msg_data + 10, p.data, 10);
										add_msg(msg_data, 20);

									}
									else if(strcmp(it->d.name, "lamp2") == 0)
									{
										light2 = atoi((char*)p.data);
										cout << "new light data:"<< light2 << endl;

										bzero(msg_data, 20);
										memcpy(msg_data, it->d.name, 10);
										memcpy(msg_data + 10, p.data, 10);
										add_msg(msg_data, 20);

									}
									break;

								}


							}
							read_sync_unlock(semid_devonline, count_devonline);

							//light = atoi((char*)p.data);
							//cout << "new light data:"<< light << endl;
						}
						break;
					case TEMPRATURE:
						{
							temprature = atoi((char*)p.data);
							cout << "new temprature data:" << temprature << endl;

							bzero(msg_data, 20);
							memcpy(msg_data, "fan1", 4);
							memcpy(msg_data + 10, p.data, 10);
							add_msg(msg_data, 20);
						}
						break;
					case CONTRL_DEV_CMD: //根据mac找到设备，直接把控制命令转发
						{
							SockClient sock_tmp;
							int sfd = search_dev_from_dev_online(p.device_id);
							if(sfd >= 0)
							{
								sock_tmp.sockfd = sfd;
							}
							sock_tmp._send(buf); //转发命令


							//把服务器中该设备的状态更改
							write_sync_lock(semid_devonline);
							list<dev_info_e>::iterator it = dev_online.begin();
							unsigned char msg_data[20] = {0};
							for(; it != dev_online.end(); it++)
							{
								if(strncmp((char*)p.device_id, it->d.mac, 8) == 0)
								{

									if(strcmp((char*)p.data, "off") == 0)
									{
										it->status = 0;

										bzero(msg_data, 20);
										memcpy(msg_data, it->d.name, 10);
										memcpy(msg_data + 10, "off", 3);
										add_msg(msg_data, 20);

									}
									if(strcmp((char*)p.data, "on") == 0)
									{
										it->status = 1;

										bzero(msg_data, 20);
										memcpy(msg_data, it->d.name, 10);
										memcpy(msg_data + 10, "on", 2);
										add_msg(msg_data, 20);
									}
									if(strcmp((char*)(p.data), "auto") == 0)
									{
										it->lamp_auto = 1;
										bzero(msg_data, 20);
										memcpy(msg_data, it->d.name, 10);
										memcpy(msg_data + 10, "auto", 4);
										add_msg(msg_data, 20);

									}
									if(strcmp((char*)p.data, "manual") == 0)
									{
										it->lamp_auto = 0;
										bzero(msg_data, 20);
										memcpy(msg_data, it->d.name, 10);
										memcpy(msg_data + 10, "manual", 6);
										add_msg(msg_data, 20);
									}
								}
							}
							write_sync_unlock(semid_devonline);

							//应答，设置成功
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
						}
						break;
					case GET_DEV_SENCEINFO_CMD:
						{

							phone_info_e myphone;
							memcpy(myphone.p.phone_num, p.data, 20);
							search_phone_from_phone_online(myphone);

							p1.package_header = 0x55;
							p1.cmd_type = 0x0B;
							p1.cmd = RES; 	                    //应答

							p1.torken_len = 20; 
							p1.torken = new unsigned char[p1.torken_len];
							memcpy(p1.torken, phone_e.torken, 20);


							p1.extend_info[0] = dev_online.size();

							p1.data = new unsigned char[sizeof(dev_info_e)*dev_online.size() +30 ]; //后30个字节包括光照1、光照2、温度

							unsigned char* pos = p1.data;
							//dev_info_e tmp;
							list<dev_info_e>::iterator dev_it = dev_online.begin();
							while(dev_it != dev_online.end())
							{
								//tmp = *dev_it;
								memcpy(pos, &(*dev_it), sizeof(dev_info_e));
								pos += sizeof(dev_info_e);
								dev_it++;
							}

							sprintf((char*)pos, "%d", light1);
							sprintf((char*)pos+10, "%d", light2);
							sprintf((char*)pos+20, "%d", temprature);

							p1.package_tail = 0x55;

							len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + sizeof(dev_info_e)*dev_online.size() + 30; 
							p1.len_low = len & 0x0ff;
							p1.len_high = len >> 8;

							bzero(buf, MAX_PACKAGE_SIZE);
							p1.fill_buf(buf);        //CRC16校验
							p1.CRC_16(buf);
							p1.fill_buf(buf);
							client._send(buf);
						}
						break;
					default:
						break;

				}
			}

		}
		if(link_data_count == 100) //100 * 100ms  = 10s 内没有数据到达，发送心跳包，维持tcp连接
		{
			p1.package_header = 0x55;
			p1.cmd_type = 0x0A;
			p1.cmd = HEARTBEAT_CMD;                     //登录成功应答

			p1.torken_len = 1;                           //无torken
			p1.torken = new unsigned char[p1.torken_len];
			p1.torken[0] = -1;

			p1.data = new unsigned char[1]; 
			p1.data[0] = -1;  

			p1.package_tail = 0x55;

			int len = PACKAGE_LEN_EXCEPT_DATA + p1.torken_len + 1; 
			p1.len_low = len & 0x0ff;
			p1.len_high = len >> 8;

			bzero(buf, MAX_PACKAGE_SIZE);
			p1.fill_buf(buf);        	//CRC16校验
			p1.CRC_16(buf);
			p1.fill_buf(buf);

			client._send(buf);


			bzero(buf, MAX_PACKAGE_SIZE);
			p1.clean_data();

			//根据sockfd查找设备名称，给qt界面发送离线或者在线消息
			bzero(msg_data, 20);
			read_sync_lock(semid_devonline, count_devonline);
			list<dev_info_e>::iterator it = dev_online.begin();
			for(; it != dev_online.end(); it++)
			{
				if(it->sockfd == clnfd)
				{
					memcpy(msg_data, it->d.name, 10); //获取名字就可以
					break;
				}


			}
			read_sync_unlock(semid_devonline, count_devonline);

			if(RecvPacket(client.sockfd, buf))
			{
				p1.parse_buf(buf);
				if(strcmp((char*)p1.data, "success") == 0)
				{
					cout << "recv xintiao  ack!" << endl;
					memcpy(msg_data + 10, "login", 5);
					add_msg(msg_data, 20); 

				}


			}
			else
			{
				cout << "not recv xin tiao ack!" << endl;
				memcpy(msg_data + 10, "logout", 6);
				add_msg(msg_data, 20);

				rmdev_from_list(clnfd);
				rmphone_from_list(clnfd);//从在线设备列表中移除该设备

				break; //心跳检测失败，说明设备已经下线，发送logout消息给qt程序，跳出循环，终止线程


			}
			link_data_count = 0;
		}
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

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//功能： 接收消息队列输入线程，
//       发送控制命令到设备
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void *thread_input(void *arg){
	unsigned char buf[MAX_PACKAGE_SIZE];
	unsigned char msg_data[20] = {0};

	SockClient client;
	char c;
	string option;
	int i;
	while(1)
	{
		cin >> c;
		if(c == 'p')
		{
			i = 0;

			read_sync_lock(semid_devonline, count_devonline);

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

			client.sockfd = it->sockfd;
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
							bzero(msg_data, 20);
							memcpy(msg_data, it->d.name, 10);
							memcpy(msg_data + 10, "on", 2);
							add_msg(msg_data, 20);
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

					p1.clean_data();
					bzero(buf, MAX_PACKAGE_SIZE);
					if(RecvPacket(client.sockfd, buf))
					{
						p1.parse_buf(buf);
						if(strcmp((char*)p1.data, "success") == 0)
						{
							cout << "recv ack!" << endl;
							it->status = 0;
							bzero(msg_data, 20);
							memcpy(msg_data, it->d.name, 10);
							memcpy(msg_data + 10, "off", 3);
							add_msg(msg_data, 20);
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

					p1.clean_data();
					bzero(buf, MAX_PACKAGE_SIZE);
					if(RecvPacket(client.sockfd, buf))
					{
						p1.parse_buf(buf);
						if(strcmp((char*)p1.data, "success") == 0)
						{
							cout << "recv ack!" << endl;
							it->lamp_auto = 1;
							bzero(msg_data, 20);
							memcpy(msg_data, it->d.name, 10);
							memcpy(msg_data + 10, "auto", 4);
							add_msg(msg_data, 20);
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
							bzero(msg_data, 20);
							memcpy(msg_data, it->d.name, 10);
							memcpy(msg_data + 10, "manual", 6);
							add_msg(msg_data, 20);
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

							bzero(msg_data, 20);
							memcpy(msg_data, it->d.name, 10);
							memcpy(msg_data + 10, "on", 2);
							add_msg(msg_data, 20);
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
							bzero(msg_data, 20);
							memcpy(msg_data, it->d.name, 10);
							memcpy(msg_data + 10, "off", 3);
							add_msg(msg_data, 20);
						}

					}
					else
					{
						cout << "set fan/switch error!" << endl;
					}
				}
			}
			read_sync_unlock(semid_devonline, count_devonline);
		}
	}
}


int search_dev_from_dev_online(dev_info_e& dev_e) //根据名称查询所有的设备信息
{

	read_sync_lock(semid_devonline, count_devonline);

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


	read_sync_unlock(semid_devonline, count_devonline);
	return find_ok;
}

void* thread_msg_input(void* arg)//中控界面发给中控程序的信息
{
	unsigned char buf[MAX_PACKAGE_SIZE];
	unsigned char msg_data[20] = {0};
	SockClient client;
	Msgbuf msg;
	while(1)
	{
		cout << 4 << endl;
		bzero(&msg, sizeof(Msgbuf));

		cout << 5 << endl;
		int ret = msgrcv(msgid, (void *)&msg, MAX_MSG_SIZE, MSG_QTTOSERVER, 0); //阻塞接收消息

		cout << 6 << endl;
		if(ret < 0)
			err_fun(__FILE__, __LINE__, "msgrcv", errno);
		cout << "recv a msg from qt phone " << msg.mtext << endl;
		cout << "recv a msg from qt phone " << msg.mtext +10 << endl;

		dev_info_e dev;
		memcpy(dev.d.name, msg.mtext, 10);
		if(!search_dev_from_dev_online(dev))
			continue;
		client.sockfd = dev.sockfd;
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

						//把服务器中该设备的状态更改
						write_sync_lock(semid_devonline);
						list<dev_info_e>::iterator it = dev_online.begin();
						for(; it != dev_online.end(); it++)
						{
							if(strncmp((char*)dev.d.mac, it->d.mac, 8) == 0)
							{
								it->status = 1;
								bzero(msg_data, 20);
								memcpy(msg_data, it->d.name, 10);
								memcpy(msg_data + 10, "on", 2);
								add_msg(msg_data, 20);
							}
						}
						write_sync_unlock(semid_devonline);
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

						//把服务器中该设备的状态更改
						write_sync_lock(semid_devonline);
						list<dev_info_e>::iterator it = dev_online.begin();
						for(; it != dev_online.end(); it++)
						{
							if(strncmp((char*)dev.d.mac, it->d.mac, 8) == 0)
							{
								it->status = 0;
								bzero(msg_data, 20);
								memcpy(msg_data, it->d.name, 10);
								memcpy(msg_data + 10, "off", 3);
								add_msg(msg_data, 20);
							}
						}
						write_sync_unlock(semid_devonline);
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
				cout << "a" << endl;
				if(RecvPacket(client.sockfd, buf))
				{
					cout << "b" << endl;
					p1.parse_buf(buf);
					if(strcmp((char*)p1.data, "success") == 0)
					{
						cout << "recv ack!" << endl;

						//把服务器中该设备的状态更改
						write_sync_lock(semid_devonline);
						list<dev_info_e>::iterator it = dev_online.begin();
						for(; it != dev_online.end(); it++)
						{
							if(strncmp((char*)dev.d.mac, it->d.mac, 8) == 0)
							{
								it->lamp_auto = 1;
								bzero(msg_data, 20);
								memcpy(msg_data, it->d.name, 10);
								memcpy(msg_data + 10, "auto", 4);
								add_msg(msg_data, 20);
							}
						}
						write_sync_unlock(semid_devonline);

						//it->lamp_auto = 1;
					}

				}
				else
				{
					cout << "set auto error!" << endl;
				}
				cout << 1 << endl;
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

						//把服务器中该设备的状态更改
						write_sync_lock(semid_devonline);
						list<dev_info_e>::iterator it = dev_online.begin();
						for(; it != dev_online.end(); it++)
						{
							if(strncmp((char*)dev.d.mac, it->d.mac, 8) == 0)
							{
								it->lamp_auto = 0;
								bzero(msg_data, 20);
								memcpy(msg_data, it->d.name, 10);
								memcpy(msg_data + 10, "manual", 6);
								add_msg(msg_data, 20);
							}
						}
						write_sync_unlock(semid_devonline);

						//it->lamp_auto = 0;
					}

				}
				else
				{
					cout << "set manual error!" << endl;
				}
			}
			cout << 2 << endl;
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

						//把服务器中该设备的状态更改
						write_sync_lock(semid_devonline);
						list<dev_info_e>::iterator it = dev_online.begin();
						for(; it != dev_online.end(); it++)
						{
							if(strncmp((char*)dev.d.mac, it->d.mac, 8) == 0)
							{
								it->status = 1;
								bzero(msg_data, 20);
								memcpy(msg_data, it->d.name, 10);
								memcpy(msg_data + 10, "on", 2);
								add_msg(msg_data, 20);
							}
						}
						write_sync_unlock(semid_devonline);
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
				cout << "a" << endl;
				if(RecvPacket(client.sockfd, buf))
				{
					cout << "b" << endl;
					p1.parse_buf(buf);
					if(strcmp((char*)p1.data, "success") == 0)
					{
						cout << "recv ack!" << endl;

						//把服务器中该设备的状态更改
						write_sync_lock(semid_devonline);
						list<dev_info_e>::iterator it = dev_online.begin();
						for(; it != dev_online.end(); it++)
						{
							if(strncmp((char*)dev.d.mac, it->d.mac, 8) == 0)
							{
								it->status = 0;
								bzero(msg_data, 20);
								memcpy(msg_data, it->d.name, 10);
								memcpy(msg_data + 10, "off", 3);
								add_msg(msg_data, 20);
							}
						}
						write_sync_unlock(semid_devonline);
						//it->status = 0;
					}

				}
				else
				{
					cout << "set fan/switch error!" << endl;
				}
			}
		}

		cout << 3 << endl;
	}
}	

int main()
{
	signal(SIGINT, signal_fun);
	signal(SIGPIPE, signal_fun);
	mk_get_msg(&msgid, MSG_FILE_SERVER, 0644, 'a');
	cout << "msgid:" << msgid << endl;
	SockServer s(SERPORT);
	Protocol p;
	unsigned char buf[MAX_PACKAGE_SIZE] = {0};

	init_sem(); //获取并初始化信号量

	s._bind();
	s._listen();

	output_select();

	pthread_t thread_id;
	pthread_create(&thread_id, NULL, thread_input, NULL);//输入控制数据线程
	
	send_regist_msg();//向qt界面发送已经注册的设备和手机信息

	//打开接收消息队列信息的线程
	pthread_create(&thread_id, NULL, thread_msg_input, NULL);


	while(1)
	{
		int clnfd = s._accept();

		//打开处理线程
		pthread_create(&thread_id, NULL, thread_recv, &clnfd);
	}
}

