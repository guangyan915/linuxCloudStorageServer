#include <netinet/in.h> // sockaddr_in
#include <sys/types.h>  // socket
#include <sys/socket.h> // socket
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h> // epoll
#include <sys/ioctl.h>
#include <sys/time.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include "ThreadPool.hpp"
#include "Protocol.hpp"
#include "DBOperator.hpp"


#define CONFIG_FILE_PATH "./configFile/server.config"

#define MAX_EVENTS 1024


class CloudStorageServer
{
private:
    int listen_fd;                            // 监听的fd
    int listen_backlog;                       // listen队列大小
    std::string server_port;                  // 绑定的端口
    int epoll_fd;                             // epoll fd
    mutex mtx;                                // 互斥锁
    map<int, std::pair<int, std::string>> login_client;     // 记录登录的用户，以fd作为键，登录类型，和登录名作为值
public:
    CloudStorageServer();
    ~CloudStorageServer();
private:
    void Socket();
    void Setsockopt();
    void Bind();
    void Listen();
    void EpollCreate();
    void Accept();
public:
    void Run();
    void handleConnetClose(int fd);
    static CloudStorageServer& getInstance();
    static void recvMsg(int fd);

public:
    void handleLoginRequest(MessagePack* msg_pack, int fd);
    void handleRegisterRequest(MessagePack* msg_pack, int fd);
    void handleFindUserRequest(MessagePack* msg_pack, int fd);  
private:
    int toLoadConfig();
    void setConfigValue(std::string s);
    
    void handleMsgPack(DataPack* pack, int fd);
    void handleFilePack(DataPack* pack, int fd);
};


void CloudStorageServer::setConfigValue(std::string s) {  
  std::string key;
  std::string value;
  int i = 0;
  for(; i < s.size(); i++) {
    if(s[i] == ':') break;
    key += s[i];
  }
  i++;
  while(i < s.size() && s[i] == ' ') i++;
  for(; i < s.size(); i++) {
    value += s[i];
  }
  if(key == "server_port") {
    server_port = value;
  }
  if(key == "listen_backlog")  {
    listen_backlog = atoi(value.c_str());
  }
}


int CloudStorageServer::toLoadConfig() {
  std::fstream file(CONFIG_FILE_PATH);
  if(file.is_open()) {
    std::string line;
    while (getline(file, line)) {
      setConfigValue(line);
    }
  }
  else {
    std::cout << "配置文件server.config打开失败，请检查本程序当前在路径目录是否有configFile,配置文件应该在其中！";
    return -1;
    exit(-1);
  }
  file.close();
  return 0;
}

CloudStorageServer::CloudStorageServer()
{
  toLoadConfig();
  sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));  
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htons(INADDR_ANY);  
  server_addr.sin_port = htons(atoi(server_port.c_str()));
    
  listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) 
  {
    perror("Create Socket Failed!");
    exit(1); 
  }
  // 端口复用
  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

CloudStorageServer::~CloudStorageServer()
{
  close(listen_fd);
  close(epoll_fd);
}

void CloudStorageServer::Socket() {
  // 创建监听套接字
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }
}

void CloudStorageServer::Setsockopt() {
	// 端口复用   
	// 设置 SO_REUSEADDR 选项
	int reuse = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
		std::cerr << "Failed to set SO_REUSEADDR" << std::endl;
    close(listen_fd);
    return exit(EXIT_FAILURE);
  }
}

void CloudStorageServer::Bind()
{
  // 绑定地址和端口号
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(atoi(server_port.c_str()));
  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    perror("Failed to bind socket");
    exit(EXIT_FAILURE);  
  }
}

void CloudStorageServer::Listen()
{
  // 监听连接
  if (listen(listen_fd, SOMAXCONN) == -1) {
    perror("Failed to listen on socket");
    exit(EXIT_FAILURE);  
  }
}

void CloudStorageServer::EpollCreate() {
  // 创建 epoll 文件描述符
  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {    
    perror("Failed to create epoll file descriptor");    
    exit(EXIT_FAILURE);  
  }
  // 添加监听套接字到 epoll 中  
  struct epoll_event event;  
  event.data.fd = listen_fd;
  event.events = EPOLLIN;  
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {    
    perror("Failed to add listen socket to epoll");    
    exit(EXIT_FAILURE);
  }
}

void CloudStorageServer::Accept()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0)
    {
        perror("Failed to accept on socket");
        close(client_fd);
        exit(1);
    }

    std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr) \
       << ":" << ntohs(client_addr.sin_port) << std::endl;

    // 在epoll_fd中注册新建立的连接
    struct epoll_event event{};
    event.data.fd = client_fd;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
      std::cerr << "Failed to add client_fd to epoll\n";
      close(client_fd);  
    }
}

