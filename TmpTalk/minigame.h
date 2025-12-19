#ifndef MINIGAME_H
#define MINIGAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "common.h" 

typedef struct {
    int is_active;           
    char current_answer[256]; 
    time_t start_time;      
} GameState;

void start_typing_game(char *requester_role, char *project_id);
int check_game_answer(char *msg, char *role, char *project_id);
void end_game();

#endif