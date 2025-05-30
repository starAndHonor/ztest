
#pragma once
#include "ztest_base.hpp"
#include "ztest_utils.hpp"
#include <any>
#include <exception>
#include <functional>
template <typename InputType, typename OutputType> class ZTestDataManager {

public:
  using Input = InputType;
  using Output = OutputType;
  ZTestDataManager(initializer_list<pair<Input, Output>> cases)
      : _data(cases) {}

  auto current() const { return _data[_index]; }
  bool has_next() const { return _index < _data.size(); }
  void next() { ++_index; }
  void reset() { _index = 0; }

private:
  vector<pair<Input, Output>> _data;
  size_t _index = 0;
};

template <typename Input, typename Output>
class ZTestParameterized : public ZTestBase {
protected:
  ZTestDataManager<Input, Output> &_data;

public:
  ZTestParameterized(const string &name, ZType type, const string &desc,
                     ZTestDataManager<Input, Output> &data)
      : ZTestBase(name, type, desc), _data(data) {}

  ZState run() override {
    _data.reset();
    while (_data.has_next()) {
      if (run_single_case() != ZState::z_success)
        return ZState::z_failed;
      _data.next();
    }
    return ZState::z_success;
  }

  virtual ZState run_single_case() = 0;
};