void CloudStorageServer::Run()
{
  Socket();
	Setsockopt();
  Bind();
  Listen();
  EpollCreate();
  // 创建线程池
  // 最小线程数量，最大线程数量， 任务队列最大长度， 线程空闲时间：ms
  //ThreadPool thread_poll(4, 12, 100, 5000);
  // 处理事件循环
  std::vector<epoll_event> events(MAX_EVENTS);
  while (true) {    
    // 等待事件
    int nfds = epoll_wait(epoll_fd, events.data(), events.size(), -1);
    if (nfds < 0) {
      std::cerr << "Failed to wait for events\n";
      break;
    }
    // 处理事件
    for (int i = 0; i < nfds; i++) {
      auto fd = events[i].data.fd;
      if(fd == listen_fd && events[i].events & EPOLLIN) {
        // 处理新连接
        Accept();
      }
      else {
        // 处理客户端事件        
        std::cout << fd << ":客户端发送消息\n";
        if(events[i].events & EPOLLIN) {
          //Task task(recvMsg, fd);
          //thread_poll.AddTask(task);
          recvMsg(fd);
        }
        //else 
      }


    }

  }
}

void CloudStorageServer::handleConnetClose(int fd) {
  // 关掉描述符
  close(fd);
  // 将该fd也从epoll中注销
  struct epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN;
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
}

CloudStorageServer& CloudStorageServer::getInstance() {
  static CloudStorageServer instance;
  return instance;
}

void CloudStorageServer::recvMsg(int fd)
{
  // 获取包大小
  unsigned int total_size = 0;
  int ret = recv(fd, &total_size, sizeof(total_size), 0);
  if(ret == 0) {
    // 连接关闭
    std::cout << "有客户端退出!\n";
    CloudStorageServer::getInstance().handleConnetClose(fd);
    return;
  }
  else if(ret < 0) {
    // recv出错
    CloudStorageServer::getInstance().handleConnetClose(fd);
    perror("recv error!\n");
    return;
  }

  // 获取包
  std::unique_ptr<DataPack, FreeDeleter> pack = makeDataPack(total_size - sizeof(DataPack));
  std::cout << "包大小：" << total_size << std::endl;
  // 需要偏移记录包大小的变量字节数,也要少读这字节数
  ret = recv(fd, (char*)pack.get() + sizeof(total_size), total_size - sizeof(total_size), 0);
  if(ret == 0) {
    // 连接关闭
    std::cout << "有客户端退出!\n";
    CloudStorageServer::getInstance().handleConnetClose(fd);
    return;
  }
  else if(ret < 0) {
    // recv出错
    CloudStorageServer::getInstance().handleConnetClose(fd);
    perror("recv error!\n");
    return;
  }
  if(pack->pack_type == PACK_TYPE_MSG) {
    CloudStorageServer::getInstance().handleMsgPack(pack.get(), fd);
  }
  else if(pack->pack_type == PACK_TYPE_FILE){  
    CloudStorageServer::getInstance().handleFilePack(pack.get(), fd);
  }

  return;
}


void CloudStorageServer::handleMsgPack(DataPack* pack, int fd) {
  MessagePack* msg_pack = (MessagePack*)pack->pack_data;
  switch(msg_pack->msg_type) {
    case MSG_TYPE_LOGIN_REQUEST :
      handleLoginRequest(msg_pack, fd);
      break;
    case MSG_TYPE_REGISTER_REQUEST :
      handleRegisterRequest(msg_pack, fd);
    case MSG_TYPE_FIND_USER_REQUEST :
      handleFindUserRequest(msg_pack, fd);
    default :
      break;
  }
}

void CloudStorageServer::handleLoginRequest(MessagePack* msg_pack, int fd) {
  std::string login_name = std::string(msg_pack->common); 
  std::string passwd = std::string(msg_pack->common + 32);
  std::unique_ptr<DataPack, FreeDeleter> pack = makeDataPackMsg(0);
  pack->pack_type = PACK_TYPE_MSG;
  MessagePack* msg = (MessagePack*)pack->pack_data;
  msg->msg_type = MSG_TYPE_LOGIN_RESPOND;
  int ret = DBOperator::getInstance().loginIsSucceed(login_name, passwd);
  if(ret == 0) {
    // 不在线,可以登录
    strcpy((char*)msg->common, LOGIN_SUCCEED);
  }
  else if(ret == 1) {
    // 在线，暂时不能登录
    strcpy((char*)msg->common, LOGIN_USER_ONLINE);
  }
  else {
    // 账号或密码错误
    strcpy((char*)msg->common, LOGIN_NAMEORPASSWD_ERROR);
  }
  
  ret = send(fd, pack.get(), pack->total_size, 0);
  if(ret <= 0) {
    handleConnetClose(fd);
    perror("send error\n");
  }
  else {
    std::cout << "send："  << ret << "byte"  << std::endl;
  }
}


