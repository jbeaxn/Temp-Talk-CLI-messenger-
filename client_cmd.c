#include "common.h"

extern char my_project_id[];
extern char my_role[];
void upload_file(int sock, char *filename); 
// client_ui.c에서 정의된 함수
void add_to_history(const char *fmt_msg, char *raw, int is_volatile, int timer_sec);


static int is_command(const char *msg, const char *cmd) {
    size_t len = strlen(cmd);
    return (strncmp(msg, cmd, len) == 0 && 
            (msg[len] == '\0' || msg[len] == ' '));
}

int process_command(char *msg, int sock, Packet *pkt) {
    // 명령어가 아니면 일반 메시지
    if (!msg || msg[0] != '/') return 0;
    
    // 1. 파일 업로드
    if (is_command(msg, "/upload")) {
        char filename[256];
        sscanf(msg + 8, "%s", filename);
        upload_file(sock, filename);
        return 1; 
    }
    
    // 2. 파일 목록
    else if (is_command(msg, "/list")) {
        memset(pkt, 0, sizeof(Packet));
        pkt->type = MSG_COMMAND;
        strcpy(pkt->data, "/list");
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0; 
    }
    
    // 3. 파일 열기
    else if (is_command(msg, "/open")) {
        char target_file[256];
        sscanf(msg + 6, "%s", target_file);

        pkt->type = MSG_OPEN_REQ;
        strcpy(pkt->data, target_file); 
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0;
    }
    
    // 4. 참여자 확인
    else if (is_command(msg, "/who")) {
        memset(pkt, 0, sizeof(Packet));
        pkt->type = MSG_COMMAND;
        strcpy(pkt->data, "/who");
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0;
    }
    
    // 5. 게임
    else if (is_command(msg, "/game")) {
        memset(pkt, 0, sizeof(Packet));
        pkt->type = MSG_COMMAND;
        strcpy(pkt->data, "/game");
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0;
    }
    
    // 6. 폭탄 메시지
    else if (is_command(msg, "/bomb")) {
        int sec;
        char content[BUF_SIZE] = {0};
        if (sscanf(msg + 6, "%d %[^\n]", &sec, content) != 2) {
            fprintf(stderr, "❌ Usage: /bomb [초] [메시지]\n");
            return 1;
        }
        if (sec < 1 || sec > 300) {
            fprintf(stderr, "❌ 타이머는 1-300초여야 합니다\n");
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
    
    /*
    // 7. ✅ 시스템 알림
    else if (is_command(msg, "/alert")) {
        
        memset(pkt, 0, sizeof(Packet));
        pkt->type = MSG_ANNOUNCEMENT;
        strncpy(pkt->data, content, BUF_SIZE - 1);
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0;
    }
    */
    
    // 8. 프로젝트 만료
    else if (is_command(msg, "/expire")) {
        int days;
        if (sscanf(msg, "/expire %d", &days) != 1) {
        // 숫자가 제대로 입력되지 않은 경우
        char err_msg[BUF_SIZE];
        snprintf(err_msg, sizeof(err_msg),
                 ANSI_COLOR_RED "[시스템] 사용법: /expire <일수>" ANSI_COLOR_RESET);
        add_to_history(err_msg, NULL, 0, 0);
        return 1; // 명령 처리 끝
    }
        
        pkt->type = MSG_EXPIRE_SET;
        pkt->timer_sec = days;
        strcpy(pkt->project_id, my_project_id);
        strcpy(pkt->role, my_role);
        return 0;
    }
    
    // 9. 도움말
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
        getchar(); // ✅ 사용자가 엔터를 누를 때까지 대기
        return 1;
    }
    
    // 10. 종료
    else if (is_command(msg, "/exit")) {
        printf("\n" ANSI_COLOR_YELLOW "👋 종료합니다...\n" ANSI_COLOR_RESET);
        close(sock);
        exit(0);
    }
    
    // 알 수 없는 명령어
    else {
        fprintf(stderr, ANSI_COLOR_RED " 알 수 없는 명령어: %s\n" ANSI_COLOR_RESET, msg);
        fprintf(stderr, "💡 '/help'를 입력하세요\n");
        return 1;
    }
    
    return 0;
}