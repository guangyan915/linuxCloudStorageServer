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
  bool userIdIsExist(const std::string& id);
  bool setOnlineName(const std::string& value);
  bool setNotOnlineName(const std::string& value);
  int loginIsSucceed(const std::string& name, const std::string& passwd);
  bool insertUser(const std::string& name, const std::string& passwd); 
  std::vector<std::vector<std::string>> getUserInfo(const std::string& find_criteria, const std::string& user);
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

bool DBOperator::userIdIsExist(const std::string& id) {
  // 如果有非数字，就崩了
  for(int i = 0; i < id.size(); i++) {
    if(id[i] < '0' || id[i] > '9') return false;
  }
  std::string sql = QString("select id from userInfo where id = %0").arg(id).toStdString();
  //std::cout << sql << std::endl;
  auto ret = MySQL::getInstance().query(sql);
  std::cout << sql << std::endl;
  return ret.empty() ? false : true;
}

bool DBOperator::setOnlineName(const std::string& value) {
  std::string sql = QString("update userInfo set online = 1 where name = '%0'").arg(value).toStdString();
}

bool DBOperator::setNotOnlineName(const std::string& value) {
  std::string sql = QString("update userInfo set online = 0 where name = '%0'").arg(value).toStdString();
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

bool DBOperator::insertUser(const std::string& name, const std::string& passwd) {
  std::string sql = QString("insert into userInfo (name, passwd, online) values('%0', '%1', 0)")\
                    .arg(name).arg(passwd).toStdString();
 return MySQL::getInstance().exec(sql);
}

std::vector<std::vector<std::string>> DBOperator::getUserInfo(const std::string& find_criteria, const std::string& user) {
  std::string sql;
  //std::cout << find_criteria << std::endl;
  if(find_criteria == "id") {
    sql = QString("select * from userInfo where id = %0").arg(user).toStdString();
  }
  else if(find_criteria == "name") {
    sql = QString("select * from userInfo where name = '%0'").arg(user).toStdString();
  }
  std::cout << "sql:"  << sql << std::endl;
  return MySQL::getInstance().query(sql);
}
