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
  int getUserId(unsigned int login_manner, const std::string& user);
  bool setOnline(unsigned int login_manner, const std::string& user);
  bool setNotOnline(unsigned int login_manner, const std::string& user);
  int loginIsSucceed(const std::string& name, const std::string& passwd);
  bool insertUser(const std::string& name, const std::string& passwd); 
  std::vector<std::vector<std::string>> getUserInfo(const std::string& find_criteria, const std::string& user);
  int userIsOnline(const std::string& name);
  int userIsOnline(unsigned int id);
  int isUserFriend(const std::string& name1, const std::string& name2);
  int insertFriend(const std::string& name1, const std::string& name2);
  bool insertNotOnlineMessage(int recv_id, int send_id, int message_type, const std::string& message);
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

bool DBOperator::setOnline(unsigned int login_manner, const std::string& user) {
  std::string sql;
  if(login_manner == 0) sql = QString("update userInfo set online = 1 where name = '%0'").arg(user).toStdString();
  else if(login_manner == 1);
  else if(login_manner == 2);
  else if(login_manner == 3);
  else if(login_manner == 4);
  std::cout << "setonlie:" << sql << std::endl;
  return MySQL::getInstance().exec(sql);
}

bool DBOperator::setNotOnline(unsigned int login_manner, const std::string& user) {

  std::string sql;
  if(login_manner == 0) sql = QString("update userInfo set online = 0 where name = '%0'").arg(user).toStdString();
  else if(login_manner == 1);
  else if(login_manner == 2);
  else if(login_manner == 3);
  else if(login_manner == 4);
  std::cout << "setNotOnline:"  << sql << std::endl;
  return MySQL::getInstance().exec(sql);
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

int DBOperator::userIsOnline(const std::string& name) {
  std::string sql = QString("select online from userInfo where name = '%0'").arg(name).toStdString();
  std::cout << sql << std::endl;
  auto ret = MySQL::getInstance().query(sql);
  if(ret.empty()) {
    // 用户不存在
    return -1;
  }
  return atoi(ret[0][0].c_str());
}

int DBOperator::userIsOnline(unsigned int id) {
  std::string sql = QString("select online from userInfo where id = %0").arg(std::to_string(id)).toStdString();
  std::cout << sql << std::endl;
  auto ret = MySQL::getInstance().query(sql);
  if(ret.empty()) {
    // 用户不存在
    return -1;
  }
  return atoi(ret[0][0].c_str());

}

int DBOperator::getUserId(unsigned int login_manner, const std::string &user) {
  std::string sql;
  // 暂时只有用户名登录
  if(login_manner == 0) {
    sql = QString("select id from userInfo where name = '%0'").arg(user).toStdString();
  }

  auto ret = MySQL::getInstance().query(sql);
  // 用户不存在
  if(ret.empty()) return -1;
  return atoi(ret[0][0].c_str());
}

int DBOperator::insertFriend( const std::string &name1, const std::string &name2 ) {
  int id1 = getUserId(0, name1);
  int id2 = getUserId(0, name2);
  if(id1 == -1 || id2 == -1) return -1;
  std::string sql = QString("insert into friendInfo value(%0, %1)").arg(std::to_string(id1)).arg(std::to_string(id2)).toStdString();
  if(MySQL::getInstance().exec(sql)) {
    return 1;
  }
  else return 0;
}

int DBOperator::isUserFriend( const std::string &name1, const std::string &name2 ) {
  int id1 = getUserId(0, name1);
  int id2 = getUserId(0, name2);
  if(id1 == -1 || id2 == -1) {
    return -1;
  }

  std::string sql = QString("select id1 from friendInfo where id1 = %0 && id2 = %1").arg(std::to_string(id1)).arg(std::to_string(id2)).toStdString();
  auto ret = MySQL::getInstance().query(sql);
  if(!ret.empty()) return 1;
  sql = QString("select id1 from friendInfo where id1 = %0 && id2 = %1").arg(std::to_string(id2)).arg(std::to_string(id1)).toStdString();
  if(!MySQL::getInstance().query(sql).empty()) return 1;
  return 0;
  // -1 用户不存在
  // 1 是好友
  // 不是好友
}

bool DBOperator::insertNotOnlineMessage(int recv_id, int send_id, int message_type, const std::string &message) {
  std::string sql = QString("insert into notNolineMessage (recv_id, send_id, message_type, message, send_time) value(%0, %1, %2, '%3', now())").arg(std::to_string(recv_id)).arg(std::to_string(send_id)).arg(std::to_string(message_type)).arg(message).toStdString();
  std::cout << sql << std::endl;
  return MySQL::getInstance().exec(sql);
}
