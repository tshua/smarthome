#ifndef H_COMMON_H
#define H_COMMON_H

#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

#define MSG_FILE_DEV "./files/msgfile_dev"
#define MSG_FILE_PHONE "./files/msgfile_phone"
#define MSG_FILE_SERVER "./files/msgfile_server"

#define MSG_TYPE 1
#define MSG_SIZE 1024

#define SHM_FILE "./shmfile"
#define SHM_SIZE 1024

#define SEM_FILE "./files/sem_file"
#define NSEMS 2

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include "../header/protocol.h"


#define read_lock(fd, l_whence, l_offset, l_len) \
    lock_fun(fd, F_SETLK, F_RDLCK, l_whence, l_offset, l_len)
#define read_lockw(wfd, l_whence, l_offset, l_len)\
    lock_fun(fd, F_SETLKW, F_RDLCK, l_whence, l_offset, l_len)
#define write_lock(fd, l_whence, l_offset, l_len)\
    lock_fun(fd, F_SETLK, F_WRLCK, l_whence, l_offset, l_len)
#define write_lockw(fd, l_whence, l_offset, l_len)\
    lock_fun(fd, F_SETLKW, F_WRLCK, l_whence, l_offset, l_len)
#define unlock(fd, l_whence, l_offset, l_len)\
    lock_fun(fd, F_SETLK, F_UNLCK, l_whence, l_offset, l_len)



int lock_fun(int fd, int cmd, int l_type, int l_whence, off_t l_offset, off_t l_len);


union semun
{
    int            val;    /* Value for SETVAL */
    struct semid_ds *buf;      /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;    /* Array for GETALL, SETALL */
    struct seminfo  *__buf;    /* Buffer for IPC_INFO */
};

struct pthread_arg
{
    int pth_no;
    pthread_t pth_id;
    int semid;
    void *addr;
    int sockfd;
};

struct Msgbuf
{
    long mtype;
    char mtext[MAX_MSG_SIZE];
};

void err_fun(const char *file_name, const int line, const char* fun_name, int err_no);

//创建/获取 消息队列
int mk_get_msg(int *msgid, const char* filename, mode_t creatmsg_mode, int proj_id);

//删除消息队列
int rm_msg(const int msgid, const char* filename);

//创建/获取 共享内存
int mk_get_shm(int *shmid, mode_t creatshm_mode, int proj_id);

//释放共享内存
int rm_shm(const int shmida, void *addr);

//创建/获取信号量
int get_sem(int *semid, const char *sem_file, int nsems, int proj_id, mode_t creatsem_mode);

//初始化sem
int init_sem(const int semid, int sennum, int value);

//删除sem
int del_sem(const int semid, int nsems, const char* filename);

//p操作
int sem_p(const int semid, int semnum, int pn);

//v操作
int sem_v(const int semid, int semnum, int vn);



#endif
