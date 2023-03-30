#ifndef PROTOCLO_H
#define PROTOCLO_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory>

#define LOGIN_SUCCEED "login succeed"
#define LOGIN_USER_ONLINE "user online"
#define LOGIN_NAMEORPASSWD_ERROR "name or passwd error"

enum PACK_TYPE {
    PACK_TYPE_MIN = 0,

    PACK_TYPE_MSG,                  // 消息包
    PACK_TYPE_FILE,                 // 文件包
};

struct FilePack {
    unsigned int pack_size;         // 文件包大小
    char data[];                    // 报数据
};

enum MSG_TYPE {
    MSG_TYPE_MIN = 0,

    MSG_TYPE_LOGIN_REQUEST,         // 登录请求
    MSG_TYPE_LOGIN_RESPOND,         // 登录回复
};

struct MessagePack {
    unsigned int pack_size;         // 消息包大小
    unsigned int msg_type;          // 消息包类型
    char common[64];                // 可以存放一些不大于64字节的消息： 比如用户名、密码之类
    char data[];                    // 大于64字节的消息放在这里
};

struct DataPack
{
    unsigned int total_size;        // 数据总大小
    unsigned int pack_type;         // 数据包类型
    char pack_data[];               // 数据包
};


std::unique_ptr<DataPack> makeDataPack(unsigned int pack_size) {
    unsigned int total_size = sizeof(DataPack) + pack_size;
    std::unique_ptr<DataPack> pack((DataPack*)malloc(total_size));
    if(pack == nullptr) {
        exit(EXIT_FAILURE);
    }
    memset(pack.get(), 0, total_size);
    pack->total_size = total_size;
    return pack;
}

std::unique_ptr<DataPack> makeDataPackMsg(unsigned int pack_size)
{
    std::unique_ptr<DataPack> pack = makeDataPack(pack_size + sizeof(MessagePack));
    if(pack == nullptr) {
        exit(EXIT_FAILURE);
    }
    pack->pack_type = PACK_TYPE_MSG;
    MessagePack* msg_page = (MessagePack*)pack->pack_data;
    msg_page->pack_size = pack_size;
    return pack;
}

std::unique_ptr<DataPack> makeDataPackFile(unsigned int pack_size)
{
    std::unique_ptr<DataPack> pack = makeDataPack(pack_size + sizeof(FilePack));
    if(pack == nullptr) {
        exit(EXIT_FAILURE);
    }
    pack->pack_type = PACK_TYPE_FILE;
    FilePack* file_pack = (FilePack*)pack->pack_data;
    file_pack->pack_size = pack_size;
    return pack;
}


/*
DataPack* makeDataPack(unsigned int pack_size);
DataPack* makeDataPackMsg(unsigned int pack_size);
DataPack* makeDataPackFile(unsigned int pack_size);

DataPack* makeDataPack(unsigned int pack_size) {
    unsigned int total_size = sizeof(DataPack) + pack_size;
    DataPack* pack = (DataPack*)malloc(total_size);
    if(pack == nullptr) {
        exit(EXIT_FAILURE);
    }
    memset(pack, 0, total_size);
    pack->total_size = total_size;
    return pack;
}



DataPack *makeDataPackMsg(unsigned int pack_size)
{
    DataPack* pack = makeDataPack(pack_size + sizeof(MessagePack));
    if(pack == nullptr) {
        exit(EXIT_FAILURE);
    }
    //memset(pack, 0, pack_size + sizeof(MessagePack));
    pack->pack_type = PACK_TYPE_MSG;
    MessagePack* msg_page = (MessagePack*)pack->pack_data;
    msg_page->pack_size = pack_size;
    return pack;
}

DataPack *makeDataPackFile(unsigned int pack_size)
{
    DataPack* pack = makeDataPack(pack_size + sizeof(FilePack));
    if(pack == nullptr) {
        exit(EXIT_FAILURE);
    }
    //memset(pack, 0, pack_size + sizeof(FilePack));
    pack->pack_type = PACK_TYPE_FILE;
    FilePack* file_page = (FilePack*)pack->pack_data;
    file_page->pack_size = pack_size;
    return pack;
}
*/
#endif // PROTOCLO_H
