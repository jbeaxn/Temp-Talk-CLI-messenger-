#include "common.h"

extern char my_project_id[];
extern char my_role[];
void upload_file(int sock, char *filename); 
void add_to_history(const char *fmt_msg, char *raw, int is_volatile, int timer_sec);

static int is_command(const char *msg, const char *cmd) {
    size_t len = strlen(cmd);
    return (strncmp(msg, cmd, len) == 0 && 
            (msg[len] == '\0' || msg[len] == ' '));
}

int process_command(char *msg, int sock, Packet *pkt) {
    // 명령어가 아니면 일반 메시지
    if (!msg || msg[0] != '/') return 0;
    
    // /upload
    if (is_command(msg, "/upload")) {
        char filename[256];
        sscanf(msg + 8, "%s", filename);
        upload_file(sock, filename);
        return 1; 
    }

    // /list
    else if (is_command(msg, "/list")) {
        memset(pkt, 0, sizeof(Packet));
        pkt->type = MSG_COMMAND;
        strcpy(pkt->data, "/list");
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0; 
    }

    // /open
    else if (is_command(msg, "/open")) {
        char target_file[256];
        sscanf(msg + 6, "%s", target_file);
        pkt->type = MSG_OPEN_REQ;
        strcpy(pkt->data, target_file); 
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0;
    }

    // /who
    else if (is_command(msg, "/who")) {
        memset(pkt, 0, sizeof(Packet));
        pkt->type = MSG_COMMAND;
        strcpy(pkt->data, "/who");
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0;
    }

    // /who
    else if (is_command(msg, "/game")) {
        memset(pkt, 0, sizeof(Packet));
        pkt->type = MSG_COMMAND;
        strcpy(pkt->data, "/game");
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0;
    }

    // /bomb
    else if (is_command(msg, "/bomb")) {
        int sec;
        char content[BUF_SIZE] = {0};
        if (sscanf(msg + 6, "%d %[^\n]", &sec, content) != 2) {
            char err[100]; sprintf(err, ANSI_COLOR_RED "Usage: /bomb [sec] [msg]" ANSI_COLOR_RESET);
            add_to_history(err, NULL, 0, 0);
            return 1;
        }
        memset(pkt, 0, sizeof(Packet));
        pkt->type = MSG_CHAT;
        pkt->is_volatile = 1;
        pkt->timer_sec = sec;
        strncpy(pkt->data, content, BUF_SIZE - 1);
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0; 
    }

    // /alert
    else if (is_command(msg, "/alert")) {
        char alert_msg[BUF_SIZE];
        sscanf(msg + 7, "%[^\n]", alert_msg);
        memset(pkt, 0, sizeof(Packet));
        pkt->type = MSG_ALERT;
        strncpy(pkt->data, alert_msg, BUF_SIZE - 1);
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0; 
    }

    // /expire
    else if (is_command(msg, "/expire")) {
        int days;
        if (sscanf(msg, "/expire %d", &days) != 1) {
            char err_msg[BUF_SIZE];
            snprintf(err_msg, sizeof(err_msg), ANSI_COLOR_RED "[시스템] 사용법: /expire <일수>" ANSI_COLOR_RESET);
            add_to_history(err_msg, NULL, 0, 0);
            return 1; 
        }
        
        pkt->type = MSG_EXPIRE_SET;
        pkt->timer_sec = days * 86400; 
        //pkt->timer_sec = days; (테스트용 /expire 30 -> 30초후 폭파)
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0;
    }

    // /help
    else if (is_command(msg, "/help")) {
        printf("\n" ANSI_COLOR_CYAN);
        printf("╔═══════════════════════════════════════════╗\n" ANSI_COLOR_RESET);
        printf("║       ✨ Temp-Talk 명령어 도움말          ║\n" ANSI_COLOR_RESET);
        printf("╠═══════════════════════════════════════════╣\n" ANSI_COLOR_RESET);
        printf("║ 📎 /upload [파일]  - 파일 업로드          ║\n" ANSI_COLOR_RESET);
        printf("║ 📋 /list           - 파일 목록            ║\n" ANSI_COLOR_RESET);
        printf("║ 📂 /open [파일]    - 파일 열기            ║\n" ANSI_COLOR_RESET);
        printf("║ 👥 /who            - 참여자 확인          ║\n" ANSI_COLOR_RESET);
        printf("║ 🎮 /game           - 미니게임             ║\n" ANSI_COLOR_RESET);
        printf("║ 💣 /bomb [초] [내용] - 폭탄 메시지        ║\n" ANSI_COLOR_RESET);
        printf("║ 🚨 /alert [내용]   - 시스템 알림          ║\n" ANSI_COLOR_RESET);
        printf("║ ⏰ /expire [일]    - 프로젝트 만료        ║\n" ANSI_COLOR_RESET);
        printf("║ ❓ /help           - 도움말               ║\n" ANSI_COLOR_RESET);
        printf("║ 🚪 /exit           - 종료                 ║\n" ANSI_COLOR_RESET);
        printf("╚═══════════════════════════════════════════╝\n" ANSI_COLOR_RESET);
        printf("\n💡 엔터를 누르면 채팅으로 돌아갑니다...\n");
        getchar();
        return 1;
    }
    else if (is_command(msg, "/exit")) {
        close(sock);
        exit(0);
    }
    else {
        add_to_history(ANSI_COLOR_RED "알 수 없는 명령어입니다." ANSI_COLOR_RESET, NULL, 0, 0);
        return 1;
    }
    return 0;
}