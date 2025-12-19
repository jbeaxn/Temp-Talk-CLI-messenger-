#include "common.h"

// 클라이언트 정보 구조체
typedef struct {
    int socket;
    char project_id[MAX_ID_LEN];
    char role[MAX_ROLE_LEN];
} ClientInfo;

typedef struct {
    char project_id[MAX_ID_LEN];
    time_t expire_time;   
    int is_set;           
} ProjectState;

ClientInfo *clients[MAX_CLNT];
ProjectState projects[MAX_CLNT]; 
int clnt_cnt = 0;
int proj_cnt = 0; 

pthread_mutex_t mutx;

// 인원수 방송 (디버그 메시지 없음)
void broadcast_user_count(char *project_id) {
    int count = 0;
    for (int i = 0; i < clnt_cnt; i++) {
        if (strcmp(clients[i]->project_id, project_id) == 0) {
            count++;
        }
    }
    
    Packet pkt;
    memset(&pkt, 0, sizeof(Packet));
    pkt.type = MSG_USER_COUNT;
    pkt.data_len = count; 
    strcpy(pkt.project_id, project_id);

    for (int i = 0; i < clnt_cnt; i++) {
        if (strcmp(clients[i]->project_id, project_id) == 0) {
            write(clients[i]->socket, &pkt, sizeof(Packet));
        }
    }
}

// 만료 체크 스레드
void *monitor_expiration_thread(void *arg) {
    while (1) {
        sleep(1); 
        
        pthread_mutex_lock(&mutx);
        time_t now = time(NULL);
        
        for (int i = 0; i < proj_cnt; i++) {
            if (projects[i].is_set && projects[i].expire_time <= now) {
                char *target_proj = projects[i].project_id;
                
                for (int j = 0; j < clnt_cnt; j++) {
                    if (strcmp(clients[j]->project_id, target_proj) == 0) {
                        Packet end_pkt;
                        memset(&end_pkt, 0, sizeof(Packet));
                        end_pkt.type = MSG_PROJECT_END;
                        write(clients[j]->socket, &end_pkt, sizeof(Packet));
                        
                        printf(ANSI_COLOR_RED "[EXPIRE] Project '%s' expired. Logging out user: %s\n" ANSI_COLOR_RESET,
                               target_proj, clients[j]->role);
                               
                        close(clients[j]->socket);
                    }
                }
                projects[i].is_set = 0; 
                printf(ANSI_COLOR_RED "[SYSTEM] Project '%s' has been terminated.\n" ANSI_COLOR_RESET, target_proj);
                fflush(stdout);
            }
        }
        pthread_mutex_unlock(&mutx);
    }
    return NULL;
}

void init_room_manager() {
    pthread_mutex_init(&mutx, NULL);
    for(int i=0; i<MAX_CLNT; i++) projects[i].is_set = 0;
    
    pthread_t tid;
    pthread_create(&tid, NULL, monitor_expiration_thread, NULL);
    pthread_detach(tid);
}

void update_project_expiration(char *project_id, int seconds) {
    pthread_mutex_lock(&mutx);
    int found = -1;
    for (int i = 0; i < proj_cnt; i++) {
        if (strcmp(projects[i].project_id, project_id) == 0) {
            found = i;
            break;
        }
    }
    if (found == -1) {
        found = proj_cnt++;
        strcpy(projects[found].project_id, project_id);
    }
    projects[found].expire_time = time(NULL) + seconds;
    projects[found].is_set = 1;
    printf(ANSI_COLOR_YELLOW "[Timer] Project '%s' will expire in %d seconds.\n" ANSI_COLOR_RESET, 
           project_id, seconds);
    fflush(stdout);
    pthread_mutex_unlock(&mutx);
}

void send_packet_to_all(Packet *pkt) {
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) {
        if (strcmp(clients[i]->project_id, pkt->project_id) == 0) {
            write(clients[i]->socket, pkt, sizeof(Packet));
        }
    }
    pthread_mutex_unlock(&mutx);
}

