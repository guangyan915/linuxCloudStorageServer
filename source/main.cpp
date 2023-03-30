#include <iostream>
#include "CloudStorageServer.hpp"

int main() {
  //if(DBOperator::getInstance().userNameIsExist("lucy")) std::cout << "存在\n";
  //else std::cout << "不存在\n";
  //std::cout << "online:"  << DBOperator::getInstance().loginIsSucceed("lucy", "lucy") << std::endl;
  CloudStorageServer server;
  server.Run();
  
}

