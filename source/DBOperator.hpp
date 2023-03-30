#include "MySQL.hpp"
#include "QString.hpp"
#include <stdlib.h>

class DBOperator {
private:
  DBOperator() {}
  ~DBOperator() {}
public:
  static DBOperator& getInstance();
  bool userNameIsExist(const std::string& name);
  int loginIsSucceed(const std::string& name, const std::string& passwd);
};

DBOperator& DBOperator::getInstance() {
  static DBOperator instance;
  return instance;
}

bool DBOperator::userNameIsExist(const std::string& name) {
  std::string sql = QString("select name from userInfo where name = '%0'").arg(name).toStdString();
  auto ret = MySQL::getInstance().query(sql);
  std::cout << sql << std::endl;
  return ret.empty() ? false : true;
}

int DBOperator::loginIsSucceed(const std::string& name, const std::string& passwd) {
  std::string sql = QString("select online from userInfo where name = '%0' and passwd = '%1'").arg(name).arg(passwd).toStdString();
  std::cout << sql << std::endl;
  auto ret = MySQL::getInstance().query(sql);
  // -1 不在线
  if(ret.empty()) return -1;
  else {
    // 1 在线
    // 0 不在线
    return atoi(ret[0][0].c_str());
  }
}
