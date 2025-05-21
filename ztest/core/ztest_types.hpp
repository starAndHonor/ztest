/*
 * @Author: starAndHonor 13750616920@163.com
 * @Date: 2025-05-08 21:26:32
 * @LastEditors: starAndHonor 13750616920@163.com
 * @LastEditTime: 2025-05-21 19:39:00
 * @FilePath: /ztest/ztest/core/ztest_types.hpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#pragma once
#include <ostream>
#include <sstream>
enum class ZState { z_failed, z_success, z_unknown };
enum class ZType { z_safe, z_unsafe };

constexpr const char *toString(ZState state) {
  switch (state) {
  case ZState::z_success:
    return "Passed";
  case ZState::z_failed:
    return "Failed";
  case ZState::z_unknown:
    return "Unknown";
  default:
    return "Undefined";
  }
}
inline std::ostream &operator<<(std::ostream &os, ZState state) {
  switch (state) {
  case ZState::z_failed:
    os << "Failed";
    break;
  case ZState::z_success:
    os << "Successed";
    break;
  case ZState::z_unknown:
    os << "Unknown";
    break;
  default:
    os.setstate(std::ios_base::failbit);
  }
  return os;
}
enum class ZLogLevel { DEBUG, INFO, WARNING, ERROR };
