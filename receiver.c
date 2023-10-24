
#include "receiver.h"   // receiver.h
#include "crc.h"
#include "receiverUtil.h"

void init_receiver(Receiver *receiver,
                   int id) {
    receiver->recv_id = id;
    receiver->input_framelist_head = NULL;

    receiver->arr_buffer = malloc(glb_senders_array_length * sizeof(Frame **));
    memset(receiver->arr_buffer, 0, glb_senders_array_length * sizeof(Frame **));
    for (int i = 0; i < glb_senders_array_length; ++i) {
        receiver->arr_buffer[i] = malloc(SLIDE_WINDOW_SIZE * sizeof(Frame *));
        memset(receiver->arr_buffer[i], 0, SLIDE_WINDOW_SIZE * sizeof(Frame *));
    }

    receiver->arr_Seq = malloc(glb_senders_array_length * sizeof(uint8_t));
    memset(receiver->arr_Seq, -1, glb_senders_array_length);
    receiver->arr_WS = malloc(glb_senders_array_length * sizeof(uint8_t));
    memset(receiver->arr_WS, -1, glb_senders_array_length);
}

Frame *buildAck(Receiver *receiver, Frame *inframe) {
    //构建ack
    Frame *outframe = (Frame *) malloc(sizeof(Frame));
    memset(outframe, 0, sizeof(Frame));
//        strcpy(outframe->data, outgoing_cmd->message);
    outframe->seq = ++receiver->arr_Seq[inframe->src];
    outframe->src = inframe->dest;
    outframe->dest = inframe->src;
    outframe->ack = inframe->seq;
    outframe->flag = ACK;
    outframe->checkSum = crc16((const uint8_t *) outframe, MAX_FRAME_SIZE - 2);
    return outframe;
}

void handle_incoming_msgs(Receiver *receiver,
                          LLnode **outgoing_frames_head_ptr) {
    //TODO: Suggested steps for handling incoming frames
    //    1) 将帧从发送方出队->input_framelist_head
    //    2) 将 char * 缓冲区转换为 Frame 数据类型
    //    3) 检查帧是否损坏
    //    4) 检查该帧是否是给该接收者的
    //    5) 为发送者/接收者对执行滑动窗口协议

    int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
    if (incoming_msgs_length > 0) {
        fprintf(stderr, "receiver处理消息数量：%d\n", incoming_msgs_length);
    }
    while (incoming_msgs_length > 0) {
        //从链接列表的前面弹出一个节点并更新计数
        LLnode *ll_inmsg_node = ll_pop_node(&receiver->input_framelist_head);
        incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        fprintf(stderr, "收到并弹出\n");

        //DUMMY CODE: Print the raw_char_buf
        //NOTE: 你不应该盲目地打印消息！
        //      Ask yourself: 这个消息真的是给我的吗？
        //                    此消息是否已损坏？
        //                    这是一条旧的、重新传输的消息吗？
        char *raw_char_buf = (char *) ll_inmsg_node->value;

        //校验和
        uint16_t checkSum = crc16(ll_inmsg_node->value, MAX_FRAME_SIZE);
        if (checkSum != 0) {
            fprintf(stderr, "receiver校验存在差错\n");
            free(raw_char_buf); //释放raw_char_buf
            free(ll_inmsg_node);
            continue;
        }

        Frame *inframe = convert_char_to_frame(raw_char_buf);
        free(raw_char_buf); //释放raw_char_buf

        fprintf(stderr, "收到消息%d：%s\n", inframe->seq, inframe->data);

        //帧目标接收为本接收者
        if (inframe->dest == receiver->recv_id) {
            fprintf(stderr, "receiver接收到属于消息\n");
            fprintf(stderr, "receiver接收消息序列号%d窗口%d\n", inframe->seq, receiver->arr_WS[inframe->src]);
            //判断为新消息，回复ack
            if ((receiver->arr_WS[inframe->src] < inframe->seq && inframe->seq <= receiver->arr_WS[inframe->src] + SLIDE_WINDOW_SIZE)
                || (inframe->seq < receiver->arr_WS[inframe->src] && inframe->seq + 256 <= receiver->arr_WS[inframe->src] + SLIDE_WINDOW_SIZE)) {
                //新消息添加到接收缓存
                int pos = (inframe->seq < receiver->arr_WS[inframe->src] ? inframe->seq + 256 : inframe->seq) - receiver->arr_WS[inframe->src] - 1;
                fprintf(stderr, "receiver缓存下标：%d\n", pos);
                receiver->arr_buffer[inframe->src][pos] = inframe;
                fprintf(stderr, "receiver缓存新消息：%s\n", inframe->data);
                //构建ack
                fprintf(stderr, "receiver构建回复：%hhu\n", inframe->seq);
                Frame *outframe = buildAck(receiver, inframe);
                fprintf(stderr, "receiver回复ack：%hhu\n", outframe->ack);

                //整合连续帧并输出消息
                if (receiver->arr_buffer[inframe->src][0] != NULL) {
                    mergeMessage(receiver, inframe->src);
                    //打印消息
//                    printf("<RECV-%d>:[%s]\n", receiver->recv_id, message);
                }

                //发送ack
                char *char_buf = convert_frame_to_char(outframe);
                free(outframe);
                ll_append_node(outgoing_frames_head_ptr, char_buf); //添加到发送队列
            }
            //旧消息，回复ack
            else {
                fprintf(stderr, "receiver收到旧消息：%s\n", inframe->data);
                //发送ack
                fprintf(stderr, "receiver构建回复：%hhu\n", inframe->seq);
                Frame *outframe = buildAck(receiver, inframe);  //构建ack
                fprintf(stderr, "receiver回复ack：%hhu\n", outframe->ack);
                char *char_buf = convert_frame_to_char(outframe);
                free(outframe);
                free(inframe);
                ll_append_node(outgoing_frames_head_ptr, char_buf); //添加到发送队列
            }
        }

        free(ll_inmsg_node);
    }
}

