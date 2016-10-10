#ifndef PROTOCOL_H
#define PROTOCOL_H


#define SERPORT 5000
#define SERADDR	"192.168.1.103"

#define MAX_PACKAGE_SIZE 1024
#define MAX_MSG_SIZE 1024
#define PACKAGE_LEN_EXCEPT_DATA 24
#define CRC_SEED   0xFFFF   // 该位称为预置值，使用人工算法（长除法）时 需要将除数多项式先与该与职位 异或 ，才能得到最后的除数多项式
#define POLY16 0x1021       // 该位为简式书写 实际为0x11021

#define CMDTYPE_LINKOPRATION    0x0A //链路
#define CMDTYPE_CONTROLDEVICE 0x0B //操作设备

/* 具体命令字 */
#define LOGIN_CMD               0x01 //登录
#define REGIST_CMD              0x02 //注册
#define CONTRL_DEV_CMD          0x03//控制设备
#define DELETE_DEV_CMD          0x04//删除设备
#define GET_DEVLIST_CMD         0x05//请求设备列表
#define HEARTBEAT_CMD           0x06//心跳检测命令
#define REGIST_DEV_CMD          0x07//注册设备信息
#define OFFLINE_CMD             0x08//MT下线
#define GET_DEV_SENCEINFO_CMD   0x09 //同步dev——online
#define RES 			0x14 //应答包
#define DEV_LOGIN 		0x15 //设备登录
#define LIGHT 			0x16 //光照信息
#define STATUS_LIGHT 		0x17 //关照和灯的状态
#define TEMPRATURE 		0x18 //温度



#define DOCRC16         0x10
#define NOCRC16         0x11
#define ENCRYPT         0x12
#define NOENCRYPT       0x13

#define MSG_DEVTOQT         1
#define MSG_SERVERTOQT      2
#define MSG_QTTOSERVER      3


class Protocol
{
public:
    Protocol();
    Protocol(Protocol &p);
    ~Protocol();
	
    void clean_data();
    int CRC_16(unsigned char* buf);                      //计算CRC，并填充到package中
    int fill_buf(unsigned char* buf);  //填充buf,返回长度
    int parse_buf(unsigned char* buf); //解析buf，将buf中数据转换成数据包的形式
    int transferEncode(unsigned char* buf);   //添加转义字符
    int transferDecode(unsigned char* buf, int length);   //去除转义字符

public:
    unsigned char package_header;   //包头
    unsigned char len_low;          //长度低位
    unsigned char len_high;         //长度高位
    unsigned char cmd_type;         //命令类别
    unsigned char cmd;              //命令字
    unsigned char cmd_order_low;    //命令序号低位
    unsigned char cmd_order_high;   //命令序号高位
    unsigned char have_extend;      //是否有扩展位
    unsigned char extend_info[2];   //扩展信息
    unsigned char status;           //状态
    unsigned char device_id[8];     //设备ID
    unsigned char torken_len;       //令牌长度
    unsigned char *torken;          //通信令牌
    unsigned char *data;            //数据区
    unsigned char have_CRC16;       //是否有CRC
    unsigned char CRC_low;          //CRC低位
    unsigned char CRC_high;         //CRC高位
    unsigned char package_tail;     //包尾
};


struct phone_info
{
	char phone_num[20];
	char phone_name[10];
	char password[20];
	char mail[20];
	char is_regist;

};

struct dev_info
{
	char mac[9];
	char type[10];
	char name[10];

};

struct phone_info_e
{
	phone_info p;
	unsigned char torken[20];

};

struct dev_info_e
{
	dev_info d;
	unsigned char torken[20];
	int status; //0 关  1 开
	int lamp_auto; //只和灯有关 0 手动 1自动
	int sockfd;

};


#endif // PROTOCOL_H
