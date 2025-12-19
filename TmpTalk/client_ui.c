#include "common.h"
#include <termios.h>
#include <unistd.h>
#include <pthread.h>

extern char my_project_id[];
extern char my_role[];
int process_command(char *msg, int sock, Packet *pkt);
void save_file_chunk(char *filename, char *data, int len);
void set_volatile_timer(int sec);
void redraw_chat(); 

pthread_mutex_t ui_mutex = PTHREAD_MUTEX_INITIALIZER;

volatile int is_running = 1;

#define MAX_HISTORY 100
#define MAX_ALERTS 3
#define VIEW_ROWS 15 

typedef struct {
    char msg[BUF_SIZE + 512]; 
    char raw_msg[BUF_SIZE]; 
    int is_volatile;        
    time_t timestamp;   
    int msg_id;  
    time_t volatile_end_time;
    int original_timer;     
} ChatLog;

ChatLog history[MAX_HISTORY];
int history_count = 0;
int next_msg_id = 1;
char current_file_name[256] = "";

char current_input_buf[BUF_SIZE] = ""; 
int current_input_len = 0;
struct termios orig_termios;

char system_alerts[MAX_ALERTS][BUF_SIZE];
int alert_count = 0;
time_t project_expire_time = 0;
long total_data_usage = 0;
int active_users = 1;

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); 
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
void cleanup_terminal() {
    disable_raw_mode();
}

void format_bytes(long bytes, char *out, size_t out_size) {
    if (bytes < 1024) snprintf(out, out_size, "%ldB", bytes);
    else if (bytes < 1024 * 1024) snprintf(out, out_size, "%.1fKB", bytes / 1024.0);
    else snprintf(out, out_size, "%.1fMB", bytes / (1024.0 * 1024.0));
}

void format_time_remaining(time_t expire_time, char *out, size_t out_size) {
    if (expire_time == 0) { snprintf(out, out_size, "∞"); return; }
    
    time_t now = time(NULL);
    long diff = expire_time - now;
    
    if (diff <= 0) {
        snprintf(out, out_size, ANSI_COLOR_RED "00:00:00" ANSI_COLOR_RESET);
    } else {
        long hours = diff / 3600;
        long mins = (diff % 3600) / 60;
        long secs = diff % 60;
        
        if (diff < 3600) {
            snprintf(out, out_size, ANSI_COLOR_RED "%02ld:%02ld:%02ld" ANSI_COLOR_RESET, hours, mins, secs);
        } else {
            snprintf(out, out_size, ANSI_COLOR_YELLOW "%02ld:%02ld:%02ld" ANSI_COLOR_RESET, hours, mins, secs);
        }
    }
}

void get_current_time(char *out, size_t out_size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(out, out_size, "%02d:%02d", t->tm_hour, t->tm_min);
}

const char* get_message_icon(const char *msg) {
    if (strstr(msg, "[알림]")) return "🚨";
    if (strstr(msg, "[폭탄]") || strstr(msg, "💣")) return "💣";
    if (strstr(msg, "[파일]")) return "📎";
    if (strstr(msg, "[시스템]")) return "🔔";
    if (strstr(msg, "[나]")) return "📤";
    return "💬";
}

