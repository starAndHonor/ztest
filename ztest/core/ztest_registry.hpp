#pragma once
#include "ztest_base.hpp"
#include "ztest_timer.hpp"
#include "ztest_types.hpp"
#include <mutex>
#include <thread>
// ZTestRegistry定义了测试注册中心，用于gtest类似语法。
class ZTestRegistry {
private:
  vector<shared_ptr<ZTestBase>> _tests; // 所有被注册的测试用例
  mutex _mutex;

public:
  /**
   * @description: 单例模式,唯一的注册实例
   * @return reg
   */
  static ZTestRegistry &instance() {
    static ZTestRegistry reg;
    return reg;
  }
  /**
   * @description: 将测试用例加入注册中心
   * @param {shared_ptr<ZTestBase>} test
   * @return none
   */
  void addTest(shared_ptr<ZTestBase> test) {
    lock_guard<mutex> lock(_mutex);
    _tests.push_back(test);
  }
  /**
   * @description: 获取所有测试用例
   * @return 所有测试用例
   */
  vector<shared_ptr<ZTestBase>> takeTests() {
    lock_guard<mutex> lock(_mutex);
    vector<shared_ptr<ZTestBase>> result;
    result.swap(_tests);
    return result;
  }
};