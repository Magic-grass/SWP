#include "input.h"

//*********************************************************************
//NOTE: We will overwrite this file, so whatever changes you put here
//      WILL NOT persist
//*********************************************************************

/* getline implementation is copied from glibc. */

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif
#ifndef SSIZE_MAX
# define SSIZE_MAX ((ssize_t) (SIZE_MAX / 2))
#endif

ssize_t getline(char **lineptr, size_t *n, FILE *fp) {
    ssize_t result;
    size_t cur_len = 0;

    if (lineptr == NULL || n == NULL || fp == NULL) {
        return -1;
    }

    if (*lineptr == NULL || *n == 0) {
        *n = 120;
        *lineptr = (char *) malloc(*n);
        if (*lineptr == NULL) {
            result = -1;
            goto end;
        }
    }

    for (;;) {
        int i;

        i = getc(fp);
        if (i == EOF) {
            result = -1;
            break;
        }

        /* Make enough space for len+1 (for final NUL) bytes.  */
        if (cur_len + 1 >= *n) {
            size_t needed_max =
                    SSIZE_MAX < SIZE_MAX ? (size_t) SSIZE_MAX + 1 : SIZE_MAX;
            size_t needed = 2 * *n + 1;   /* Be generous. */
            char *new_lineptr;

            if (needed_max < needed)
                needed = needed_max;
            if (cur_len + 1 >= needed) {
                result = -1;
                goto end;
            }

            new_lineptr = (char *) realloc(*lineptr, needed);
            if (new_lineptr == NULL) {
                result = -1;
                goto end;
            }

            *lineptr = new_lineptr;
            *n = needed;
        }

        (*lineptr)[cur_len] = i;
        cur_len++;

        if (i == '\n')
            break;
    }
    (*lineptr)[cur_len] = '\0';
    result = cur_len ? (ssize_t) cur_len : result;

    end:
    return result;
}

void *run_stdinthread(void *threadid) {
    fd_set read_fds, master_fds;
    int fd_max;
    int select_res;
    int sender_id;
    int receiver_id;
    int sscanf_res;
    size_t input_buffer_size = (size_t) DEFAULT_INPUT_BUFFER_SIZE;
    char *input_buffer;
    char *input_message;
    int input_bytes_read;
    char input_command[MAX_COMMAND_LENGTH];
    Sender *sender;

    //将 fd_sets 归零
    FD_ZERO(&read_fds);
    FD_ZERO(&master_fds);

    //将标准输入添加到 fd_set
    FD_SET(STDIN_FILENO, &master_fds);
    fd_max = STDIN_FILENO;

    while (1) {
        //Copy the master set to avoid permanently altering the master set
        read_fds = master_fds;

        if ((select_res = select(fd_max + 1,
                                 &read_fds,
                                 NULL,
                                 NULL,
                                 NULL)) == -1) {
            perror("Select failed - exiting program");
            pthread_exit(NULL);
        }

        //用户输入一个字符串，读入输入并准备发送
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {

            input_buffer = (char *) malloc(input_buffer_size * sizeof(char));

            //NULL 设置整个输入缓冲区
            memset(input_buffer, 0, input_buffer_size * sizeof(char));

            //将命令行读入输入缓冲区
            input_bytes_read = getline(&input_buffer, &input_buffer_size, stdin);

            //将命令的读入缓冲区清零
            memset(input_command, 0, MAX_COMMAND_LENGTH * sizeof(char));

            //将内存清零以供消息通信
            input_message = (char *) malloc((input_bytes_read + 1) * sizeof(char));
            memset(input_message, 0, (input_bytes_read + 1) * sizeof(char));

            //扫描输入的参数
            sscanf_res = sscanf(input_buffer,
                                "%s %d %d %[^\n]",
                                input_command,
                                &sender_id,
                                &receiver_id,
                                input_message);

            //解析的对象数量少于预期
            if (sscanf_res < 4) {
                if (strcmp(input_command, "exit") == 0) {
                    free(input_message);
                    free(input_buffer);
                    return 0;
                } else {
                    fprintf(stderr, "Command is ill-formatted\n");
                }
            } else {
                if (strcmp(input_command, "msg") == 0) {
                    //检查以确保发送者和接收者 ID 在正确的范围内
                    if (sender_id >= glb_senders_array_length || sender_id < 0) {
                        fprintf(stderr, "Sender id is invalid\n");
                    }
                    if (receiver_id >= glb_receivers_array_length || receiver_id < 0) {
                        fprintf(stderr, "Receiver id is invalid\n");
                    }

                    //仅在有效时添加
                    if (sender_id < glb_senders_array_length &&
                        receiver_id < glb_receivers_array_length &&
                        sender_id >= 0 &&
                        receiver_id >= 0) {
                        //将消息添加到适当线程的接收缓冲区
                        Cmd *outgoing_cmd = (Cmd *) malloc(sizeof(Cmd));
                        char *outgoing_msg = (char *) malloc(sizeof(char) * (strlen(input_message) + 1));

                        //将输入消息复制到传出命令对象中
                        strcpy(outgoing_msg, input_message);

                        outgoing_cmd->src_id = sender_id;
                        outgoing_cmd->dst_id = receiver_id;
                        outgoing_cmd->message = outgoing_msg;

                        //将其添加到适当的输入缓冲区
                        sender = &glb_senders_array[sender_id];

                        //锁定缓冲区，添加到输入列表，并向线程发出信号
                        pthread_mutex_lock(&sender->buffer_mutex);
                        ll_append_node(&sender->input_cmdlist_head, (void *) outgoing_cmd);
                        pthread_cond_signal(&sender->buffer_cv);
                        pthread_mutex_unlock(&sender->buffer_mutex);
                    }
                } else {
                    fprintf(stderr, "Unknown command:%s\n", input_buffer);
                }
            }

            //最后，释放 input_buffer 和 input_message
            free(input_buffer);
            free(input_message);
        }
    }
    pthread_exit(NULL);
}
