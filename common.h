#ifndef COMMON_H_
#define COMMON_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <math.h>
#include <sys/time.h>

#define MAX_COMMAND_LENGTH 16
#define AUTOMATED_FILENAME 512

#define MAX_FRAME_SIZE 64

#define SLIDE_WINDOW_SIZE 4

//TODO: You should change this!
//Remember, your frame can be AT MOST 32 bytes!
#define FRAME_PAYLOAD_SIZE 56
#define FRAME_HEAD_SIZE 8

#define ACK 1
typedef unsigned char uchar_t;

//System configuration information
struct SysConfig_t {
    float drop_prob;
    float corrupt_prob;
    unsigned char automated;
    char automated_file[AUTOMATED_FILENAME];
};
typedef struct SysConfig_t SysConfig;

//Command line input information
struct Cmd_t {
    uint16_t src_id;
    uint16_t dst_id;
    char *message;
};
typedef struct Cmd_t Cmd;

//Linked list information
static enum LLtype {
    llt_string,
    llt_frame,
    llt_integer,
    llt_head
} LLtype;

struct LLnode_t {
    struct LLnode_t *prev;
    struct LLnode_t *next;
    enum LLtype type;

    void *value;
};
typedef struct LLnode_t LLnode;

struct Frame_t {
    uint8_t src;    //源ID
    uint8_t dest;   //目的ID
    uint8_t seq;    //序列号
    uint8_t ack;    //确认号
    uint8_t flag;   //标志
    uint8_t windowSize; //窗口大小
    char data[FRAME_PAYLOAD_SIZE];  //负载
    uint16_t checkSum;  //CRC16校验和
};
typedef struct Frame_t Frame;


//Receiver and sender data structures
struct Receiver_t {
    //不要改变以下变量
    pthread_mutex_t buffer_mutex;
    pthread_cond_t buffer_cv;
    LLnode *input_framelist_head;
    int recv_id;

    Frame **buffer; //缓存，直接存储Frame类型

    uint8_t Seq;//帧编号

    uint8_t WS; //窗口起始
};

struct Sender_t {
    //不要改变以下变量
    pthread_mutex_t buffer_mutex;
    pthread_cond_t buffer_cv;
    LLnode *input_cmdlist_head;
    LLnode *input_framelist_head;
    int send_id;

    LLnode *buffer; //缓存，直接存储SendPair类型

    uint8_t Seq;//帧编号

    uint8_t WS; //窗口起始
    uint8_t WE; //窗口终止
    uint8_t LAR;//最近收到的确认帧
    uint8_t LFS;//最近发送的帧
};

static enum SendFrame_DstType {
    ReceiverDst,
    SenderDst
} SendFrame_DstType;

typedef struct Sender_t Sender;
typedef struct Receiver_t Receiver;

typedef struct {
    Frame *frame;
    struct timeval *time;
} SendPair;

//Declare global variables here
//DO NOT CHANGE: 
//   1) glb_senders_array
//   2) glb_receivers_array
//   3) glb_senders_array_length
//   4) glb_receivers_array_length
//   5) glb_sysconfig
//   6) CORRUPTION_BITS
extern Sender *glb_senders_array;  //发送者数组
extern Receiver *glb_receivers_array;  //接收者数组
extern int glb_senders_array_length;   //发送者数组长度
extern int glb_receivers_array_length; //接收者数组长度
extern SysConfig glb_sysconfig;
extern int CORRUPTION_BITS;
#endif
