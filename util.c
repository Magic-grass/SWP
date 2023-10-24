#include "util.h"

//Linked list functions
int ll_get_length(LLnode *head) {
    LLnode *tmp;
    int count = 1;
    if (head == NULL)
        return 0;
    else {
        tmp = head->next;
        while (tmp != head) {
            count++;
            tmp = tmp->next;
        }
        return count;
    }
}

void ll_append_node(LLnode **head_ptr,
                    void *value) {
    LLnode *prev_last_node;
    LLnode *new_node;
    LLnode *head;

    if (head_ptr == NULL) {
        return;
    }

    //Init the value pntr
    head = (*head_ptr);
    new_node = (LLnode *) malloc(sizeof(LLnode));
    new_node->value = value;

    //The list is empty, no node is currently present
    if (head == NULL) {
        (*head_ptr) = new_node;
        new_node->prev = new_node;
        new_node->next = new_node;
    } else {
        //Node exists by itself
        prev_last_node = head->prev;
        head->prev = new_node;
        prev_last_node->next = new_node;
        new_node->next = head;
        new_node->prev = prev_last_node;
    }
}

LLnode *ll_pop_node(LLnode **head_ptr) {
    LLnode *last_node;
    LLnode *new_head;
    LLnode *prev_head;

    prev_head = (*head_ptr);
    if (prev_head == NULL) {
        return NULL;
    }
    last_node = prev_head->prev;
    new_head = prev_head->next;

    //我们即将将 head ptr 设置为空，因为列表中只有一件事
    if (last_node == prev_head) {
        (*head_ptr) = NULL;
        prev_head->next = NULL;
        prev_head->prev = NULL;
        return prev_head;
    } else {
        (*head_ptr) = new_head;
        last_node->next = new_head;
        new_head->prev = last_node;

        prev_head->next = NULL;
        prev_head->prev = NULL;
        return prev_head;
    }
}

//移除节点
LLnode *ll_remove_node(LLnode **head, LLnode *node) {
    //待移除节点为空，则直接返回
    if (node == NULL) {
        return NULL;
    }

    LLnode *prev_head = *head;
    //待移除节点为头结点
    if (prev_head == node) {
        //仅有头节点一个节点
        if (prev_head->prev == prev_head) {
            *head = NULL;
            prev_head->next = NULL;
            prev_head->prev = NULL;
            return prev_head;
        }
        //多个结点，头节点设置为下一个结点
        else {
            *head = node->next;
        }
    }
    //更新待移除节点的前一个节点的 next 指针
    if (node->prev != NULL) {
        node->prev->next = node->next;
    }
    //更新待移除节点的后一个节点的 prev 指针
    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    node->next = NULL;
    node->prev = NULL;

    return node;
}

void ll_destroy_node(LLnode *node) {
    if (node->type == llt_string) {
        free((char *) node->value);
    }
    free(node);
}

//Compute the difference in usec for two time objects
long timeval_usecdiff(struct timeval *start_time,
                      struct timeval *finish_time) {
    long usec;
    usec = (finish_time->tv_sec - start_time->tv_sec) * 1000000;
    usec += (finish_time->tv_usec - start_time->tv_usec);
    return usec;
}


//Print out messages entered by the user
void print_cmd(Cmd *cmd) {
    fprintf(stderr, "src=%d, dst=%d, message=%s\n",
            cmd->src_id,
            cmd->dst_id,
            cmd->message);
}


char *convert_frame_to_char(Frame *frame) {
    //TODO: You should implement this as necessary
    char *char_buffer = (char *) malloc(MAX_FRAME_SIZE);
    memset(char_buffer, 0, MAX_FRAME_SIZE);
    memcpy(char_buffer, frame, MAX_FRAME_SIZE);
    return char_buffer;
}


Frame *convert_char_to_frame(char *char_buf) {
    //TODO: You should implement this as necessary
    Frame *frame = (Frame *) malloc(sizeof(Frame));
    memset(frame, 0, MAX_FRAME_SIZE);
    memcpy(frame, char_buf, MAX_FRAME_SIZE);
    return frame;
}

SendPair *buildSendPair(Frame *frame) {
    SendPair *pair = (SendPair *) malloc(sizeof(SendPair));
    pair->frame = frame;
    pair->time = NULL;
//    gettimeofday(&pair->time, NULL);   //初始化发送时间
    return pair;
}

void setTimeNow(struct timeval **timeval) {
    *timeval = malloc(sizeof(struct timeval));
    memset(*timeval, 0, sizeof(struct timeval));
    gettimeofday(*timeval, NULL);//初始化发送时间
}

int isString(char *str, size_t maxLength) {
    if (str != NULL && strnlen(str, maxLength) == maxLength - 1) {
        return 1;
    }
    return 0;
}


