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
  /**
 * @description: 启动计时器
 */
  void start() {
    if (!_is_running)
      _start_time = high_resolution_clock::now(), _is_running = true;
  }
  /**
 * @description: 停止计时器
 */
  void stop() {
    if (_is_running)
      _end_time = high_resolution_clock::now(), _is_running = false;
  }
  /**
 * @description: 重启计时器
 */
  void restart() {
    stop();
    start();
  }
  /**
 * @description: 获取已 elapsed 的时间（秒为单位）
 * @return 经过的时间（秒）
 */
  double getElapsedSeconds() const {
    if (_is_running) {
      auto now = high_resolution_clock::now();
      return duration_cast<duration<double>>(now - _start_time).count();
    } else {
      return duration_cast<duration<double>>(_end_time - _start_time).count();
    }
  }
  /**
 * @description: 获取已 elapsed 的时间（毫秒为单位）
 * @return 经过的时间（毫秒）
 */
  double getElapsedMilliseconds() const { return getElapsedSeconds() * 1000.0; }
  /**
 * @description: 获取计时器的开始时间点
 * @return 开始时间的 time_point 对象
 */
  system_clock::time_point getStartTime() const { return _start_time; }
  /**
 * @description: 获取计时器的结束时间点
 * @return 结束时间的 time_point 对象
 */
  system_clock::time_point getEndTime() const { return _end_time; }
};