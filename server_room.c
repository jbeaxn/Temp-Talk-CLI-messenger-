#include "common.h"

// 클라이언트 정보 구조체
typedef struct {
    int socket;
    char project_id[MAX_ID_LEN];
    char role[MAX_ROLE_LEN];
} ClientInfo;

ClientInfo *clients[MAX_CLNT];
int clnt_cnt = 0;
pthread_mutex_t mutx;

void init_room_manager() {
    pthread_mutex_init(&mutx, NULL);
}

// [신규] 특정 프로젝트의 모든 인원에게 패킷 전송 (나 포함)
// minigame.c 에서 사용하기 위해 전역으로 둠
void send_packet_to_all(Packet *pkt) {
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) {
        // 프로젝트 ID가 같은 사람들에게만 전송
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
    
    clnt_cnt++;
    pthread_mutex_unlock(&mutx);
}

void remove_client(int sock) {
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < clnt_cnt; i++) {
        if (clients[i]->socket == sock) {
            printf(ANSI_COLOR_RED "[Logout] Project: %s | User: %s Disconnected.\n" ANSI_COLOR_RESET, 
                   clients[i]->project_id, clients[i]->role);
            free(clients[i]); 
            while (i < clnt_cnt - 1) {
                clients[i] = clients[i + 1];
                i++;
            }
            clnt_cnt--;
            break;
        }
    }
    pthread_mutex_unlock(&mutx);
}

// 나(sender_sock)를 제외하고 전송
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