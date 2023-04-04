#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <mutex>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <fstream>
#include <chrono>

class MySQLConnection {
public:
    MySQLConnection(const std::string& host, const std::string& user, const std::string& passwd, const std::string& db)
        : host_(host), user_(user), passwd_(passwd), db_(db), conn_(nullptr) {
    }

    ~MySQLConnection() {
        if (conn_ != nullptr) {
            mysql_close(conn_);
        }
    }

    void connect() {
        // 初始化MySQL连接
        conn_ = mysql_init(nullptr);
        if (conn_ == nullptr) {
            throw std::runtime_error("Failed to initialize MySQL connection.");
        }
        // 连接MySQL
        if (!mysql_real_connect(conn_, host_.c_str(), user_.c_str(), passwd_.c_str(), db_.c_str(), 0, nullptr, 0)) {
            std::string err_msg = "Failed to connect to database: ";
            err_msg += mysql_error(conn_);
            mysql_close(conn_);
            conn_ = nullptr;
            throw std::runtime_error(err_msg);
        }
    }

    MYSQL* get() const {
        return conn_;
    }

private:
    std::string host_;
    std::string user_;
    std::string passwd_;
    std::string db_;
    MYSQL* conn_;
};

class MySQL {
public:
    static MySQL& getInstance() {
        static MySQL instance;
        return instance;
    }

    MySQL(const MySQL&) = delete;
    MySQL& operator=(const MySQL&) = delete;

    // 执行查询语句，返回查询结果
    std::vector<std::vector<std::string>> query(const std::string& sql) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::vector<std::string>> result;
        // 获取MySQL连接
        MYSQL* conn = connection_->get();
        if (conn == nullptr) {
            throw std::runtime_error("Failed to get MySQL connection.");
        }
        // 执行SQL语句
        if (mysql_query(conn, sql.c_str())) {
            std::string err_msg = "Failed to execute SQL: ";
            err_msg += mysql_error(conn);
            logError(err_msg);
            throw std::runtime_error(err_msg);
        }
        // 获取查询结果集
        std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> res(mysql_store_result(conn), mysql_free_result);
        if (res == nullptr) {
            std::string err_msg = "Failed to get result set: ";
            err_msg += mysql_error(conn);
            logError(err_msg);
            throw std::runtime_error(err_msg);
        }
        // 获取查询结果集的列数
        int column_count = mysql_num_fields(res.get());
        // 遍历查询结果集
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res.get()))) {
            std::vector<std::string> row_vec;
            for (int i = 0; i < column_count; i++) {
                row_vec.push_back(row[i] ? row[i] : "");
            }
            result.push_back(row_vec);
        }
        return result;
    }

    bool exec(const std::string& sql) {
        std::lock_guard<std::mutex> lock(mutex_);
        // 获取MySQL连接
        MYSQL* conn = connection_->get();
        if (conn == nullptr) {
            throw std::runtime_error("Failed to get MySQL connection.");
        }
        // 执行SQL语句
        if (mysql_query(conn, sql.c_str())) {
            std::string err_msg = "Failed to execute SQL: ";
            err_msg += mysql_error(conn);
            logError(err_msg);
            return false;
        }
        return true;
    }
private:
    MySQL() {
        // 创建MySQL连接
        try {
          connection_ = std::make_unique<MySQLConnection>("127.0.0.1", "root", "lgy937114", "cloudStorageInfo");
          connection_->connect();
        }
        catch (const std::exception& ex) {
            logError(ex.what());
            throw;
        }
    }

    void logError(const std::string& msg) {
        static std::mutex log_mutex;
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream logfile("mysql_error.log", std::ios::app);
        if (logfile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto now_c = std::chrono::system_clock::to_time_t(now);
            logfile << std::ctime(&now_c) << msg << std::endl;
            logfile.close();
        }
    }

    std::mutex mutex_;
    std::unique_ptr<MySQLConnection> connection_;
};
