#include "TCPSocket.h"

void setConfigValue(std::string s) {  
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
  if(key == "server_ip") {
    TCPSocket::getInstance().setServerIp(value);
  }
  if(key == "server_port") {
    TCPSocket::getInstance().setServerPort(value);
  }
  if(key == "listen_backlog") {
    TCPSocket::getInstance().setListenBacklog(atoi(value.c_str()));
  }
}

int TCPSocket::toLoadConfig() {
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
    //exit(-1);
  }
  //std::cout << server_ip << std::endl;
  //std::cout << server_port << std::endl;
  file.close();
  return 0;
}

int TCPSocket::Socket() {
  listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_socket == -1) {
    std::cout << "socket套接字建立失败！";
    exit(-1);
  }
  return listen_socket;
}

int Bind() {
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(atoi(server_port.c_str());
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  int res = bind(listen_socket, (sockaddr*)server_addr, sizeof(server_addr));
  if(res < 0) {
    std::cout << "bind error！";
    exit(-1);
  }
  return res;
}

int TCPSocket::Listen() {
  int res = listen(listen_socket, listen_backlog);
  if(res == -1) {
    std::cout << "listen error!";
    exit(-1);
  }
  return res;
}

int TCPSocket::Accept(int listen_socket, struct sockaddr *addr, socklen_t *addrlen) {
  res = accept(listen_socket, addr, addrlen);
  if(res < 0) {
    std::cout << "accept error!";
    exit(-1);
  }
  return res;
}

TCPSocket& TCPSocket::getInstance() {
  static TCPSocket instance;
  return instance;
}

void TCPSocket::setServerIp(std::string ip) {
  server_ip = ip;
}

void TCPSocket::setServerPort(std::string port) {
  server_port = port;
}
