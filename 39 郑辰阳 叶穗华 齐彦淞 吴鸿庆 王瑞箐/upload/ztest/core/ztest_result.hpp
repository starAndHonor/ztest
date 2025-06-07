#pragma once
#include "ztest_base.hpp"
#include "ztest_timer.hpp"
#include <mutex>
#include <ostream>
#include <sstream>
#include <vector>
// ZTestResult是每一个测试的最终状态

class ZTestResult {
private:
  string _test_name;
  double _duration;
  high_resolution_clock::time_point _start_time;
  high_resolution_clock::time_point _end_time;
  ZState _test_state;
  string _error_msg;
  int _iterations;
  double _avg_time;
  ZType _test_type;
  std::vector<double> _iterationTimestamps;

public:
  ZTestResult()
      : _test_name("unknown"), _duration(0.0), _test_state(ZState::z_failed),
        _error_msg("") {}
  ZTestResult(string test_name, ZType ztype, double duration, ZState state,
              string error_msg, int iterations = 1)
      : _test_name(test_name), _duration(duration), _test_state(state),
        _error_msg(error_msg), _iterations(iterations),
        _avg_time(duration / iterations), _test_type(ztype) {}
  virtual ~ZTestResult() = default; // Add this line

  bool operator==(const ZTestResult &other) const {
    return _test_name == other._test_name && _test_state == other._test_state;
  }
  /**
   * @description: 设置测试结果
   * @return {*}
   */
  void setResult(const string &name, ZType ztype, ZState state,
                 string error_msg, system_clock::time_point start_time,
                 system_clock::time_point end_time, double used_time,
                 int iterations = 1) {
    _test_name = name;
    _test_type = ztype;
    _test_state = state;
    _error_msg = error_msg;
    _start_time = start_time;
    _end_time = end_time;
    _duration = used_time;
    _iterations = iterations;
    _avg_time = used_time / iterations;
  }

  const double &getUsedTime() const { return _duration; }

  const string &getErrorMsg() const { return _error_msg; }

  const string &getName() const { return _test_name; }
  const double getAverageTime() const { return _avg_time; }
  const int getIterations() const { return _iterations; }
  ZState getState() const { return _test_state; }
  ZType getType() const { return _test_type; }

  /**
   * @description: 获取测试结果字符串
   * @return {*}
   */
  string getResultString(const string &test_name = "") const {
    ostringstream oss;
    const auto duration_str = to_string(static_cast<int>(_duration)) + " ms";

    constexpr const char *green = "\033[1;32m";
    constexpr const char *red = "\033[1;31m";
    constexpr const char *reset = "\033[0m";

    if (_test_state == ZState::z_success)
      oss << green << "[    OK    ] " << reset;
    else
      oss << red << "[  FAILED  ] " << reset;

    oss << test_name << " (" << duration_str << ")";

    if (!_error_msg.empty()) {
      oss << "\n" << red << "Error: " << reset << _error_msg;
    }

    return oss.str();
  }

  const std::vector<double> &getIterationTimestamps() const {
    return _iterationTimestamps;
  }
  void setIterationTimestamps(const std::vector<double> &timestamps) {
    _iterationTimestamps = timestamps;
  }
};

class ZTestResultManager {
private:
  unordered_map<string, ZTestResult> _results;
  mutex _mutex;

public:
  ZTestResultManager() = default;
  static ZTestResultManager &getInstance() {
    static ZTestResultManager instance;
    return instance;
  }

  void addResult(const ZTestResult &result) {
    lock_guard<mutex> lock(_mutex);
    _results[result.getName()] = result;
  }

  const ZTestResult &getResult(const string &name) const {
    return _results.at(name);
  }

  const unordered_map<string, ZTestResult> &getResults() const {
    return _results;
  }
};