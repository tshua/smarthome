#include "recvmsgthread.h"
#include "protocol.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


RecvMsgThread::RecvMsgThread()
{
    mk_get_msg(&msgid, 0644, 'a');
}

RecvMsgThread::~RecvMsgThread()
{
    rm_msg(msgid);
}


Msgbuf*  RecvMsgThread::getMsgBuf()
{
    return &msg;
}

int RecvMsgThread::getMsgId()
{
    return msgid;
}

void RecvMsgThread::run()
{
    while(true)
    {
        int ret = msgrcv(msgid, (void *)&msg, MAX_MSG_SIZE, MSG_DEVTOQT, 0); //阻塞接收消息
        if(ret < 0)
            err_fun(__FILE__, __LINE__, "msgrcv", errno);
        else
            emit sig_recvDataOk(&msg);
    }
}
