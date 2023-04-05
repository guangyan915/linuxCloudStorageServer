#ifndef PROTOCLO_H
#define PROTOCLO_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory>

#define SYETEM_ERROR "system error"

#define LOGIN_SUCCEED "login succeed"
#define LOGIN_USER_ONLINE "user online"
#define LOGIN_NAMEORPASSWD_ERROR "name or passwd error"

#define REGISTER_USER_EXIST "name exist"
#define REGISTER_SUCCEED "register succeed"

//查找条件
#define FIIND_CRITERIA_ID "id"
#define FIIND_CRITERIA_NAME "name"

#define USER_NOT_EXIST "user not exist"
#define USER_EXIST "user exist"

#define ADD_FRIEND_REQUEST_SEND "add friend send"
#define ADD_FRIEND_REQUEST_SEND_ERROR "add friend send error"
#define IS_USER_FRIEND "is your friend"

struct UserSelfInfo {
    
};

class UserInfo{
public:
    unsigned int id;
    char name[32];
    char online;
    // 待拓展 
    
        
    UserInfo& operator= (const UserInfo& other) {
      id = other.id;
      strcpy(name, other.name);
      online = other.online;  
      return *this;
    }
};

enum PACK_TYPE {
    PACK_TYPE_MIN = 0,

    PACK_TYPE_MSG,                  // 消息包
    PACK_TYPE_FILE,                 // 文件包
};

struct FilePack {
    unsigned int data_size;         // data包大小
    char data[];                    // 报数据
};

enum MSG_TYPE {
    MSG_TYPE_MIN = 0,

    MSG_TYPE_LOGIN_REQUEST,         // 登录请求
    MSG_TYPE_LOGIN_RESPOND,         // 登录回复

    MSG_TYPE_REGISTER_REQUEST,    // 注册请求
    MSG_TYPE_REGISTER_RESPOND,      // 注册回复
    
    MSG_TYPE_FIND_USER_REQUEST,        // 查找用户请求
    MSG_TYPE_FIND_USER_RESPOND,        // 查找用户回复
    

    MSG_TYPE_ADD_FRIEND_REQUEST,        // 加好友请求
    MSG_TYPE_ADD_FRIEND_RESPOND,        // 加好友回复
};

struct MessagePack {
    unsigned int data_size;         // data大小
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



// 使用智能指针来管理

struct FreeDeleter {
  void operator()(void* ptr) const {
    std::free(ptr);
  }
};

std::unique_ptr<DataPack, FreeDeleter> makeDataPack(unsigned int pack_size) {
    unsigned int total_size = sizeof(DataPack) + pack_size;
    std::unique_ptr<DataPack, FreeDeleter> pack((DataPack*)std::malloc(total_size));
    if(pack == nullptr) {
        exit(EXIT_FAILURE);
    }
    memset(pack.get(), 0, total_size);
    pack->total_size = total_size;
    return pack;
}

std::unique_ptr<DataPack, FreeDeleter> makeDataPackMsg(unsigned int data_size)
{
    std::unique_ptr<DataPack, FreeDeleter> pack = makeDataPack(data_size + sizeof(MessagePack));
    if(pack == nullptr) {
        exit(EXIT_FAILURE);
    }
    pack->pack_type = PACK_TYPE_MSG;
    MessagePack* msg_page = (MessagePack*)pack->pack_data;
    msg_page->data_size = data_size;
    return pack;
}

std::unique_ptr<DataPack, FreeDeleter> makeDataPackFile(unsigned int data_size)
{
    std::unique_ptr<DataPack, FreeDeleter> pack = makeDataPack(data_size + sizeof(FilePack));
    if(pack == nullptr) {
        exit(EXIT_FAILURE);
    }
    pack->pack_type = PACK_TYPE_FILE;
    FilePack* file_pack = (FilePack*)pack->pack_data;
    file_pack->data_size = data_size;
    return pack;
}

#endif // PROTOCLO_H
