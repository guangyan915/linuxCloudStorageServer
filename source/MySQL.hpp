#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <mutex>

class MySQL {
public:
    static MySQL &getInstance() {
        static MySQL instance("127.0.0.1", "root", "lgy937114", "cloudStorageInfo");
        return instance;
    }

    MySQL(const MySQL &) = delete;
    MySQL &operator=(const MySQL &) = delete;

    ~MySQL() {
        // 关闭MySQL连接
        mysql_close(&mysql_);
    }

    // 执行查询语句，返回查询结果
    std::vector<std::vector<std::string>> query(const std::string &sql) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::vector<std::string>> result;
        // 执行SQL语句
        if (mysql_query(&mysql_, sql.c_str())) {
            std::cerr << "Failed to execute SQL: " << mysql_error(&mysql_) << std::endl;
            return result;
        }
        // 获取查询结果集
        MYSQL_RES *res = mysql_store_result(&mysql_);
        if (res == nullptr) {
            std::cerr << "Failed to get result set: " << mysql_error(&mysql_) << std::endl;
            return result;
        }
        // 获取查询结果集的列数
        int column_count = mysql_num_fields(res);
        // 遍历查询结果集
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            std::vector<std::string> row_vec;
            for (int i = 0; i < column_count; i++) {
                row_vec.push_back(row[i] ? row[i] : "");
            }
            result.push_back(row_vec);
        }
        // 释放查询结果集
        mysql_free_result(res);
        return result;
    }

    // 执行SQL语句，不返回查询结果
    void exec(const std::string &sql) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (mysql_query(&mysql_, sql.c_str())) {
            std::cerr << "Failed to execute SQL: " << mysql_error(&mysql_) << std::endl;
        }
    }

private:
    MySQL(const std::string &host, const std::string &user, const std::string &passwd, const std::string &db)
            : host_(host), user_(user), passwd_(passwd), db_(db) {
        // 初始化MySQL连接
        mysql_init(&mysql_);
        // 连接MySQL
        if (!mysql_real_connect(&mysql_, host_.c_str(), user_.c_str(), passwd_.c_str(), db_.c_str(), 0, nullptr, 0)) {
            std::cerr << "Failed to connect to database: " << mysql_error(&mysql_) << std::endl;
        }
    }

    std::string host_;
    std::string user_;
    std::string passwd_;
    std::string db_;
    MYSQL mysql_;
    std::mutex mutex_;
};