void redraw_chat() {
    if (!is_running) return;

    pthread_mutex_lock(&ui_mutex);
    
    char buf[16384]; 
    int len = 0;
    
    len += sprintf(buf + len, "\033[?25l\033[2J\033[H"); 

    char data_str[32], time_str[64], current_time[16];
    format_bytes(total_data_usage, data_str, sizeof(data_str));
    format_time_remaining(project_expire_time, time_str, sizeof(time_str));
    get_current_time(current_time, sizeof(current_time));

    len += sprintf(buf + len, ANSI_COLOR_CYAN "╔═══════════════════════════════════════════════════════════════════╗\n");
    len += sprintf(buf + len, "║" ANSI_COLOR_RESET " ✨ " ANSI_COLOR_BOLD "Temp-Talk" ANSI_COLOR_RESET ANSI_COLOR_CYAN " - 임시조직 전용 메신저                           ║\n");
    len += sprintf(buf + len, "╠═══════════════════════════════════════════════════════════════════╣\n" ANSI_COLOR_RESET);
    
    char proj_line[256];
    snprintf(proj_line, sizeof(proj_line), "  📁 프로젝트: " ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET, my_project_id);
    len += sprintf(buf + len, ANSI_COLOR_CYAN "║" ANSI_COLOR_RESET "%-80s" ANSI_COLOR_CYAN "║\n", proj_line); 

    char role_line[256];
    snprintf(role_line, sizeof(role_line), "  👤 역할: " ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET, my_role);
    len += sprintf(buf + len, "║" ANSI_COLOR_RESET "%-85s" ANSI_COLOR_CYAN "║\n", role_line);
    len += sprintf(buf + len, "╠═══════════════════════════════════════════════════════════════════╣\n" ANSI_COLOR_RESET);
    len += sprintf(buf + len, ANSI_COLOR_CYAN "║" ANSI_COLOR_RESET "  📊 데이터: %-8s │ ⏰ 만료: %s │ 👥 " ANSI_COLOR_GREEN "%d명" ANSI_COLOR_RESET " │ 🕐 %s       " ANSI_COLOR_CYAN "║\n",
           data_str, time_str, active_users, current_time);
    len += sprintf(buf + len, "╚═══════════════════════════════════════════════════════════════════╝" ANSI_COLOR_RESET "\n");

    len += sprintf(buf + len, "\n");
    time_t now = time(NULL);
    int start_index = (history_count > VIEW_ROWS) ? history_count - VIEW_ROWS : 0;

    for (int i = start_index; i < history_count; i++) {
        if (history[i].is_volatile && history[i].volatile_end_time <= now) {
             len += sprintf(buf + len, ANSI_COLOR_DIM "  [만료됨] 💥 펑!\n" ANSI_COLOR_RESET);
             continue;
        }
        struct tm *t = localtime(&history[i].timestamp);
        const char *icon = get_message_icon(history[i].msg);
        if (history[i].is_volatile) {
            int remaining = (int)(history[i].volatile_end_time - now);
            const char *color = (remaining > history[i].original_timer / 2) ? ANSI_COLOR_YELLOW : ANSI_COLOR_RED;
            len += sprintf(buf + len, ANSI_COLOR_DIM "  [%02d:%02d]" ANSI_COLOR_RESET " %s💣 %s[%d초]%s %s\n",
                   t->tm_hour, t->tm_min, color, ANSI_COLOR_BOLD, remaining, ANSI_COLOR_RESET, history[i].msg);
        } else {
            len += sprintf(buf + len, ANSI_COLOR_DIM "  [%02d:%02d]" ANSI_COLOR_RESET " %s %s\n", 
                   t->tm_hour, t->tm_min, icon, history[i].msg);
        }
    }
    len += sprintf(buf + len, "\n" ANSI_COLOR_CYAN "───────────────────────────────────────────────────────────────────\n");
    len += sprintf(buf + len, ANSI_COLOR_DIM "  /upload  /list  /open  /bomb  /expire  /game  /who  /exit  /help\n" ANSI_COLOR_DIM);
    len += sprintf(buf + len, ANSI_COLOR_BOLD "\n💬 메시지 > %s" ANSI_COLOR_RESET, current_input_buf);
    len += sprintf(buf + len, "\033[?25h"); 
    write(STDOUT_FILENO, buf, len);
    pthread_mutex_unlock(&ui_mutex);
}

void add_to_history(const char *fmt_msg, char *raw, int is_volatile, int timer_sec) {
    if (!fmt_msg && !raw) return;
    pthread_mutex_lock(&ui_mutex);
    if (history_count >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++) history[i] = history[i + 1];
        history_count--;
    }
    ChatLog *log = &history[history_count];
    log->timestamp = time(NULL);
    log->msg_id = next_msg_id++;
    log->is_volatile = is_volatile;
    if (fmt_msg) snprintf(log->msg, sizeof(log->msg), "%s", fmt_msg);
    if (raw) snprintf(log->raw_msg, sizeof(log->raw_msg), "%s", raw);
    if (is_volatile) {
        log->volatile_end_time = time(NULL) + timer_sec;
        log->original_timer = timer_sec;
        total_data_usage += strlen(raw ? raw : "");
    } else {
        log->volatile_end_time = 0;
        total_data_usage += strlen(fmt_msg ? fmt_msg : "");
    }
    history_count++;
    pthread_mutex_unlock(&ui_mutex);
}

