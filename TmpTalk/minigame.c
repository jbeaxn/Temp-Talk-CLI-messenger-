#include "minigame.h"

extern void send_packet_to_all(Packet *pkt);

// 미니게임 문제
const char *quiz_sentences[] = {
    "System Programming is fun",
    "Show me the money",
    "Stay hungry stay foolish",
    "Talk is cheap show me the code",
    "Temp Talk is the best messenger",
    "Wake up Neo",
    "Hello World",
    "Time is gold",
    "Winter is coming",
    "May the force be with you"
};
const int quiz_count = 10;

GameState game_state = {0, "", 0};

// 현재 게임이 진행 중인 프로젝트 ID
char current_game_project[MAX_ID_LEN] = ""; 

pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

void start_typing_game(char *requester_role, char *project_id) {
    pthread_mutex_lock(&game_mutex);
    
    Packet pkt;
    memset(&pkt, 0, sizeof(Packet));
    pkt.type = MSG_CHAT;
    strcpy(pkt.role, "[Game]");
    strcpy(pkt.project_id, project_id);

    // 이미 게임 중인지 확인
    if (game_state.is_active) {
        snprintf(pkt.data, sizeof(pkt.data), 
                 "\n⚠️  이미 게임이 진행 중입니다! (Project: %s)\n", 
                 current_game_project);
        send_packet_to_all(&pkt); 
        pthread_mutex_unlock(&game_mutex);
        return;
    }

    // 문제 출제
    srand(time(NULL));
    int idx = rand() % quiz_count;
    strcpy(game_state.current_answer, quiz_sentences[idx]);
    
    game_state.is_active = 1;
    game_state.start_time = time(NULL);
    strcpy(current_game_project, project_id); // 현재 게임 방 설정

    snprintf(pkt.data, sizeof(pkt.data), 
             "\n"
             "✨━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━✨\n"
             "\n"
             "        🎮  스피드 타자 게임 시작!  🎮\n"
             "\n"
             "    💨 아래 문장을 가장 빠르게 입력하세요!\n"
             "\n"
             "    📝  %s\n"
             "\n"
             "✨━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━✨\n",
             game_state.current_answer);
    
    send_packet_to_all(&pkt);
    
    pthread_mutex_unlock(&game_mutex);
}

// 정답 확인 함수 (정답이면 1 반환)
int check_game_answer(char *msg, char *role, char *project_id) {
    int is_correct = 0;

    pthread_mutex_lock(&game_mutex);
    
    // 게임 중이고, 해당 프로젝트에서 발생한 메시지인지 확인
    if (game_state.is_active && strcmp(current_game_project, project_id) == 0) {
        if (strcmp(msg, game_state.current_answer) == 0) {
            is_correct = 1;
            game_state.is_active = 0; // 게임 종료
            
            double elapsed = difftime(time(NULL), game_state.start_time);
            
            Packet pkt;
            memset(&pkt, 0, sizeof(Packet));
            pkt.type = MSG_CHAT;
            strcpy(pkt.role, "[Game]");
            strcpy(pkt.project_id, project_id);
            
            snprintf(pkt.data, sizeof(pkt.data), 
                     "\n"
                     "🎉━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━🎉\n"
                     "\n"
                     "              ✨ 정답입니다! ✨\n"
                     "\n"
                     "               🏆   우승자: %s\n"
                     "               ⏱️   기록: %.2f초\n"
                     "\n"
                     "                          축하합니다! 🎊\n"
                     "\n"
                     "🎉━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━🎉\n",
                     role, elapsed);
            send_packet_to_all(&pkt);
        }
    }
    
    pthread_mutex_unlock(&game_mutex);
    return is_correct;
}

void end_game() {
    pthread_mutex_lock(&game_mutex);
    game_state.is_active = 0;
    pthread_mutex_unlock(&game_mutex);
}