#include "sender.h"   // sender.h
#include "crc.h"

void init_sender(Sender *sender, int id) {
    //TODO: 完善这个方法
    sender->send_id = id;
    sender->input_cmdlist_head = NULL;
    sender->input_framelist_head = NULL;
    sender->buffer = NULL;

    sender->Seq = -1;
    sender->WS = -1;
    sender->LAR = -1;
    sender->LFS = -1;
}

struct timeval *sender_get_next_expiring_timeval(Sender *sender) {
    //You should fill in this function so that it returns the next timeout that should occur
    //TODO: 返回下一个应该发生的超时
    return NULL;
}


void handle_incoming_acks(Sender *sender, LLnode **outgoing_frames_head_ptr) {
    //TODO: Suggested steps for handling incoming ACKs
    //    1) 从发送方取消ACK队列-> input_framelist_head
    //    2) 将char *缓冲区转换为Frame数据类型
    //    3) 检查帧是否损坏
    //    4) 检查该帧是否属于该发送方
    //    5) 为发送方/接收方对做滑动窗口协议
    int length = ll_get_length(sender->input_framelist_head);
    if (length > 0) {
        fprintf(stderr, "sender处理消息数量：%d\n", length);
    }
    while (length > 0) {
        //取帧
        LLnode *node = ll_pop_node(&sender->input_framelist_head);
        length = ll_get_length(sender->input_framelist_head);

        //检查帧
        uint16_t checkSum = crc16(node->value, MAX_FRAME_SIZE);//校验和
        if (checkSum != 0) {
            fprintf(stderr, "sender校验存在差错\n");
            free(node->value);
            continue;
        }

        Frame *frame = convert_char_to_frame(node->value);
        //目的地址为发送者地址且为确认帧
        if (frame->dest == sender->send_id && (frame->flag & ACK) == ACK) {
            fprintf(stderr, "sender收到ack：%d\n", frame->ack);
            fprintf(stderr, "sender WS：%d Seq：%d\n", sender->WS, sender->Seq);
            //确认帧在合理范围
            if ((sender->WS <= sender->Seq && (sender->WS < frame->ack && frame->ack <= sender->Seq))
                || (sender->WS > sender->Seq && (sender->WS < frame->ack || frame->ack <= sender->Seq))) {
                fprintf(stderr, "sender收到合理ack：%d\n", frame->ack);
                LLnode *node_buffer = sender->buffer;
                //遍历缓存找到序列号等于ack的并移除
                do {
                    fprintf(stderr, "sender遍历缓存seq：%d\n", ((SendPair *) node_buffer->value)->frame->seq);
                    if (((SendPair *) node_buffer->value)->frame->seq == frame->ack) {
                        fprintf(stderr, "sender根据ack：%d移除缓存seq\n", frame->ack);
                        ll_remove_node(&sender->buffer, node_buffer);   //已确认的帧出列buffer
                        //更新窗口起始位置
                        sender->WS = ll_get_length(sender->buffer) == 0 ? sender->Seq :
                                     ((SendPair *) sender->buffer->value)->frame->seq - 1;
                        free(((SendPair *) node_buffer->value)->frame);
                        free(node_buffer->value);
                        free(node_buffer);
                        break;
                    }
                    node_buffer = node_buffer->next;
                } while (node_buffer != sender->buffer);
            }
        }

        free(node->value);
        free(node);
        free(frame);
    }
}


