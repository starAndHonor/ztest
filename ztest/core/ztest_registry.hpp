#pragma once
#include "ztest_base.hpp"
#include "ztest_timer.hpp"
#include "ztest_types.hpp"
#include <mutex>
#include <thread>
// ZTestRegistry定义了测试注册中心，用于gtest类似语法。
class ZTestRegistry {
private:
  vector<unique_ptr<ZTestBase>> _tests;
  mutex _mutex;

public:
  static ZTestRegistry &instance() {
    static ZTestRegistry reg;
    return reg;
  }

  void addTest(unique_ptr<ZTestBase> test) {
    lock_guard<mutex> lock(_mutex);
    _tests.push_back(move(test));
  }

  vector<unique_ptr<ZTestBase>> takeTests() {
    lock_guard<mutex> lock(_mutex);
    vector<unique_ptr<ZTestBase>> result;
    result.swap(_tests);
    return result;
  }
};
