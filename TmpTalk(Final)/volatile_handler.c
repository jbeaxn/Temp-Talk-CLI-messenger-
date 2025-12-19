#include "common.h"

// client_ui.c에 있는 함수를 쓰겠다고 선언
extern void delete_volatile_msg();

void handle_alarm(int sig) {
    // 화면 전체를 지우는 대신, UI에 있는 '삭제 후 다시 그리기' 함수 호출
    delete_volatile_msg();
    
    // 안내 메시지가 필요하면 출력 (선택 사항)
    // printf(ANSI_COLOR_RED "\n[System] 휘발성 메시지가 소멸했습니다.\n" ANSI_COLOR_RESET);
}

void init_signal_handler() {
    signal(SIGALRM, handle_alarm);
}

void set_volatile_timer(int sec) {
    alarm(sec); // sec초 후에 SIGALRM 발생
}