void handle_input_cmds(Sender *sender, LLnode **outgoing_frames_head_ptr) {
    //TODO: Suggested steps for handling input cmd
    //    1) 从sender->input_cmdlist_head取消Cmd队列
    //    2) 转换为帧
    //    3) 根据滑动窗口协议设置帧
    //    4) 计算CRC并将CRC添加到帧中

    int input_cmd_length = ll_get_length(sender->input_cmdlist_head);

    //重新检查命令队列长度，看看stdin_thread是否向我们转储了一个命令
    input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    while (input_cmd_length > 0) {
        //序列号不在窗口范围内
        if (!((sender->WS <= sender->Seq && (sender->Seq - sender->WS) < SLIDE_WINDOW_SIZE)
              || (sender->WS > sender->Seq && (sender->Seq + 256 - sender->WS) < SLIDE_WINDOW_SIZE))) {
            fprintf(stderr, "序列号%d不在窗口%d范围内\n", sender->Seq, sender->WS);
            break;
        }

        //弹出一个node并更新input_cmd_length
        LLnode *ll_input_cmd_node = ll_pop_node(&sender->input_cmdlist_head);
        input_cmd_length = ll_get_length(sender->input_cmdlist_head);

        //转换为Cmd类型并释放节点
        Cmd *outgoing_cmd = (Cmd *) ll_input_cmd_node->value;
        free(ll_input_cmd_node);


        //DUMMY CODE: 将原始char buf添加到outgoing_frames列表中
        //NOTE: 不应盲目地发出这条信息!
        //      扪心自问: 这条消息是否真的发送给了正确的接收者(回想一下，send的默认行为是向所有接收者广播)?
        //                    接收方的输入队列中是否有足够的空间来处理此消息?
        //                    之前发送给这个接收者的消息是否真的传递给了接收者?
        size_t msg_length = strlen(outgoing_cmd->message);

        //处理超出负载大小
        size_t offset = 0;
        size_t maxStrLen = FRAME_PAYLOAD_SIZE - 1;
        for (int i = 0; msg_length > maxStrLen; ++i) {
            Frame *outgoing_frame = buildOutgoingFrame(sender, outgoing_cmd);
            strncpy(outgoing_frame->data, outgoing_cmd->message + offset, maxStrLen);
            outgoing_frame->checkSum = crc16((const uint8_t *) outgoing_frame, MAX_FRAME_SIZE - 2);

            fprintf(stderr, "sender封装帧：%d\n", outgoing_frame->seq);
            ll_append_node(&sender->buffer, buildSendPair(outgoing_frame)); //将pair添加到缓存队列

            offset += maxStrLen;
            msg_length -= maxStrLen;
        }
        if (msg_length > 0) {
            Frame *outgoing_frame = buildOutgoingFrame(sender, outgoing_cmd);
            strncpy(outgoing_frame->data, outgoing_cmd->message + offset, msg_length);
            outgoing_frame->checkSum = crc16((const uint8_t *) outgoing_frame, MAX_FRAME_SIZE - 2);

            fprintf(stderr, "sender封装帧：%d\n", outgoing_frame->seq);
            ll_append_node(&sender->buffer, buildSendPair(outgoing_frame)); //将pair添加到缓存队列

//            char *outgoing_charbuf = convert_frame_to_char(outgoing_frame); //将消息转换为charbuf
//            ll_append_node(outgoing_frames_head_ptr, outgoing_charbuf); //将charbuf添加到发送队列
        }
        free(outgoing_cmd->message);
        free(outgoing_cmd);
    }
}


void handle_timedout_frames(Sender *sender, LLnode **outgoing_frames_head_ptr) {
    //TODO: 处理超时数据报的建议步骤
    //    1) 迭代您为每个接收者维护的滑动窗口协议信息
    //    2) 找到超时的帧并将其添加到传出帧中
    //    3) 更新传出帧的下一个超时字段
    struct timeval curtime;
    gettimeofday(&curtime, NULL);   //当前时间
    LLnode *node_buffer = sender->buffer;
    if (node_buffer == NULL) {
        return;
    }
    //遍历发送缓存
    do {
        struct timeval *time;
        SendPair *pair = ((SendPair *) node_buffer->value);
        time = pair->time;
        long interval = (curtime.tv_sec - time->tv_sec) * 1000000 + (curtime.tv_usec - time->tv_usec);    //时间间隔（微妙）
        //超时0.05秒重发帧
        if (interval > 50000) {
            fprintf(stderr, "sender处理超时%ld消息：%s\n", interval, pair->frame->data);
            pair->time->tv_sec = curtime.tv_sec;
            pair->time->tv_usec = curtime.tv_usec;
            char *outgoing_charbuf = convert_frame_to_char(pair->frame);//将消息转换为charbuf
            ll_append_node(outgoing_frames_head_ptr, outgoing_charbuf); //将charbuf添加到发送队列
        }
        node_buffer = node_buffer->next;
    } while (node_buffer != NULL && node_buffer != sender->buffer);
}

Frame *buildOutgoingFrame(Sender *sender, Cmd *outgoing_cmd) {
    Frame *outgoing_frame = (Frame *) malloc(sizeof(Frame));
    memset(outgoing_frame, 0, sizeof(Frame));
    outgoing_frame->seq = ++sender->Seq;
    outgoing_frame->src = sender->send_id;
    outgoing_frame->dest = outgoing_cmd->dst_id;
    return outgoing_frame;
}

