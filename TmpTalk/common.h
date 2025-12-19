#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>

#define PORT 8080
#define BUF_SIZE 1024
#define FILE_BUF_SIZE 4096
#define MAX_CLNT 100
#define MAX_ID_LEN 30
#define MAX_ROLE_LEN 20

#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
#define ANSI_COLOR_BLUE     "\x1b[34m"
#define ANSI_COLOR_MAGENTA  "\x1b[35m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_WHITE    "\x1b[37m"

#define ANSI_COLOR_RESET    "\x1b[0m"
#define ANSI_COLOR_BOLD     "\x1b[1m"
#define ANSI_COLOR_DIM      "\x1b[2m"

typedef enum {
    MSG_LOGIN,
    MSG_CHAT,
    MSG_FILE_UPLOAD_START,
    MSG_FILE_DATA,
    MSG_COMMAND,
    MSG_LIST_RES,
    MSG_NAME_CHANGED,
    MSG_EXPIRE_SET,
    MSG_EXPIRE_WARNING,
    MSG_PROJECT_END,
    MSG_OPEN_REQ,
    MSG_OPEN_RES,
    MSG_EXIT,

    MSG_USER_JOIN,
    MSG_USER_LEAVE,
    MSG_USER_COUNT,
    MSG_ANNOUNCEMENT
} MsgType;

typedef struct {
    MsgType type;
    char project_id[MAX_ID_LEN];
    char role[MAX_ROLE_LEN];
    char data[BUF_SIZE];
    int data_len;

    int is_volatile;
    int timer_sec;
} Packet;

static inline ssize_t recvn(int fd, void *vptr, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *ptr = (char *)vptr;

    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) nread = 0;
            else return -1;
        } else if (nread == 0) {
            break;
        }
        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft);
}

void print_help(void);

#endif
