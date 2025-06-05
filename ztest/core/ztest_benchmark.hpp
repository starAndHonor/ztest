
#pragma once
#include "ztest_base.hpp"
#include "ztest_result.hpp"
#include "ztest_timer.hpp"
#include <functional>
#include <iostream>
#include <vector>

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

  std::unique_ptr<ZTestBase> clone() const override {
    return std::make_unique<ZBenchMark>(*this);
  }
  int getIterations() const { return _iterations; }
  vector<double> getIterationTimestamps() const { return _iterationTimestamps; }
  virtual ZState run_single_case() {}
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