void *send_msg(void *arg) {
    int sock = *((int*)arg);
    char msg_buf[BUF_SIZE];
    Packet pkt;
    
    enable_raw_mode();
    atexit(cleanup_terminal);
    redraw_chat();

    while (is_running) { 
        char c = getchar();
        if (c == 127 || c == 8) {
            if (current_input_len > 0) {
                current_input_len--;
                current_input_buf[current_input_len] = '\0';
                redraw_chat(); 
            }
            continue;
        }
        if (c == '\n' || c == '\r') {
            if (current_input_len == 0) continue;
            strcpy(msg_buf, current_input_buf);
            memset(current_input_buf, 0, BUF_SIZE);
            current_input_len = 0;
            
            memset(&pkt, 0, sizeof(Packet));
            
            int is_cmd = process_command(msg_buf, sock, &pkt);

            if (is_cmd == 1) { redraw_chat(); continue; }

            if (pkt.type == 0) {
                pkt.type = MSG_CHAT;
                pkt.is_volatile = 0;
                strncpy(pkt.data, msg_buf, BUF_SIZE - 1);
                strcpy(pkt.project_id, my_project_id);
                strcpy(pkt.role, my_role);
            }

            if (!pkt.is_volatile && pkt.type == MSG_CHAT) { 
                char my_fmt[BUF_SIZE + 512];
                snprintf(my_fmt, sizeof(my_fmt), ANSI_COLOR_YELLOW "[나]" ANSI_COLOR_RESET " %s", msg_buf);
                add_to_history(my_fmt, NULL, 0, 0);
            }
            else if (pkt.is_volatile && pkt.type == MSG_CHAT) {
                add_to_history(pkt.data, pkt.data, 1, pkt.timer_sec);
            }
            
            if (pkt.type == MSG_EXPIRE_SET) {
                project_expire_time = time(NULL) + pkt.timer_sec;
                int display_days = pkt.timer_sec / 86400;
                char now_str[16];
                get_current_time(now_str, sizeof(now_str));
                char sys_msg[256];
                snprintf(sys_msg, sizeof(sys_msg), 
                         "[%s] 🔔 " ANSI_COLOR_YELLOW "[시스템] 프로젝트가 %d일 후 자동 소멸됩니다." ANSI_COLOR_RESET, 
                         now_str, display_days);
                add_to_history(sys_msg, NULL, 0, 0);
            }

            write(sock, &pkt, sizeof(Packet));
            redraw_chat(); 
            continue;
        }
        if (current_input_len < BUF_SIZE - 1) {
            current_input_buf[current_input_len++] = c;
            current_input_buf[current_input_len] = '\0';
            redraw_chat(); 
        }
    }
    return NULL;
}

