#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "common.h"

void err_fun(const char *file_name, const int line, const char* fun_name, int err_no)
{
    fprintf(stderr, "in %s , %d fun %s is fail: %s\n",file_name, line, fun_name, strerror(err_no));
    exit(-1);
}

//创建/获取 消息队列
int mk_get_msg(int *msgid, mode_t creatmsg_mode, int proj_id)
{
    int fd = -1;
    key_t key = -1;

    /*创建一个新文件，新文件路径名用于生成key值，如果文件存在，
     *不报错，直接用
     * */
    fd = open(MSG_FILE, O_CREAT, 0664);
    if(fd < 0 && EEXIST != errno)
        err_fun(__FILE__, __LINE__, "open", errno);

    /*利用文件路径和课题ID获取key*/
    key = ftok(MSG_FILE, proj_id);
    if(key < -1)
        err_fun(__FILE__, __LINE__, "ftok", errno);

    /*如果没有被用，则创建新的消息队列，否则直接打开已有队列*/
    *msgid = msgget(key, IPC_CREAT | creatmsg_mode);
    if(*msgid < 0)
        err_fun(__FILE__, __LINE__, "msgget", errno);

    return 1;
}


//删除消息队列
int rm_msg(const int msgid)
{
    int ret = -1;

    ret = msgctl(msgid, IPC_RMID, NULL);
    if(ret < 0)
        err_fun(__FILE__, __LINE__, "msgget", errno);
    remove(MSG_FILE);
    return 1;
}


//创建/获取 共享内存
int mk_get_shm(int *shmid, mode_t creatshm_mode, int proj_id)
{
        int fd = -1;
        key_t key = -1;

        /* 创建用于获取key值的文件，可不用指定读写权限 */
        fd = open(SHM_FILE, O_CREAT|O_RDWR, 0664);
        if(fd<0 && EEXIST!=errno) err_fun(__FILE__, __LINE__, "open", errno);

        /* 获取key值 */
        key = ftok(SHM_FILE, proj_id);
        if(key < -1) err_fun(__FILE__, __LINE__, "ftok", errno);

        /* 根据key值创建新的或者获取已有共享内存 */
        *shmid = shmget(key, SHM_SIZE, IPC_CREAT|creatshm_mode);
        if(*shmid<0 && EINVAL!=errno) err_fun(__FILE__, __LINE__, "shmget", errno);

        return 1;
}


//释放共享内存
int rm_shm(const int shmid, void *addr)
{
        int ret = -1;

        /* 取消映射 */
        ret = shmdt(addr);
        if(ret < 0) err_fun(__FILE__, __LINE__, "shmdt", errno);

        /* 删除共享内存 */
        ret = shmctl(shmid, IPC_RMID, NULL);
        if(ret<0 && EINVAL!=errno) err_fun(__FILE__, __LINE__, "shmctl", errno);

        remove(SHM_FILE); //删除用于获取key值的文件

        return 1;
}


//创建/获取信号量
int get_sem(int *semid, const char *sem_file, int nsems, int proj_id, mode_t creatsem_mode)
{
    int fd = -1;
    key_t key = -1;

    fd = open(sem_file, O_CREAT | O_RDWR, 0644);
    if(fd < 0)
        err_fun(__FILE__, __LINE__, "open", errno);

    key = ftok(sem_file, proj_id);
    if(key < -1)
        err_fun(__FILE__, __LINE__, "ftok", errno);

    *semid  = semget(key, nsems, IPC_CREAT | creatsem_mode);
    if(*semid < 0)
        err_fun(__FILE__, __LINE__, "semget", errno);

    return 1;
}


//初始化sem
int init_sem(const int semid, int semnum, int value)//信号量的fd，信号量的编号，信号量的值
{
    int ret = -1;

    ret = semctl(semid, semnum, SETVAL, value);
    if(ret < 0)
    {
        err_fun(__FILE__, __LINE__, "semctl", errno);
        return -1;
    }

    return 1;
}


//删除sem
int del_sem(const int semid, int nsems, const char* filename)
{
    int ret = -1,i = 0;

    for(i = 0; i < nsems; i++)
    {
        ret = semctl(semid, i, IPC_RMID, NULL);
        if(ret < 0)
            err_fun(__FILE__, __LINE__, "semctl", errno);

    }

    remove(filename);

    return 1;
}


//v操作
int sem_v(const int semid, int semnum, int vn)
{
    //printf("vn %d\n",vn);
    int ret = -1;
    struct sembuf sops = {0,0,0};

    sops.sem_num = semnum;//信号编号，semid对用的信号集 中 第几个信号量
    sops.sem_op = vn;  	//v值
    sops.sem_flg = SEM_UNDO;

    ret = semop(semid, &sops, 1);
    if(ret < 0)
        err_fun(__FILE__, __LINE__, "semop", errno);

    return 1;
}


//p操作
int sem_p(const int semid, int semnum, int pn)
{
    sem_v(semid, semnum, -1*pn);

    return 1;
}


int lock_fun(int fd, int cmd, int l_type, int l_whence, off_t l_offset, off_t l_len)
{
    struct flock f_lock;
    f_lock.l_type   = l_type;
    f_lock.l_whence = l_whence;
    f_lock.l_start  = l_offset;
    f_lock.l_len    = l_len;
    return(fcntl(fd, cmd, &f_lock));

    return 1;
}
