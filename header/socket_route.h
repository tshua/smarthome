#ifndef H_SOCKET_ROUTE_H
#define H_SOCKET_ROUTE_H



// 在规定的时间内等待数据到达
// 输入：dwTime = 需要等待的时间（微秒）
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int WaitData(int hSocket, unsigned int dwTime);

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 接收规定字节的数据，如果缓冲区中的数据不够则等待
// 返回：eax = TRUE，连接中断或发生错误
//	 FALSE，成功
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int RecvData(int hSocket, unsigned char *lpData, unsigned int  dwSize);


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 接收一个符合规范的数据包
// 返回： TRUE （失败）
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool RecvPacket(int hSocket, unsigned char *lpBuffer);


#endif
