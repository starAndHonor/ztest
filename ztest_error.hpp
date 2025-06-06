
#pragma once
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;
//ZTestFailureException 类是自定义的测试失败异常类
class ZTestFailureException : public std::exception {
private:
  string _msg;

public:
    /**
 * @description: 构造函数，初始化测试失败异常信息
 * @param test_name 失败的测试用例名称
 * @param expected 期望的测试结果
 * @param actual 实际的测试结果
 */
  ZTestFailureException(const string &test_name, const string &expected,
                        const string &actual) {
    ostringstream oss;
    oss << "Test Failure in " << test_name << ":\n"
        << "  Expected: " << expected << "\n"
        << "  Actual  : " << actual;
    _msg = oss.str();
  }
  /**
 * @description: 获取异常信息字符串
 * @return 异常信息的C风格字符串指针
 */
  const char *what() const noexcept override { return _msg.c_str(); }
};
