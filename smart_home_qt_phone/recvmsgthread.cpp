#include "recvmsgthread.h"
#include "protocol.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <QDebug>
#include <common.h>

RecvMsgThread::RecvMsgThread()
{
    mk_get_msg(&msgid, MSG_FILE_PHONE, 0644, 'a');
    qDebug()<< msgid;
}

RecvMsgThread::~RecvMsgThread()
{
    //rm_msg(msgid, MSG_FILE_PHONE);
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
        int ret = msgrcv(msgid, (void *)&msg, MAX_MSG_SIZE, MSG_PHONETOQT, 0); //阻塞接收消息
        if(ret < 0)
            err_fun(__FILE__, __LINE__, "msgrcv", errno);
        else
        {
            Msgbuf* pmsg = new Msgbuf(msg);//默认构造函数
            emit sig_recvDataOk(pmsg);
        }
    }
}
