#include "common.h"

extern char my_project_id[];
extern char my_role[];

extern void add_to_history(char *fmt_msg, char *role, char *raw, int is_volatile, int timer_sec);
extern void redraw_chat();

void upload_file(int sock, char *filename) {
    char log_msg[BUF_SIZE + 512]; 
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        sprintf(log_msg, ANSI_COLOR_RED "[System] 파일을 찾을 수 없습니다: %s" ANSI_COLOR_RESET, filename);
        add_to_history(log_msg, NULL, NULL, 0, 0);
        redraw_chat(); 
        return;
    }

    sprintf(log_msg, ANSI_COLOR_BLUE "[File] 파일 업로드를 시작했습니다: %s" ANSI_COLOR_RESET, filename);
    add_to_history(log_msg, NULL, NULL, 0, 0);
    redraw_chat(); 

    Packet pkt;
    pkt.type = MSG_FILE_UPLOAD_START;
    strcpy(pkt.project_id, my_project_id);
    strcpy(pkt.role, my_role);
    
    char *pure_filename = strrchr(filename, '/'); 
    if (pure_filename) {
        strcpy(pkt.data, pure_filename + 1); 
    } else {
        strcpy(pkt.data, filename); 
    }
    write(sock, &pkt, sizeof(Packet));

    pkt.type = MSG_FILE_DATA;
    int len;
    while ((len = read(fd, pkt.data, BUF_SIZE)) > 0) {
        pkt.data_len = len;
        write(sock, &pkt, sizeof(Packet));
        usleep(1000); 
    }
    close(fd);
}

void save_file_chunk(char *filename, char *data, int len) {
    char save_name[300];
    sprintf(save_name, "recv_%s", filename);

    int fd = open(save_name, O_WRONLY | O_CREAT | O_APPEND, 0644);
    write(fd, data, len);
    close(fd);
}