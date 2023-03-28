#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string>
#include <fstream>


#define CONFIG_FILE_PATH "./configFile/server.config"

class TCPSocket {
private:
  TCPSocket() {}

public:
  int toLoadConfig();

  int Socket();
  int Bind();
  int Listen();
  int Accept(int listen_socket, struct sockaddr *addr, socklen_t *addrlen);

  int recvMsg(void* fd);
  int sendMsg(int* fd, const std::string& msg, size_t msg_size);
  
  void setServerIp(std::string ip);
  void setServerPort(std::string port);
  void setListenBacklog(int backlog);

public:               
  static TCPSocket& getInstance();     // 单例模式

private:
  int listen_socket;            // 服务端socket
  std::string server_ip;        
  std::string server_port;
  int listen_backlog;
};


#endif