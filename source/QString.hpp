#include <string>
#include <iostream>
#include <sstream>

class QString
{
public:
    // str 待格式化字符
    QString(const std::string& str) : mString(str) {}

    // 返回格式化后的字符
    std::string toStdString() const { return mString; }

    // 参数
    QString arg(const std::string& param)
    {
        std::ostringstream oss;
        oss << "%" << mCount;
        replace(oss.str(), param);
        mCount++;
        return *this;
    }

private:
    // 替换字符 根据qt里面的用%0 %1 ....代表需要替换的参数
    void replace(const std::string& key, const std::string& param)
    {
        auto index = mString.find(key);
        if (index != std::string::npos)
        {
            mString.replace(index, key.length(), param);
        }
    }

private:
    std::string mString;
    // %x 计数器 arg 调用一次 mCount加1
    int mCount = 0;
};
