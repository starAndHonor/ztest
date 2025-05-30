
#pragma once
#include "ztest_base.hpp"
#include "ztest_result.hpp"
#include "ztest_timer.hpp"
#include <functional>
#include <vector>

class ZBenchMark : public ZTestBase {
private:
  std::function<void()> _benchmark_func;
  int _iterations = 1000; // 默认迭代次数

public:
  ZBenchMark(const std::string &name, const std::string &description = "")
      : ZTestBase(name, ZType::z_benchmark, description) {}

  ZBenchMark &withIterations(int iterations) {
    _iterations = iterations;
    return *this;
  }

  ZBenchMark &setBenchmarkFunc(std::function<void()> func) {
    _benchmark_func = func;
    return *this;
  }

  std::unique_ptr<ZTestBase> clone() const override {
    return std::make_unique<ZBenchMark>(*this);
  }
  int getIterations() const { return _iterations; }
  ZState run() override {
    if (!_benchmark_func)
      return ZState::z_failed;

    ZTimer timer;
    timer.start();

    for (int i = 0; i < _iterations; ++i) {
      _benchmark_func();
    }

    timer.stop();
    double avg_time = timer.getElapsedMilliseconds() / _iterations;

    ZTestResult result;
    result.setResult(getName(), ZType::z_benchmark, ZState::z_success, "",
                     timer.getStartTime(), timer.getEndTime(), avg_time,
                     _iterations);

    {
      //   std::lock_guard<std::mutex> lock(
      //       ZTestResultManager::getInstance()._mutex);
      ZTestResultManager::getInstance().addResult(std::move(result));
    }

    setState(ZState::z_success);
    return ZState::z_success;
  }
};