void CloudStorageServer::handleRegisterRequest(MessagePack* msg_pack, int fd) {
  std::string login_name = std::string(msg_pack->common); 
  std::string passwd = std::string(msg_pack->common + 32);
  std::unique_ptr<DataPack, FreeDeleter> pack = makeDataPackMsg(0);
  pack->pack_type = PACK_TYPE_MSG;
  MessagePack* msg = (MessagePack*)pack->pack_data;
  msg->msg_type = MSG_TYPE_REGISTER_RESPOND;
  if(DBOperator::getInstance().userNameIsExist(login_name)) {
    // 用户名存在
    strcpy(msg->common, REGISTER_USER_EXIST);
  }
  else {
    // 用户名不存在
    strcpy(msg->common, REGISTER_SUCCEED);
    if(!DBOperator::getInstance().insertUser(login_name, passwd)) {
      strcpy(msg->common, SYETEM_ERROR);
    }
  }
  if(send(fd, pack.get(), pack->total_size, 0) <= 0) {
    handleConnetClose(fd);
  }
}

void CloudStorageServer::handleFindUserRequest(MessagePack* msg_pack, int fd) {
  std::string find_criteria(msg_pack->common);
  std::string user(msg_pack->common + 32);
  
  //std::cout << "find:" << find_criteria << std::endl;
  //std::cout << "user:" << user << std::endl;
  // 按用户名查找
  if(strcmp(find_criteria.c_str(), FIIND_CRITERIA_NAME) == 0) {
    // 用户不存在
    if(!DBOperator::getInstance().userNameIsExist(user)) {
      std::unique_ptr<DataPack, FreeDeleter> pack = makeDataPackMsg(0);
      MessagePack* msg = (MessagePack*)pack->pack_data;
      msg->msg_type = MSG_TYPE_FIND_USER_RESPOND;
      strcpy(msg->common, USER_NOT_EXIST);
      if(send(fd, pack.get(), pack->total_size, 0) <= 0) {
        handleConnetClose(fd);
      }
    }
    // 用户存在
    else {
      std::vector<UserInfo> user_info;
      auto ret = DBOperator::getInstance().getUserInfo(find_criteria, user);
      for(auto e : ret) {
        UserInfo temp;
        temp.id = atoi(e[0].c_str());
        strcpy(temp.name, e[1].c_str());
        temp.online = e[3][0];
        user_info.push_back(temp);
      }
      
      //std::cout << "id:"<< ret[0][0] << endl;
      //std::cout << "name:"<< ret[0][1] << endl;
      //std::cout << "online:"<< ret[0][3] << endl;
      std::unique_ptr<DataPack,FreeDeleter> pack = makeDataPackMsg(user_info.size() * sizeof(UserInfo));
      MessagePack* msg = (MessagePack*)pack->pack_data;
      msg->msg_type = MSG_TYPE_FIND_USER_RESPOND;
      strcpy(msg->common, USER_EXIST);
      for(int i = 0; i < user_info.size(); i++) {
        memcpy(msg->data + i * sizeof(UserInfo), &user_info[i], sizeof(UserInfo));
      }
      if(send(fd, pack.get(), pack->total_size, 0) <= 0) {
        handleConnetClose(fd);
      }

    }
  }
  // 按id查找
  else if(strcmp(find_criteria.c_str(), FIIND_CRITERIA_ID) == 0) {
    // 用户不存在
    if(!DBOperator::getInstance().userIdIsExist(user)) {
      std::unique_ptr<DataPack, FreeDeleter> pack = makeDataPackMsg(0);
      MessagePack* msg = (MessagePack*)pack->pack_data;
      msg->msg_type = MSG_TYPE_FIND_USER_RESPOND;
      strcpy(msg->common, USER_NOT_EXIST);
      if(send(fd, pack.get(), pack->total_size, 0) <= 0) {
        handleConnetClose(fd);
      }
    }
    else {
      // 用户存在
      std::vector<UserInfo> user_info;
      std::cout << "id exist!" << std::endl;
      auto ret = DBOperator::getInstance().getUserInfo(find_criteria, user);
      //std::cout << "id:"<< ret[0][0] << endl;
      //std::cout << "name:"<< ret[0][1] << endl;
      //std::cout << "online:"<< ret[0][3] << endl;
      //std::cout << "name:"<< ret[0][0] << endl;
      for(auto e : ret) {
        UserInfo temp;
        temp.id = atoi(e[0].c_str());
        strcpy(temp.name, e[1].c_str());
        temp.online = e[3][0];
        user_info.push_back(temp);
      }

      std::unique_ptr<DataPack,FreeDeleter> pack = makeDataPackMsg(user_info.size() * sizeof(UserInfo));
      MessagePack* msg = (MessagePack*)pack->pack_data;
      msg->msg_type = MSG_TYPE_FIND_USER_RESPOND;
      strcpy(msg->common, USER_EXIST);
      for(int i = 0; i < user_info.size(); i++) {
        memcpy(msg->data + i * sizeof(UserInfo), &user_info[i], sizeof(UserInfo));
      }

      if(send(fd, pack.get(), pack->total_size, 0) <= 0) {
        handleConnetClose(fd);
      }
    }
  }
}

void CloudStorageServer::handleFilePack(DataPack* pack, int fd) {

}
