#include "common.h"
#include "minigame.h" 

void add_client(int sock, char *id, char *role);
void remove_client(int sock);
void send_msg_to_room(Packet *pkt, int sender_sock);
void write_log(Packet *pkt);
void get_user_list(char *project_id, char *output_buf);
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
            printf(ANSI_COLOR_GREEN "\n  ✅ 로그인 - 프로젝트: %s | 사용자: %s\n\n" ANSI_COLOR_RESET, 
                   pkt.project_id, pkt.role);
            add_client(clnt_sock, pkt.project_id, pkt.role);
        }
        else if (pkt.type == MSG_COMMAND) {
            if (strcmp(pkt.data, "/list") == 0) {
                char file_list_buf[BUF_SIZE] = "";
                snprintf(file_list_buf, sizeof(file_list_buf), 
                         "\n✨━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━✨\n"
                         "          📂 프로젝트 '%s' 파일 목록\n"
                         "✨━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━✨\n\n", 
                         pkt.project_id);

                DIR *dir;
                struct dirent *ent;
                char prefix[100];
                snprintf(prefix, sizeof(prefix), "server_file_%s_", pkt.project_id);
                int found = 0;
                int count = 0;

                if ((dir = opendir(".")) != NULL) {
                    while ((ent = readdir(dir)) != NULL) {
                        if (strncmp(ent->d_name, prefix, strlen(prefix)) == 0) {
                            char temp[256];
                            snprintf(temp, sizeof(temp), "    %d. %s\n", 
                                    ++count, ent->d_name + strlen(prefix));
                            strcat(file_list_buf, temp);
                            found = 1;
                        }
                    }
                    closedir(dir);
                }
                
                if (!found) {
                    strcat(file_list_buf, "         (업로드된 파일이 없습니다)\n\n");
                }
                
                strcat(file_list_buf, "\n         💡 파일 열기: /open [파일명]\n");
                strcat(file_list_buf, "✨━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━✨\n");

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
                
                printf(ANSI_COLOR_CYAN "\n  📄 파일 열기 - '%s' (프로젝트: %s)\n\n" ANSI_COLOR_RESET,
                       pkt.data, pkt.project_id);
            } else {
                strcpy(res_pkt.data, "\n❌ 파일을 찾을 수 없거나 열 수 없습니다.\n");
                
                printf(ANSI_COLOR_RED "\n  ❌ 파일 열기 실패 - '%s' (프로젝트: %s)\n\n" ANSI_COLOR_RESET,
                       pkt.data, pkt.project_id);
            }
            write(clnt_sock, &res_pkt, sizeof(Packet));
        }
        else if (pkt.type == MSG_FILE_UPLOAD_START) {
            printf(ANSI_COLOR_BLUE "\n  📤 파일 업로드 시작 - 프로젝트: %s | 사용자: %s | 파일: %s\n\n" ANSI_COLOR_RESET,
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
            
            if (pkt.type == MSG_EXPIRE_SET) {
                update_project_expiration(pkt.project_id, pkt.timer_sec);
            }

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