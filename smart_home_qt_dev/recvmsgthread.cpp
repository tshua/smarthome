#include "recvmsgthread.h"
#include "protocol.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <QDebug>


RecvMsgThread::RecvMsgThread()
{
    mk_get_msg(&msgid, MSG_FILE_DEV, 0644, 'a');
    qDebug() << msgid;
}

RecvMsgThread::~RecvMsgThread()
{
    //rm_msg(msgid, MSG_FILE_DEV);
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
