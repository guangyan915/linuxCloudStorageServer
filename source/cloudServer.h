#ifndef CLOUDSERVER_H
#define CLOUDSERVER_H

#include "TCPSocket.h"

class CloudServer {
private:
  CloudServer() {}
public:
  static CloudServer& getInstance();
  int runServer();

private:

};

#endif
