#pragma once
#include "ztest_base.hpp"
#include "ztest_suite.hpp"
#include <exception>
// ZtestSingleCase定义单例测试
template <typename RetType> class ZTestSingleCase : public ZTestBase {
private:
  function<RetType()> _test_func;
  RetType _expected_return;

public:
  ZTestSingleCase(string name, ZType type, string description)
      : ZTestBase(name, type, description) {}

  template <typename Func, typename... Args>
  auto &setTestFunc(Func &&f, Args &&...args) {
    _test_func = [f = forward<Func>(f),
                  args = make_tuple(forward<Args>(args)...)]() {
      return apply(f, args);
    };
    return *this;
  }

  auto &setExpectedOutput(const RetType &expected) {
    _expected_return = expected;
    return *this;
  }

  auto &withDescription(const string &desc) {
    setDescription(desc);
    return *this;
  }
  unique_ptr<ZTestBase> clone() const override {
    auto cloned = make_unique<ZTestSingleCase>(*this);
    return cloned;
  }
  ZState run() override {
    if (!_test_func)
      return ZState::z_failed;

    try {
      runBeforeAll();
      auto actual = _test_func();
      runAfterEach();
      if (actual != _expected_return) {
        ostringstream expected_oss, actual_oss;
        expected_oss << _expected_return;
        actual_oss << actual;

        throw ZTestFailureException(getName(), expected_oss.str(),
                                    actual_oss.str());
      }
      setState(ZState::z_success);
      return ZState::z_success;
    } catch (...) {
      setState(ZState::z_failed);
      throw;
    }
  }
};
template <typename RetType> class TestBuilder {
private:
  unique_ptr<ZTestSingleCase<RetType>> _test_case;

public:
  explicit TestBuilder(unique_ptr<ZTestSingleCase<RetType>> &&test)
      : _test_case(move(test)) {}
  TestBuilder &addToSuite(ZTestSuite &target_suite) {
    if (_test_case) {
      unique_ptr<ZTestBase> base_case(std::move(_test_case));
      target_suite.addTest(std::move(base_case));
    }
    return *this;
  }
  TestBuilder &afterEach(function<void()> hook) {
    _test_case->addAfterEach(move(hook));
    return *this;
  }
  TestBuilder &registerTest() {
    if (_test_case) {
      ZTestRegistry::instance().addTest(std::move(_test_case));
    }
    return *this;
  }
  TestBuilder &setExpectedOutput(const RetType &expected) {
    _test_case->setExpectedOutput(expected);
    return *this;
  }

  TestBuilder &beforeAll(function<void()> hook) {
    _test_case->addBeforeAll(move(hook));
    return *this;
  }

  TestBuilder &withDescription(const string &desc) {
    _test_case->withDescription(desc);
    return *this;
  }

  unique_ptr<ZTestSingleCase<RetType>> build() { return move(_test_case); }

  operator unique_ptr<ZTestBase>() { return move(_test_case); }
};

class TestFactory {
public:
  template <typename Func, typename... Args>
  static auto createTest(const string &name, ZType type, const string &desc,
                         Func &&f, Args &&...args)
      -> TestBuilder<decltype(apply(f, make_tuple(args...)))> {
    using RetType = decltype(apply(f, make_tuple(args...)));
    auto test = make_unique<ZTestSingleCase<RetType>>(name, type, desc);
    test->setTestFunc(forward<Func>(f), forward<Args>(args)...);
    return TestBuilder<RetType>(move(test));
  }
};