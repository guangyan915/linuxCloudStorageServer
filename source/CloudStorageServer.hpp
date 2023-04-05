#include <netinet/in.h> // sockaddr_in
#include <sys/types.h>  // socket
#include <sys/socket.h> // socket
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h> // epoll
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
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
    std::mutex mtx;                           // 互斥锁
    std::mutex mtx_send;
    std::mutex mtx_recv;
    map<int, std::pair<int, std::string>> login_user_map;     // 记录登录的用户，以fd作为键，登录类型，和登录名作为值
    map<std::string, int> login_fd_map;                       // 记录登录的用户， 以用户名为键，fd为值
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

    int Send(int fd, void* buf, size_t len, int flag = 0);
    int Recv(int fd, void* buf, size_t len, int flag = 0);
public:
    void Run();
    void handleConnetClose(int fd);
    static CloudStorageServer& getInstance();
    static void recvMsg(int fd);

public:
    void handleLoginRequest(MessagePack* msg_pack, int fd);
    void handleRegisterRequest(MessagePack* msg_pack, int fd);
    void handleFindUserRequest(MessagePack* msg_pack, int fd);  
    void handleAddFriendRequest(MessagePack* msg_pack, int fd);
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

int CloudStorageServer::Send(int fd, void* buf, size_t len, int flag) {
  mtx_send.lock();
  int ret = send(fd, buf, len, flag);
  mtx_send.unlock();
  if(ret == 0) {
    std::cout << "用户:" << login_user_map[fd].second << " 退出！" << std::endl;
    handleConnetClose(fd);
  }
  if (ret == -1) {
    int errsv = errno;  // 保存 errno 的值
    std::cerr << "send error: " << std::strerror(errno) << std::endl;
    if (errsv == EAGAIN || errsv == EWOULDBLOCK) {
      // 非阻塞套接字操作无法立即完成
    } else if (errsv == EINTR) {
      // 系统调用被信号中断
      
    } else if (errsv == EINVAL) {
      // 参数不合法
      
    } else if (errsv == EMSGSIZE) {
      // 数据超出套接字缓冲区的大小
      
    } else if (errsv == ENOTCONN) {
      // 套接字未连接
      
    } else if (errsv == EPIPE) {
      // 套接字已经被关闭，还有数据要发送
      
    } else {
      // 其他错误类型
    }
    // 暂时先这样处理，关闭连接
    
    std::cout << "用户:" << login_user_map[fd].second << " 退出！" << std::endl;
    handleConnetClose(fd);
  }
  return ret;
}

int CloudStorageServer::Recv(int fd, void* buf, size_t len, int flag) {
  mtx_recv.lock();
  int ret = recv(fd, buf, len, flag);
  mtx_recv.unlock();
  if(ret == 0) {
    std::cout << "用户:" << login_user_map[fd].second << " 退出！" << std::endl;
    handleConnetClose(fd);
  }
  if (ret == -1) {
    int errsv = errno;  // 保存 errno 的值
    std::cerr << "recv error: " << std::strerror(errno) << std::endl;
    if (errsv == EAGAIN || errsv == EWOULDBLOCK) {
      // 非阻塞套接字操作无法立即完成
      
    } else if (errsv == EINTR) {
      // 系统调用被信号中断
      
    } else if (errsv == EINVAL) {
      // 参数不合法
      
    } else if (errsv == ECONNRESET) {
      // 远程端点重置了连接。
      
    } else if (errsv == ENOTCONN) {
      // 套接字未连接
      
    } else if (errsv == EBADF) {
      // 无效的文件描述符
    } else if(errsv == ENOMEM){
      // 内存不足
    } else if(errsv == ENOTSOCK){
      // 描述符不是一个socket
    } else {
      // 其他错误
    }
    // 暂时先这样处理，关闭连接
    handleConnetClose(fd);
    std::cout << "用户:" << login_user_map[fd].second << " 退出！" << std::endl;
  }
  return ret;
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
      continue;
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
  DBOperator::getInstance().setNotOnline(login_user_map[fd].first, login_user_map[fd].second);
  
  mtx.lock();
  login_fd_map.erase(login_user_map[fd].second);
  login_user_map.erase(fd);
  // 将该fd也从epoll中注销
  struct epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN;
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
  mtx.unlock();
}

CloudStorageServer& CloudStorageServer::getInstance() {
  static CloudStorageServer instance;
  return instance;
}

void CloudStorageServer::recvMsg(int fd)
{
  // 获取包大小
  unsigned int total_size = 0;
  
  int ret = CloudStorageServer::getInstance().Recv(fd, &total_size, sizeof(total_size), 0);
  if(ret <= 0) return;
  // 获取包
  std::unique_ptr<DataPack, FreeDeleter> pack = makeDataPack(total_size - sizeof(DataPack));
  std::cout << "包大小：" << total_size << std::endl;
  // 需要偏移记录包大小的变量字节数,也要少读这字节数
  ret = CloudStorageServer::getInstance().Recv(fd, (char*)pack.get() + sizeof(total_size), total_size - sizeof(total_size), 0);
  if(ret <= 0) return;
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
      break;
    case MSG_TYPE_FIND_USER_REQUEST :
      handleFindUserRequest(msg_pack, fd);
      break;
    case MSG_TYPE_ADD_FRIEND_REQUEST :
      handleAddFriendRequest(msg_pack, fd);
      break;
    default :
      break;
  }
}

