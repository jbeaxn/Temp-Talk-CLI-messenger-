#include "common.h"
#include "minigame.h" 

// 외부 함수 참조
void add_client(int sock, char *id, char *role);
void remove_client(int sock);
void send_msg_to_room(Packet *pkt, int sender_sock);
void write_log(Packet *pkt);
void get_user_list(char *project_id, char *output_buf);
// [신규] 함수 선언 추가
void update_project_expiration(char *project_id, int days);

void make_server_filename(char *project_id, char *origin_name, char *out_name) {
    sprintf(out_name, "server_file_%s_%s", project_id, origin_name);
}

void *handle_client(void *arg) {
    int clnt_sock = *((int*)arg);
    int str_len = 0;
    Packet pkt;
    char current_upload_filename[256] = {0};

    while ((str_len = read(clnt_sock, &pkt, sizeof(Packet))) != 0) {
        if (pkt.type == MSG_LOGIN) {
            printf("[Login] ID: %s, Role: %s\n", pkt.project_id, pkt.role);
            add_client(clnt_sock, pkt.project_id, pkt.role);
        }
        else if (pkt.type == MSG_COMMAND) {
            if (strcmp(pkt.data, "/list") == 0) {
                char file_list_buf[BUF_SIZE] = "";
                sprintf(file_list_buf, "\n\n파일을 열려면 /open [파일명]을 입력하세요.\n현재 업로드된 파일 목록은 아래와 같습니다.\n\n< File in Project: %s >\n", pkt.project_id);

                DIR *dir;
                struct dirent *ent;
                char prefix[100];
                sprintf(prefix, "server_file_%s_", pkt.project_id);
                int found = 0;

                if ((dir = opendir(".")) != NULL) {
                    while ((ent = readdir(dir)) != NULL) {
                        if (strncmp(ent->d_name, prefix, strlen(prefix)) == 0) {
                            strcat(file_list_buf, "- ");
                            strcat(file_list_buf, ent->d_name + strlen(prefix));
                            strcat(file_list_buf, "\n");
                            found = 1;
                        }
                    }
                    closedir(dir);
                }
                
                if (!found) strcat(file_list_buf, "(No files uploaded yet)\n");

                Packet res_pkt;
                memset(&res_pkt, 0, sizeof(Packet));
                res_pkt.type = MSG_LIST_RES; 
                strcpy(res_pkt.data, file_list_buf);
                write(clnt_sock, &res_pkt, sizeof(Packet));
            }
            else if (strcmp(pkt.data, "/who") == 0) {
                char user_list[BUF_SIZE];
                get_user_list(pkt.project_id, user_list);
                Packet res_pkt;
                res_pkt.type = MSG_LIST_RES; 
                strcpy(res_pkt.data, user_list);
                write(clnt_sock, &res_pkt, sizeof(Packet));
            }
            else if (strcmp(pkt.data, "/game") == 0) {
                start_typing_game(pkt.role, pkt.project_id);
            }
        }
        else if (pkt.type == MSG_OPEN_REQ) {
            char server_fname[512];
            make_server_filename(pkt.project_id, pkt.data, server_fname);

            Packet res_pkt;
            memset(&res_pkt, 0, sizeof(Packet));
            res_pkt.type = MSG_OPEN_RES;
            strcpy(res_pkt.role, pkt.data); 

            FILE *fp = fopen(server_fname, "r");
            if (fp) {
                int n = fread(res_pkt.data, 1, BUF_SIZE - 1, fp);
                res_pkt.data[n] = '\0';
                fclose(fp);
            } else {
                strcpy(res_pkt.data, "(File not found or cannot open)");
            }
            write(clnt_sock, &res_pkt, sizeof(Packet));
        }
        else if (pkt.type == MSG_FILE_UPLOAD_START) {
            printf(ANSI_COLOR_BLUE "[FILE UPLOAD] Project: %s | User: %s | File: %s\n" ANSI_COLOR_RESET,
                   pkt.project_id, pkt.role, pkt.data);
            strcpy(current_upload_filename, pkt.data);
            send_msg_to_room(&pkt, clnt_sock);
            write_log(&pkt);
        }
        else if (pkt.type == MSG_FILE_DATA) {
            send_msg_to_room(&pkt, clnt_sock);
            if (strlen(current_upload_filename) > 0) {
                char server_fname[512];
                make_server_filename(pkt.project_id, current_upload_filename, server_fname);
                int fd = open(server_fname, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd != -1) {
                    write(fd, pkt.data, pkt.data_len);
                    close(fd);
                }
            }
            write_log(&pkt);
        }
        else if (pkt.type == MSG_CHAT || pkt.type == MSG_EXPIRE_SET) {
            int is_answer = 0;
            if (pkt.type == MSG_CHAT && !pkt.is_volatile) {
                is_answer = check_game_answer(pkt.data, pkt.role, pkt.project_id);
            }
            
            // [수정] 만료 설정 패킷이 들어오면 서버 상태 업데이트
            if (pkt.type == MSG_EXPIRE_SET) {
                update_project_expiration(pkt.project_id, pkt.timer_sec);
            }

            // 정답이 아니면 평소처럼 채팅 전송
            if (!is_answer) {
                send_msg_to_room(&pkt, clnt_sock);
                write_log(&pkt);
            }
        }
    }

    remove_client(clnt_sock);
    close(clnt_sock);
    return NULL;
}