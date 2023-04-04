#include <iostream>
#include "CloudStorageServer.hpp"
//#include "Protocol.hpp"

int main() {
  //if(DBOperator::getInstance().userNameIsExist("lucy")) std::cout << "存在\n";
  //else std::cout << "不存在\n";
  //std::cout << "online:"  << DBOperator::getInstance().loginIsSucceed("lucy", "lucy") << std::endl;
  CloudStorageServer server;
  server.Run();
  

  //auto pack = makeDataPackMsg(0);

  //std::unique_ptr<DataPack> pack = makeDataPackMsg(0);
  //int total_size;
  //std::cin >> total_size;
  //std::unique_ptr<DataPack, FreeDeleter> pack = makeDataPackMsg(total_size);
  //std::cout << pack->total_size << std::endl;
  
}

