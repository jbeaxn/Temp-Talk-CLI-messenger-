#include "common.h"

// 클라이언트 정보 구조체
typedef struct {
    int socket;
    char project_id[MAX_ID_LEN];
    char role[MAX_ROLE_LEN];
} ClientInfo;

// [신규] 프로젝트 상태 관리 (만료 시간 저장용)
typedef struct {
    char project_id[MAX_ID_LEN];
    time_t expire_time; 
    int is_set;
} ProjectInfo;

// ============================================================================
// [중요] 함수 프로토타입 선언 (순서 에러 방지용)
// ============================================================================
void send_msg_to_room(Packet *pkt, int sender_sock);
void broadcast_user_count(char *project_id);
void send_packet_to_all(Packet *pkt);

// ============================================================================
// [전역 변수]
// ============================================================================
ClientInfo *clients[MAX_CLNT];
ProjectInfo projects[100]; 
int clnt_cnt = 0;
int proj_cnt = 0;
pthread_mutex_t mutx;

void init_room_manager() {
    pthread_mutex_init(&mutx, NULL);
    memset(projects, 0, sizeof(projects)); 
}

// [신규] 프로젝트별 접속자 수 계산 및 브로드캐스트
void broadcast_user_count(char *project_id) {
    int count = 0;
    // 1. 해당 프로젝트 인원 수 계산
    for (int i = 0; i < clnt_cnt; i++) {
        if (strcmp(clients[i]->project_id, project_id) == 0) {
            count++;
        }
    }

    // 2. 패킷 생성
    Packet pkt;
    memset(&pkt, 0, sizeof(Packet));
    pkt.type = MSG_USER_COUNT;
    strcpy(pkt.project_id, project_id);
    pkt.data_len = count; // data_len에 인원수를 담아서 보냄

    // 3. 해당 프로젝트 인원들에게 전송
    for (int i = 0; i < clnt_cnt; i++) {
        if (strcmp(clients[i]->project_id, project_id) == 0) {
            write(clients[i]->socket, &pkt, sizeof(Packet));
        }
    }
}

// [신규] 만료 시간 설정 및 저장
void set_project_expiration(char *project_id, int days) {
    pthread_mutex_lock(&mutx);
    int found = 0;
    time_t now = time(NULL);
    time_t new_expire = now + (days * 86400);

    for (int i = 0; i < proj_cnt; i++) {
        if (strcmp(projects[i].project_id, project_id) == 0) {
            projects[i].expire_time = new_expire;
            projects[i].is_set = 1;
            found = 1;
            break;
        }
    }

    if (!found && proj_cnt < 100) {
        strcpy(projects[proj_cnt].project_id, project_id);
        projects[proj_cnt].expire_time = new_expire;
        projects[proj_cnt].is_set = 1;
        proj_cnt++;
    }
    pthread_mutex_unlock(&mutx);
}

// 특정 프로젝트의 모든 인원에게 패킷 전송 (나 포함)
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
    
    // [해결 1] 접속 시 저장된 만료 시간이 있다면 동기화 패킷 전송
    for (int i = 0; i < proj_cnt; i++) {
        if (strcmp(projects[i].project_id, id) == 0 && projects[i].is_set) {
            time_t now = time(NULL);
            long remaining = projects[i].expire_time - now;
            int days_left = remaining / 86400;
            if (days_left < 0) days_left = 0;

            Packet exp_pkt;
            memset(&exp_pkt, 0, sizeof(Packet));
            exp_pkt.type = MSG_EXPIRE_SET;
            exp_pkt.timer_sec = days_left; 
            strcpy(exp_pkt.project_id, id);
            write(sock, &exp_pkt, sizeof(Packet));
            break;
        }
    }

    pthread_mutex_unlock(&mutx);

    // [해결 2] 접속자 수 갱신 패킷 전송 (mutex 밖에서 호출)
    broadcast_user_count(id); 

    // 입장 메시지 전송
    Packet join_pkt;
    memset(&join_pkt, 0, sizeof(Packet));
    join_pkt.type = MSG_USER_JOIN;
    strcpy(join_pkt.role, final_role);
    strcpy(join_pkt.project_id, id);
    send_msg_to_room(&join_pkt, sock); 
}

void remove_client(int sock) {
    pthread_mutex_lock(&mutx);
    char project_id[MAX_ID_LEN] = "";
    char role[MAX_ROLE_LEN] = "";
    int found = 0;

    for (int i = 0; i < clnt_cnt; i++) {
        if (clients[i]->socket == sock) {
            printf(ANSI_COLOR_RED "[Logout] Project: %s | User: %s Disconnected.\n" ANSI_COLOR_RESET, 
                   clients[i]->project_id, clients[i]->role);
            
            strcpy(project_id, clients[i]->project_id);
            strcpy(role, clients[i]->role);
            free(clients[i]); 
            
            while (i < clnt_cnt - 1) {
                clients[i] = clients[i + 1];
                i++;
            }
            clnt_cnt--;
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&mutx);

    if (found) {
        // [해결 2] 퇴장 시 남은 사람들에게 접속자 수 갱신
        broadcast_user_count(project_id);

        Packet leave_pkt;
        memset(&leave_pkt, 0, sizeof(Packet));
        leave_pkt.type = MSG_USER_LEAVE;
        strcpy(leave_pkt.role, role);
        strcpy(leave_pkt.project_id, project_id);
        send_msg_to_room(&leave_pkt, -1); 
    }
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