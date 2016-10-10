
#include "../header/socket_route.h"
#include <time.h>
#include "../header/protocol.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <string.h>



//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 在规定的时间内等待数据到达
// 输入：dwTime = 需要等待的时间（微秒）
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int WaitData(int hSocket,unsigned int dwTime)
{
	int fdCount;
 	fd_set stFdSet;
	struct timeval stTimeval;
	FD_ZERO(&stFdSet);
	//stFdSet.fd_array[0]=hSocket;
	FD_SET(hSocket, &stFdSet);
	stTimeval.tv_sec=0;
	stTimeval.tv_usec=dwTime;
	fdCount=select(hSocket + 1,&stFdSet,NULL,NULL,&stTimeval);
	return fdCount; //0时间到达   >0接收到数据
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 接收规定字节的数据，如果缓冲区中的数据不够则等待
// 返回：eax = false，连接中断或发生错误
//	 true 成功
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int RecvData(int hSocket,unsigned char *lpData,unsigned int dwSize)
{
	int dwStartTime,dwNow,iErrFlag = true;
	int dwRemain,iSocketCount,iRecvCount;
	unsigned char *pCh;
	pCh=lpData;
	dwRemain=dwSize;
	dwStartTime = time(NULL);

	//********************************************************************
	while(1)
	{
		dwNow = time(NULL);//查看是否超时
		if (dwNow-dwStartTime>2) //2s内没有接收完数据，返回出错
		{
			iErrFlag = false;
			break;
		}
		//********************************************************************
		iSocketCount=WaitData(hSocket,100*1000);
		if(iSocketCount == -1)	//等待数据100ms
		{
			iErrFlag = false;
			break;
		}
		if (iSocketCount==0)  //没有接收到数据
			continue;
		iRecvCount=recv(hSocket,pCh,dwRemain,0); //有数据到达
		if(iRecvCount==-1 || iRecvCount==0)
		{
			iErrFlag = false;
			break;
		}
		if(iRecvCount<dwRemain)
		{
			pCh+=iRecvCount;
			dwRemain-=iRecvCount;
		}
		else
			break;
	}
	return (iErrFlag);
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 接收一个符合规范的数据包
// 返回： TRUE (成功) false（失败）
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool RecvPacket(int hSocket,unsigned char *lpBuffer)
{
	int dwReturn = false,iRemain, data_len;

	unsigned char *lpContent;

	// 接收数据包头部并检测数据是否正常
	if(RecvData(hSocket,lpBuffer, 4)) //先接收长度
	{	
		Protocol p;

		data_len = lpBuffer[2] + lpBuffer[3]*256;

		// 接收余下的数据
		iRemain = data_len - 4;
		if (iRemain>0)
		{
			lpContent = lpBuffer + 4;
			if (RecvData(hSocket,lpContent,iRemain))
			{
				dwReturn = true;
				data_len = p.transferDecode(lpBuffer, data_len);
				lpBuffer[1] = data_len & 0x0ff;
				lpBuffer[2] = data_len >> 8;
			}
		}		
		else
			dwReturn = false;
	}

	return dwReturn;
}
