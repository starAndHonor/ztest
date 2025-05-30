#pragma once
#include "ztest_base.hpp"
#include "ztest_context.hpp"
#include "ztest_registry.hpp"
#include "ztest_result.hpp"
// ZTestSuite 成组的测试
class ZTestSuite : public ZTestBase {
private:
  vector<unique_ptr<ZTestBase>> _sub_tests;
  vector<ZTestResult> _results;
  double _total_duration;
  int _passed;
  int _failed;

public:
  ZTestSuite(string name, ZType type, string description)
      : ZTestBase(name, type, description) {}
  ZTestSuite(const ZTestSuite &other)
      : ZTestBase(other.getName(), other.getType(), other.getDescription()) {
    for (auto &&test : other._sub_tests) {
      if (test) {
        _sub_tests.push_back(test->clone());
      }
    }
  }

  ZTestSuite &addTest(unique_ptr<ZTestBase> test) {
    _sub_tests.push_back(move(test));
    return *this;
  }

  ZTestSuite &beforeAll(function<void()> hook) {
    addBeforeAll(move(hook));
    return *this;
  }

  template <typename T> ZTestSuite &addTest(T &&test) {
    _sub_tests.push_back(forward<T>(test));
    return *this;
  }

  ZState run() override {
    _total_duration = 0.0;
    _passed = _failed = 0;
    ZTimer suite_timer;
    suite_timer.start();
    for (auto &test : _sub_tests) {
      ZTimer case_timer;
      case_timer.start();
      ZState result = test->run();
      case_timer.stop();
      runAfterEach();

      ZTestResult test_result;
      test_result.setResult(getName(), test->getType(), result, "",
                            case_timer.getStartTime(), case_timer.getEndTime(),
                            case_timer.getElapsedMilliseconds());
      _results.push_back(test_result);

      (result == ZState::z_success) ? ++_passed : ++_failed;
    }
    suite_timer.stop();
    _total_duration = suite_timer.getElapsedMilliseconds();
    setState(_failed == 0 ? ZState::z_success : ZState::z_failed);
    return getState();
  }
  unique_ptr<ZTestBase> clone() const override {
    auto cloned = make_unique<ZTestSuite>(*this);
    return cloned;
  }

  string getSummary() const {
    ostringstream oss;
    oss << "Test Suite: " << getName() << "\n"
        << "Total Cases: " << (_passed + _failed) << "\n"
        << "Passed: " << _passed << "\n"
        << "Failed: " << _failed << "\n"
        << "Total Duration: " << _total_duration << "ms\n";
    return oss.str();
  }
};
class SuiteBuilder {
private:
  std::unique_ptr<ZTestSuite> _suite;

public:
  SuiteBuilder(std::unique_ptr<ZTestSuite> suite) : _suite(std::move(suite)) {}

  SuiteBuilder &addTest(std::unique_ptr<ZTestBase> test) {
    _suite->addTest(std::move(test));
    return *this;
  }

  SuiteBuilder &beforeAll(std::function<void()> hook) {
    _suite->addBeforeAll(std::move(hook));
    return *this;
  }

  SuiteBuilder &afterEach(std::function<void()> hook) {
    _suite->addAfterEach(std::move(hook));
    return *this;
  }

  SuiteBuilder &addToRegistry() {
    if (_suite) {
      ZTestRegistry::instance().addTest(
          shared_ptr<ZTestBase>(std::move(_suite)));
    }
    return *this;
  }

  SuiteBuilder &addToContext(ZTestContext &context) {
    context.addTest(std::move(_suite));
    return *this;
  }

  std::unique_ptr<ZTestSuite> build() { return std::move(_suite); }
};
class ZTestSuiteFactory {
public:
  static SuiteBuilder createSuite(const std::string &name, ZType type,
                                  const std::string &description) {
    return SuiteBuilder(std::make_unique<ZTestSuite>(name, type, description));
  }
};