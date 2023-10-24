//
// Created by 84358 on 2023/10/9.
//

#include "receiverUtil.h"
#include "string.h"


const char *mergeMessage(Receiver *receiver) {
    Frame **buffer = receiver->buffer;
    char *message = malloc(SLIDE_WINDOW_SIZE * FRAME_PAYLOAD_SIZE + 1);
    int index = 0;
    for (; index < SLIDE_WINDOW_SIZE; ++index) {
        if (buffer[index] == NULL) {
            break;
        }
        printf("<RECV-%d>:[%s]\n", receiver->recv_id, buffer[index]->data);
        strcat(message, buffer[index]->data);
        free(buffer[index]);
//        message += buffer[index]->data;
    }
    //缓存窗口后移
    if (index > 0) {
        memcpy(buffer, buffer + index, (SLIDE_WINDOW_SIZE - index) * sizeof(Frame *));
        memset(buffer + SLIDE_WINDOW_SIZE - index, 0, index * sizeof(Frame *));
        receiver->WS = (receiver->WS + index) % 256;
    }

    return message;
}