void handle_pending_frames(Sender *sender, LLnode **outgoing_frames_head_ptr) {
    struct timeval curtime;
    gettimeofday(&curtime, NULL);   //当前时间
    LLnode *node_buffer = sender->buffer;
    if (node_buffer == NULL) {
        return;
    }
    int count = SLIDE_WINDOW_SIZE;
    //遍历发送缓存
    do {
        SendPair *pair = ((SendPair *) node_buffer->value);
        //尚未发送
        if (pair->time == NULL) {
            setTimeNow(&pair->time); //初始化发送时间
            fprintf(stderr, "初始化发送时间：%ld|%ld\n", pair->time->tv_sec, pair->time->tv_usec);
            char *outgoing_charbuf = convert_frame_to_char(pair->frame);//将消息转换为charbuf
            ll_append_node(outgoing_frames_head_ptr, outgoing_charbuf); //将charbuf添加到发送队列
        }
        //已发送(超时/等待ACK)
        else {
            struct timeval *time;
            time = pair->time;
            long interval = (curtime.tv_sec - time->tv_sec) * 1000000 + (curtime.tv_usec - time->tv_usec);    //时间间隔（微妙）
            //超时0.05秒重发帧
            if (interval > 50000) {
                fprintf(stderr, "sender处理超时%ld消息：%s\n", interval, pair->frame->data);
                pair->time->tv_sec = curtime.tv_sec;
                pair->time->tv_usec = curtime.tv_usec;
                char *outgoing_charbuf = convert_frame_to_char(pair->frame);//将消息转换为charbuf
                ll_append_node(outgoing_frames_head_ptr, outgoing_charbuf); //将charbuf添加到发送队列
            }
        }
        node_buffer = node_buffer->next;
        count--;
    } while (node_buffer != NULL && node_buffer != sender->buffer && count > 0);

}


void *run_sender(void *input_sender) {
    struct timespec time_spec;
    struct timeval curr_timeval;
    const int WAIT_SEC_TIME = 0;
//    const long WAIT_USEC_TIME = 100000;
    const long WAIT_USEC_TIME = 50000;
    Sender *sender = (Sender *) input_sender;
    LLnode *outgoing_frames_head;
    struct timeval *expiring_timeval;
    long sleep_usec_time, sleep_sec_time;

    //这个不完整的发送者线程在高层循环如下：
    //1. 确定线程下次应该唤醒的时间
    //2. 获取保护 input_cmd/inframe 队列的mutex
    //3. 将消息从输入队列中出列并将它们添加到 outgoing_frames 列表中
    //4. 释放锁
    //5. 发送消息

    pthread_cond_init(&sender->buffer_cv, NULL);
    pthread_mutex_init(&sender->buffer_mutex, NULL);

    while (1) {
        outgoing_frames_head = NULL;

        //获取当前时间
        gettimeofday(&curr_timeval,
                     NULL);

        //time_spec 是一个数据结构，用于指定线程何时应该唤醒
        //时间被指定为绝对时间
        time_spec.tv_sec = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;

        //检查我们应该处理的下一个事件
        expiring_timeval = sender_get_next_expiring_timeval(sender);

        //超时时执行完整操作
        if (expiring_timeval == NULL) {
            time_spec.tv_sec += WAIT_SEC_TIME;
            time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        } //
        else {
            //获取下一个事件与当前时间之间的差异
            sleep_usec_time = timeval_usecdiff(&curr_timeval,
                                               expiring_timeval);

            //如果差异为正，则睡眠
            if (sleep_usec_time > 0) {
                sleep_sec_time = sleep_usec_time / 1000000;
                sleep_usec_time = sleep_usec_time % 1000000;
                time_spec.tv_sec += sleep_sec_time;
                time_spec.tv_nsec += sleep_usec_time * 1000;
            }
        }

        //检查以确保我们没有“溢出”纳秒字段
        if (time_spec.tv_nsec >= 1000000000) {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }


        //*****************************************************************************************
        //NOTE: 任何涉及从输入帧或输入命令提取队列的内容都应该删除，因为其他线程可以/将会访问这些结构
        //*****************************************************************************************
        pthread_mutex_lock(&sender->buffer_mutex);

        //检查是否有待处理的
        int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
        int inframe_queue_length = ll_get_length(sender->input_framelist_head);

        //没有任何内容（cmd 或传入帧）到达，因此对发送者的条件变量进行定时等待（释放锁定）
        //条件变量上的信号将唤醒线程并获取锁
        if (input_cmd_length == 0 && inframe_queue_length == 0) {
            pthread_cond_timedwait(&sender->buffer_cv,
                                   &sender->buffer_mutex,
                                   &time_spec);
        }

        //处理接收ack
        handle_incoming_acks(sender, &outgoing_frames_head);
        //处理输入cmd
        handle_input_cmds(sender, &outgoing_frames_head);

        //处理超时帧
//        handle_timedout_frames(sender, &outgoing_frames_head);
        //处理待发送帧
        handle_pending_frames(sender, &outgoing_frames_head);

        pthread_mutex_unlock(&sender->buffer_mutex);

        //CHANGE THIS AT YOUR OWN RISK!
        //发送所有帧
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);

        while (ll_outgoing_frame_length > 0) {
            LLnode *ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char *char_buf = (char *) ll_outframe_node->value;

            fprintf(stderr, "sender发送\n");
            //不用担心释放 char_buf，下面的函数可以做到这一点
            send_msg_to_receivers(char_buf);

            //释放ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);
    return 0;
}