void *recv_msg(void *arg) {
    int sock = *((int*)arg);
    Packet pkt;
    char fmt_msg[BUF_SIZE + 512];
    int str_len;

    while (is_running && (str_len = recvn(sock, &pkt, sizeof(Packet))) > 0) {
        if (str_len != sizeof(Packet)){
            break; 
        } 

        if (pkt.type == MSG_FILE_UPLOAD_START) {
            snprintf(fmt_msg, sizeof(fmt_msg), ANSI_COLOR_BLUE "[파일]" ANSI_COLOR_RESET " %s: %s", pkt.role, pkt.data);
            strcpy(current_file_name, pkt.data);
            add_to_history(fmt_msg, NULL, 0, 0);
            redraw_chat();
        }
        else if (pkt.type == MSG_FILE_DATA) {
            save_file_chunk(current_file_name, pkt.data, pkt.data_len);
            total_data_usage += pkt.data_len;
            redraw_chat(); 
        }
        else if (pkt.type == MSG_CHAT) {
            if (pkt.is_volatile) add_to_history(pkt.data, pkt.data, 1, pkt.timer_sec);
            else {
                snprintf(fmt_msg, sizeof(fmt_msg), ANSI_COLOR_CYAN "[%s]" ANSI_COLOR_RESET " %s", pkt.role, pkt.data);
                add_to_history(fmt_msg, NULL, 0, 0);
            }
            redraw_chat();
        }
        else if (pkt.type == MSG_LIST_RES || pkt.type == MSG_OPEN_RES) {
            char *line = strtok(pkt.data, "\n");
            while (line != NULL) {
                add_to_history(line, NULL, 0, 0);
                line = strtok(NULL, "\n");
            }
            redraw_chat();
        }
        else if (pkt.type == MSG_NAME_CHANGED) {
            strncpy(my_role, pkt.role, MAX_ROLE_LEN - 1);
            snprintf(fmt_msg, sizeof(fmt_msg), ANSI_COLOR_MAGENTA "[시스템] 이름 변경: %s" ANSI_COLOR_RESET, pkt.role);
            add_to_history(fmt_msg, NULL, 0, 0);
            redraw_chat();
        }
        
        else if (pkt.type == MSG_USER_COUNT) {
            active_users = pkt.data_len;
            redraw_chat();
        }
        
        else if (pkt.type == MSG_USER_JOIN || pkt.type == MSG_USER_LEAVE) {
            if (pkt.type == MSG_USER_JOIN) 
                snprintf(fmt_msg, sizeof(fmt_msg), ANSI_COLOR_GREEN "[입장]" ANSI_COLOR_RESET " %s", pkt.role);
            else 
                snprintf(fmt_msg, sizeof(fmt_msg), ANSI_COLOR_YELLOW "[퇴장]" ANSI_COLOR_RESET " %s", pkt.role);
            
            add_to_history(fmt_msg, NULL, 0, 0);
            redraw_chat();
        }
        else if (pkt.type == MSG_EXPIRE_SET) {
            project_expire_time = time(NULL) + pkt.timer_sec;
            int display_days = (pkt.timer_sec + 86399) / 86400;
            char now_str[16];
            get_current_time(now_str, sizeof(now_str));
            snprintf(fmt_msg, sizeof(fmt_msg), 
                     "[%s] 🔔 " ANSI_COLOR_YELLOW "[시스템] 프로젝트가 %d일 후 자동 소멸됩니다." ANSI_COLOR_RESET, 
                     now_str, display_days);
            add_to_history(fmt_msg, NULL, 0, 0);
            redraw_chat();
        }
        else if (pkt.type == MSG_PROJECT_END) {
            is_running = 0; 
            usleep(100000); 
            disable_raw_mode(); 
            printf("\n\n" ANSI_COLOR_RED "🚨 [시스템] 프로젝트 사용 기간이 만료되어 종료됩니다." ANSI_COLOR_RESET "\n");
            printf("서버 연결 종료\n");
            close(sock);
            exit(0); 
        }
    }
    
    is_running = 0;
    disable_raw_mode();
    printf("\n서버 연결 종료\n");
    exit(0);
    return NULL;
}

void *bomb_timer_thread(void *arg) {
    while (is_running) { 
        sleep(1); 
        int need_redraw = 0;
        time_t now = time(NULL);
        
        pthread_mutex_lock(&ui_mutex);
        for (int i = 0; i < history_count; i++) {
            if (history[i].is_volatile && history[i].volatile_end_time > 0) {
                if (history[i].volatile_end_time > now - 2) need_redraw = 1;
            }
        }
        pthread_mutex_unlock(&ui_mutex);
        
        if (project_expire_time > 0) need_redraw = 1;
        if (need_redraw) redraw_chat();
    }
    return NULL;
}

void *check_expiration(void *arg) {
    while (is_running) { sleep(60); }
    return NULL;
}