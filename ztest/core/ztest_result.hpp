#pragma once
#include "ztest_base.hpp"
#include "ztest_timer.hpp"
#include <ostream>
#include <sstream>
// ZTestResult是每一个测试的最终状态
class ZTestResult {
private:
  double _used_time;
  string _test_name;
  high_resolution_clock::time_point _start_time;
  high_resolution_clock::time_point _end_time;
  ZState _test_state;
  string _error_msg;

public:
  ZTestResult()
      : _used_time(0.0), _test_state(ZState::z_failed), _error_msg("") {}

  bool operator==(const ZTestResult &other) const {
    return _test_name == other._test_name && _test_state == other._test_state;
  }
  void setResult(const string &name, ZState state, string error_msg,
                 system_clock::time_point start_time,
                 system_clock::time_point end_time, double used_time) {
    _test_name = name;
    _test_state = state;
    _error_msg = error_msg;
    _start_time = start_time;
    _end_time = end_time;
    _used_time = used_time;
  }

  const double &getUsedTime() const { return _used_time; }

  const string &getErrorMsg() const { return _error_msg; }

  const string &getName() const { return _test_name; }

  ZState getState() const { return _test_state; }

  string getResultString(const string &test_name = "") const {
    ostringstream oss;
    const auto duration_str = to_string(static_cast<int>(_used_time)) + " ms";

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
};