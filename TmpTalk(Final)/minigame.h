#ifndef MINIGAME_H
#define MINIGAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "common.h" 

// 게임 상태 구조체
typedef struct {
    int is_active;           // 게임 진행 중 여부
    char current_answer[256]; // 정답 문장
    time_t start_time;       // 시작 시간
} GameState;

// [수정] 프로젝트 ID를 인자로 받도록 변경
void start_typing_game(char *requester_role, char *project_id);
int check_game_answer(char *msg, char *role, char *project_id);
void end_game();

#endif