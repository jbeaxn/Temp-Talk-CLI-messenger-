#include "common.h"

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void write_log(Packet *pkt) {
    // [보안] 휘발성 메시지는 절대 로그를 남기지 않음
    if (pkt->is_volatile) return;

    pthread_mutex_lock(&log_mutex);
    int fd = open("server_log.txt", O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd != -1) {
        // [수정] 버퍼 크기를 넉넉하게 늘림 (기존 256 -> BUF_SIZE + 512)
        char log_buf[BUF_SIZE + 512]; 
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        
        sprintf(log_buf, "[%02d:%02d:%02d] [%s] %s: %s\n", 
                tm.tm_hour, tm.tm_min, tm.tm_sec, 
                pkt->project_id, pkt->role, 
                (pkt->type == MSG_FILE_DATA) ? "[FILE DATA]" : pkt->data);
        
        write(fd, log_buf, strlen(log_buf));
        close(fd);
    }
    pthread_mutex_unlock(&log_mutex);
}