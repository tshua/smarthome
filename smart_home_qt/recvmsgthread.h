#ifndef RECVMSGTHREAD_H
#define RECVMSGTHREAD_H


#include <QThread>
#include "common.h"

class RecvMsgThread : public QThread
{
    Q_OBJECT

public:
    RecvMsgThread();
    ~RecvMsgThread();
    Msgbuf* getMsgBuf();
    int getMsgId();

signals:
    void sig_recvDataOk(Msgbuf* msg);

protected:
    void run();




private:
    int msgid;
    Msgbuf msg;

};

#endif // RECVMSGTHREAD_H
