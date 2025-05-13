#pragma once
#include <chrono>
using namespace std::chrono;
// 自己实现的计时器类
class ZTimer {
private:
  high_resolution_clock::time_point _start_time;
  high_resolution_clock::time_point _end_time;
  bool _is_running;

public:
  ZTimer() : _is_running(false) {}

  void start() {
    if (!_is_running) {
      _start_time = high_resolution_clock::now();
      _is_running = true;
    }
  }

  void stop() {
    if (_is_running) {
      _end_time = high_resolution_clock::now();
      _is_running = false;
    }
  }

  void restart() {
    stop();
    start();
  }

  double getElapsedSeconds() const {
    if (_is_running) {
      auto now = high_resolution_clock::now();
      return duration_cast<duration<double>>(now - _start_time).count();
    } else {
      return duration_cast<duration<double>>(_end_time - _start_time).count();
    }
  }

  double getElapsedMilliseconds() const { return getElapsedSeconds() * 1000.0; }

  system_clock::time_point getStartTime() const { return _start_time; }

  system_clock::time_point getEndTime() const { return _end_time; }
};