void *run_receiver(void *input_receiver) {
    struct timespec time_spec;
    struct timeval curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Receiver *receiver = (Receiver *) input_receiver;
    LLnode *outgoing_frames_head;


    //This incomplete receiver thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up if there is nothing in the incoming queue(s)
    //2. Grab the mutex protecting the input_msg queue
    //3. Dequeues messages from the input_msg queue and prints them
    //4. Releases the lock
    //5. Sends out any outgoing messages

    pthread_cond_init(&receiver->buffer_cv, NULL);
    pthread_mutex_init(&receiver->buffer_mutex, NULL);

    while (1) {
        //NOTE: 将传出消息添加到outgoing_frames_head指针
        outgoing_frames_head = NULL;
        gettimeofday(&curr_timeval,
                     NULL);

        //要么超时，要么因为收到一个数据报被唤醒
        //标记：您实际上不需要在这里做任何事情，但是让接收器定期唤醒并打印信息可能对调试有用
        time_spec.tv_sec = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;
        time_spec.tv_sec += WAIT_SEC_TIME;
        time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        if (time_spec.tv_nsec >= 1000000000) {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        //*****************************************************************************************
        //NOTE: 任何涉及到从输入帧中取出队列的操作都应该在互斥锁锁定和解锁之间进行，因为其他线程可以/将访问这些结构
        //*****************************************************************************************
        pthread_mutex_lock(&receiver->buffer_mutex);

        //检查是否有消息到达
        int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        if (incoming_msgs_length == 0) {
            //没有任何东西到达，对条件变量执行定时等待(这会释放互斥锁)。同样，您不需要执行定时等待。
            //条件变量上的信号将唤醒线程并重新获取锁
            pthread_cond_timedwait(&receiver->buffer_cv,
                                   &receiver->buffer_mutex,
                                   &time_spec);
        }

        handle_incoming_msgs(receiver,
                             &outgoing_frames_head);

        pthread_mutex_unlock(&receiver->buffer_mutex);

        //CHANGE THIS AT YOUR OWN RISK!
        //发送用户添加到outgoing_frames列表中的所有帧
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        while (ll_outgoing_frame_length > 0) {
            LLnode *ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char *char_buf = (char *) ll_outframe_node->value;

            fprintf(stderr, "receiver发送\n");
            //以下函数为char_buf对象释放内存
            send_msg_to_senders(char_buf);

            //释放ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);

}
