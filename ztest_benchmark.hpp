
#pragma once
#include "ztest_base.hpp"
#include "ztest_result.hpp"
#include "ztest_timer.hpp"
#include <functional>
#include <iostream>
#include <vector>

//ZBenchMark类是用于执行基准测试的核心类，继承自 ZTestBase
class ZBenchMark : public ZTestBase {
private:
  // std::function<void()> _benchmark_func;
  int _iterations = 1000; // 默认迭代次数
  std::vector<double> _iterationTimestamps;

public:
  ZBenchMark(const std::string &name, const std::string &description = "")
      : ZTestBase(name, ZType::z_benchmark, description) {}
  ZBenchMark &withIterations(int iterations) {
    _iterations = iterations;
    return *this;
  }

  // ZBenchMark &setBenchmarkFunc(std::function<void()> func) {
  //   _benchmark_func = func;
  //   return *this;
  // }

   /**
   * @description: 创建当前对象的副本，用于测试用例的复制和管理
   * @return 迭代次数
   */
  std::unique_ptr<ZTestBase> clone() const override {
    return std::make_unique<ZBenchMark>(*this);
  }
  
  int getIterations() const { return _iterations; }
  /**
  * @description: 获取迭代时间戳
  * @return 每次迭代的耗时列表
  */
  vector<double> getIterationTimestamps() const { return _iterationTimestamps; }

  virtual ZState run_single_case() {}

  /**
   * @description: 执行基准测试的核心逻辑
   */
  ZState run() override {
    // if (!_benchmark_func)
    // return ZState::z_failed;

    ZTimer timer;
    timer.start();

    for (int i = 0; i < _iterations; ++i) {
      auto start = std::chrono::steady_clock::now();
      // _benchmark_func();
      run_single_case();
      auto end = std::chrono::steady_clock::now();
      _iterationTimestamps.push_back(
          duration_cast<duration<double>>(end - start).count());
    }
    timer.stop();

    setState(ZState::z_success);
    return ZState::z_success;
  }
};