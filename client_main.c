#include "common.h"

// 외부 함수
void *send_msg(void *arg);
void *recv_msg(void *arg);
void *bomb_timer_thread(void *arg);
void *check_expiration(void *arg);

char my_project_id[MAX_ID_LEN];
char my_role[MAX_ROLE_LEN];
int sock;

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread, timer_thread, expire_thread;

    printf("Enter Project ID: ");
    scanf("%s", my_project_id);
    printf("Enter Your Role (Name): ");
    scanf("%s", my_role);
    getchar(); // 버퍼 비우기


    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 로컬호스트
    serv_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect() error");
        exit(1);
    }

    // 로그인 패킷 전송
    Packet login_pkt;
    login_pkt.type = MSG_LOGIN;
    strcpy(login_pkt.project_id, my_project_id);
    strcpy(login_pkt.role, my_role);
    write(sock, &login_pkt, sizeof(Packet));

    // 스레드 시작
    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
    
    // [중요] 실시간 폭탄 카운트다운을 위한 스레드 실행
    pthread_create(&timer_thread, NULL, bomb_timer_thread, NULL);
    pthread_create(&expire_thread, NULL, check_expiration, NULL);

    pthread_join(snd_thread, NULL);
    pthread_join(rcv_thread, NULL);
    // timer_thread는 detach하거나 메인 종료 시 자동 종료되므로 join 안 해도 무방함
    
    close(sock);
    return 0;
}