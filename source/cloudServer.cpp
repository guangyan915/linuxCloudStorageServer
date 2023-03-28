#include "cloudServer.h"

CloudServer& CloudServer::getInstance() {
  static CloudServer instance;
  return instance;
}

int CloudServer::runServer() {
  int res = TCPSocket::Socket();
  res = TCPSocket::Bind();
  res = TCPSocket::Listen();
  sockaddr_in client_addr;

  res = TCPSocket::Accept();

  return 0;
}