void CloudStorageServer::handleLoginRequest(MessagePack* msg_pack, int fd) {
  std::string login_name = std::string(msg_pack->common); 
  std::string passwd = std::string(msg_pack->common + 32);
  unsigned int login_manner = *((unsigned int*)msg_pack->data);
  //std::cout << "login_manner:" << login_manner << std::endl;
  std::unique_ptr<DataPack, FreeDeleter> pack = makeDataPackMsg(0);
  pack->pack_type = PACK_TYPE_MSG;
  MessagePack* msg = (MessagePack*)pack->pack_data;
  msg->msg_type = MSG_TYPE_LOGIN_RESPOND;
  int ret = DBOperator::getInstance().loginIsSucceed(login_name, passwd);
  if(ret == 0) {
    // 不在线,可以登录
    strcpy((char*)msg->common, LOGIN_SUCCEED);
    mtx.lock();
    login_user_map[fd].first = login_manner;
    login_user_map[fd].second = login_name;
    login_fd_map[login_name] = fd;
    // 用户名登录
    if(login_manner == 0) DBOperator::getInstance().setOnline(login_manner, login_name);
    else if(login_manner == 1) {}
    else if(login_manner == 2) {}
    else if(login_manner == 3) {}
    else {}
    mtx.unlock();
    std::cout << login_name << "登录成功！" << std::endl;
  }
  else if(ret == 1) {
    // 在线，暂时不能登录
    strcpy((char*)msg->common, LOGIN_USER_ONLINE);
  }
  else {
    // 账号或密码错误
    strcpy((char*)msg->common, LOGIN_NAMEORPASSWD_ERROR);
  }
  
  ret = Send(fd, pack.get(), pack->total_size, 0);
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
  Send(fd, pack.get(), pack->total_size, 0);
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
      if(Send(fd, pack.get(), pack->total_size, 0) <= 0) {
        return;
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
      if(Send(fd, pack.get(), pack->total_size, 0) <= 0) {
        return;
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
      if(Send(fd, pack.get(), pack->total_size, 0) <= 0) {
        return;
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

      Send(fd, pack.get(), pack->total_size, 0);
    }
  }
}

void CloudStorageServer::handleAddFriendRequest(MessagePack* msg_pack, int fd) {
  std::string send_name(msg_pack->common);
  std::string recv_name(msg_pack->common + 32);
  
  std::cout << "add:";
  int ret = DBOperator::getInstance().userIsOnline(recv_name);
  std::unique_ptr<DataPack, FreeDeleter> pack(makeDataPackMsg(0));
  MessagePack* msg = (MessagePack*)pack->pack_data;
  msg->msg_type = MSG_TYPE_ADD_FRIEND_RESPOND;
  int is_user_friend = DBOperator::getInstance().isUserFriend(send_name ,recv_name);
  if(ret == -1 || is_user_friend == -1) {
    // 用户不存在
    strcpy(msg->common, USER_NOT_EXIST);

  }
  else if(is_user_friend == 1) {
    // 以经是好友了
    std::cout << "is your friend";
    strcpy(msg->common, IS_USER_FRIEND);
  }
  else if(ret == 1) {
    // 在线
    std::cout << "online\n";
    int data_size = msg_pack->data_size;
    std::unique_ptr<DataPack, FreeDeleter> add_pack(makeDataPackMsg(data_size));
    MessagePack* add_msg = (MessagePack*)add_pack->pack_data;
    add_msg->msg_type = MSG_TYPE_ADD_FRIEND_REQUEST;
    strcpy(add_msg->common, send_name.c_str());
    strcpy(add_msg->common + 32, recv_name.c_str());
    if(data_size != 0) {
      strcpy(add_msg->data, msg_pack->data);
    }
    // 请求转发给好友
    if(Send(login_fd_map[recv_name], add_pack.get(), add_pack->total_size, 0) <= 0) {
      strcpy(msg->common, ADD_FRIEND_REQUEST_SEND_ERROR);
      std::cout << send_name << "->" << recv_name  << " error!" << std::endl;
    }
    else {
      strcpy(msg->common, ADD_FRIEND_REQUEST_SEND);
      std::cout << send_name << "->" << recv_name << std::endl;
    }
  }
  else {
    // ret == 0 不在线
    std::cout << "not noline\n";
    int recv_id = DBOperator::getInstance().getUserId(0, recv_name);
    int send_id = DBOperator::getInstance().getUserId(0, send_name);
    if(recv_id == -1) strcpy(msg->common, USER_NOT_EXIST);
    else {
      string message (msg_pack->data);
      DBOperator::getInstance().insertNotOnlineMessage(recv_id, send_id, MSG_TYPE_ADD_FRIEND_REQUEST, message);
    }
  }
}

void CloudStorageServer::handleFilePack(DataPack* pack, int fd) {

}
