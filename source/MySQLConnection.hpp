#include <mysql/mysql.h> // 包含 MySQL 的头文件
#include <string>
#include <vector>
#include <iostream>
#include <mutex>

// MySQL 数据库连接类
class MySQLConnection {
public:
    static MySQLConnection* getInstance();
    ~MySQLConnection();

    // 执行 SQL 查询
    std::vector<std::vector<std::string>> query(const std::string& sql);
    // 执行 SQL 语句
    void execute(const std::string& sql);
private:
    MySQLConnection(std::string host = "127.0.0.1", std::string user = "root",\
        std::string password = "lgy937114", std::string database = "cloudStorageInfo");
    static MySQLConnection* instance_; // MySQLConnection 单例
    static std::mutex mutex_; // 互斥锁
    MYSQL* mysql_;
};

// 初始化 MySQLConnection 单例和互斥锁
MySQLConnection* MySQLConnection::instance_ = nullptr;
std::mutex MySQLConnection::mutex_;

// 构造函数，连接 MySQL 数据库
inline MySQLConnection::MySQLConnection(std::string host, std::string user, std::string password, std::string database)
{
    mysql_ = mysql_init(NULL);
    if (!mysql_real_connect(mysql_, host.c_str(), user.c_str(),
                            password.c_str(), database.c_str(), 0, NULL, 0)) {
        throw std::runtime_error("Failed to connect to database: " + std::string(mysql_error(mysql_)));
    }
}

// 析构函数，释放 MySQL 连接资源
inline MySQLConnection::~MySQLConnection()
{
    if (mysql_) {
        mysql_close(mysql_);
    }
}

// 获取 MySQL 连接单例
MySQLConnection* MySQLConnection::getInstance()
{
    // 加锁，保证线程安全
    std::lock_guard<std::mutex> lock(mutex_);

    if (!instance_) {
        instance_ = new MySQLConnection();
    }
    return instance_;
}

// 执行 SQL 查询并返回结果
inline std::vector<std::vector<std::string>> MySQLConnection::query(const std::string& sql)
{
    std::vector<std::vector<std::string>> result;
    MYSQL_RES* res = NULL;
    MYSQL_ROW row;

    // 执行查询
    if (mysql_query(mysql_, sql.c_str())) {
        throw std::runtime_error("Failed to execute query: " + std::string(mysql_error(mysql_)));
    }

    // 获取结果集
    res = mysql_store_result(mysql_);
    if(!res) return result;
    /*if (!res) {
        throw std::runtime_error("Failed to get result set: " + std::string(mysql_error(mysql_)));
    }
    */

    // 获取行数和列数
    int num_rows = mysql_num_rows(res);
    int num_fields = mysql_num_fields(res);

    // 遍历结果集，将结果保存到二维字符串数组中
    while ((row = mysql_fetch_row(res))) {
        std::vector<std::string> row_data;
        for (int i = 0; i < num_fields; i++) {
            if (row[i]) {
                row_data.emplace_back(row[i]);
            } else {
                row_data.emplace_back("");
            }
        }
        result.emplace_back(row_data);
    }

    // 释放结果集
    mysql_free_result(res);

    return result;
}

// 执行 SQL 语句
inline void MySQLConnection::execute(const std::string& sql)
{
    // 执行查询
    if (mysql_query(mysql_, sql.c_str())) {
        throw std::runtime_error("Failed to execute query: " + std::string(mysql_error(mysql_)));
    }
}