void add_client(int sock, char *id, char *role) {
    pthread_mutex_lock(&mutx);
    
    char final_role[MAX_ROLE_LEN + 10]; 
    strcpy(final_role, role);           
    
    int suffix = 2;      
    int is_duplicate = 1; 
    int name_changed = 0; 

    while (is_duplicate) {
        is_duplicate = 0; 
        for (int i = 0; i < clnt_cnt; i++) {
            if (strcmp(clients[i]->project_id, id) == 0 && 
                strcmp(clients[i]->role, final_role) == 0) {
                is_duplicate = 1; 
                break;
            }
        }
        if (is_duplicate) {
            // [추가된 부분] 서버 로그에 이름 변경 사실 출력
            printf(ANSI_COLOR_YELLOW "[System] Name '%s' exists in Project '%s'. Changing to '%s_%d'...\n" ANSI_COLOR_RESET, 
                   final_role, id, role, suffix);
            
            sprintf(final_role, "%s_%d", role, suffix);
            suffix++;
            name_changed = 1; 
        }
    }
    
    clients[clnt_cnt] = (ClientInfo*)malloc(sizeof(ClientInfo));
    clients[clnt_cnt]->socket = sock;
    strcpy(clients[clnt_cnt]->project_id, id);
    strcpy(clients[clnt_cnt]->role, final_role); 
    
    if (name_changed) {
        Packet change_pkt;
        memset(&change_pkt, 0, sizeof(Packet));
        change_pkt.type = MSG_NAME_CHANGED;
        strcpy(change_pkt.role, final_role); 
        write(sock, &change_pkt, sizeof(Packet));
    }
    
    // 만료 시간 전송
    for (int i = 0; i < proj_cnt; i++) {
        if (strcmp(projects[i].project_id, id) == 0 && projects[i].is_set) {
            time_t now = time(NULL);
            long diff = projects[i].expire_time - now;
            if (diff > 0) {
                Packet expire_pkt;
                memset(&expire_pkt, 0, sizeof(Packet));
                expire_pkt.type = MSG_EXPIRE_SET;
                expire_pkt.timer_sec = (int)diff; 
                strcpy(expire_pkt.project_id, id);
                write(sock, &expire_pkt, sizeof(Packet));
            }
            break;
        }
    }

    clnt_cnt++; 

    // 인원수 갱신 방송
    broadcast_user_count(id);

    pthread_mutex_unlock(&mutx);
}

void remove_client(int sock) {
    pthread_mutex_lock(&mutx);
    
    char target_project[MAX_ID_LEN] = {0}; 
    
    for (int i = 0; i < clnt_cnt; i++) {
        if (clients[i]->socket == sock) {
            printf(ANSI_COLOR_RED "[Logout] Project: %s | User: %s Disconnected.\n" ANSI_COLOR_RESET, 
                   clients[i]->project_id, clients[i]->role);
            
            strcpy(target_project, clients[i]->project_id);
            
            free(clients[i]); 
            while (i < clnt_cnt - 1) {
                clients[i] = clients[i + 1];
                i++;
            }
            clnt_cnt--;
            break;
        }
    }

    if (strlen(target_project) > 0) {
        broadcast_user_count(target_project);
    }
    
    pthread_mutex_unlock(&mutx);
}

void send_msg_to_room(Packet *pkt, int sender_sock) {
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) {
        if (strcmp(clients[i]->project_id, pkt->project_id) == 0 && clients[i]->socket != sender_sock) {
             write(clients[i]->socket, pkt, sizeof(Packet));
        }
    }
    pthread_mutex_unlock(&mutx);
}

void get_user_list(char *project_id, char *output_buf) {
    pthread_mutex_lock(&mutx);
    sprintf(output_buf, "< Current Users in Project: %s >\n", project_id);
    for (int i = 0; i < clnt_cnt; i++) {
        if (strcmp(clients[i]->project_id, project_id) == 0) {
            strcat(output_buf, "- ");
            strcat(output_buf, clients[i]->role);
            strcat(output_buf, "\n");
        }
    }
    pthread_mutex_unlock(&mutx);
}