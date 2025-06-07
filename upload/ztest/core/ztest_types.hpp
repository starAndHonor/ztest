#pragma once
#include <ostream>
#include <sstream>
enum class ZState { z_failed, z_success, z_unknown };
enum class ZType { z_safe, z_unsafe, z_benchmark, z_param };
static const char *toString(ZType type) {
  switch (type) {
  case ZType::z_safe:
    return "Safe";
  case ZType::z_unsafe:
    return "Unsafe";
  case ZType::z_benchmark:
    return "Benchmark";
  case ZType::z_param:
    return "Parameterized";
  default:
    return "Unknown";
  }
}
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