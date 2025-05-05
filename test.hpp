#include "./gui.hpp"
#include <any>
#include <bits/unique_ptr.h>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unistd.h>
#include <utility>
#include <vector>
using namespace std;
using namespace chrono;

class TestView;

#define EXPECT_EQ(expected, actual)                                            \
  do {                                                                         \
    auto &&_z_expected = (expected);                                           \
    auto &&_z_actual = (actual);                                               \
    if (_z_expected != _z_actual) {                                            \
      std::ostringstream _z_ess;                                               \
      _z_ess << _z_expected;                                                   \
      std::ostringstream _z_ass;                                               \
      _z_ass << _z_actual;                                                     \
      throw ZTestFailureException(this->getName(), _z_ess.str(),               \
                                  _z_ass.str());                               \
    }                                                                          \
  } while (0)
#define ASSERT_TRUE(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::ostringstream _z_oss;                                               \
      _z_oss << "false ( " << #cond << " )";                                   \
      throw ZTestFailureException(this->getName(), "true", _z_oss.str());      \
    }                                                                          \
  } while (0)

constexpr const char *green = "\033[1;32m";
constexpr const char *red = "\033[1;31m";
constexpr const char *reset = "\033[0m";

class ZLogger {
private:
  mutex _log_mutex;

public:
  void info(const string &s) {
    lock_guard<mutex> lock(_log_mutex);
    std::cout << s << std::flush;
  }

  void error(const string &s) {
    lock_guard<mutex> lock(_log_mutex);
    std::cout << s << std::flush;
  }

  void debug(const string &s) { std::cout << "[debug]:" + s; }

  ZLogger() {}
  ~ZLogger() {}
};

class ZTestFailureException : public std::exception {
private:
  string _msg;

public:
  ZTestFailureException(const string &test_name, const string &expected,
                        const string &actual) {
    ostringstream oss;
    oss << "Test Failure in " << test_name << ":\n"
        << "  Expected: " << expected << "\n"
        << "  Actual  : " << actual;
    _msg = oss.str();
  }

  const char *what() const noexcept override { return _msg.c_str(); }
};

ZLogger logger;

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

enum class ZState { z_failed, z_success, z_unknown };

enum class ZType { z_safe, z_unsafe };

const char *toString(ZState state) {
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

std::ostream &operator<<(std::ostream &os, ZState state) {
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

// ZtestInterface接口,定义了必须实现的方法。
class ZTestInterface {
public:
  explicit ZTestInterface() = default;
  virtual ~ZTestInterface() = default;
  virtual const string &getName() const = 0;
  virtual ZState run() = 0;
  virtual const ZType getType() const = 0;
};

// ZtestBase测试基类,定义了一些属性
class ZTestBase : public ZTestInterface {
private:
  string _name;
  ZType _type;
  string _description;
  ZState _state;
  vector<function<void()>> _before_all_hooks;
  vector<function<void()>> _after_each_hooks;

public:
  ZTestBase(string name, ZType type, string description)
      : _name(name), _type(type), _description(description) {}

  const string &getName() const override { return _name; }
  virtual unique_ptr<ZTestBase> clone() const = 0;
  const ZType getType() const override { return _type; }

  virtual const ZState getState() const { return _state; }

  virtual void setState(ZState state) { _state = state; }

  virtual const string getDescription() const { return _description; }

  virtual void setDescription(string description) {
    _description = description;
  }

  virtual ZTestBase &addBeforeAll(function<void()> hook) {
    _before_all_hooks.push_back(move(hook));
    return *this;
  }

  virtual void runBeforeAll() {
    for (auto &hook : _before_all_hooks) {
      hook();
    }
  }

  virtual ZTestBase &addAfterEach(function<void()> hook) {
    _after_each_hooks.push_back(move(hook));
    return *this;
  }

  virtual void runAfterEach() {
    for (auto &hook : _after_each_hooks) {
      hook();
    }
  }
};

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
// ZTestRegistry定义了测试注册中心，用于gtest类似语法。
class ZTestRegistry {
private:
  vector<unique_ptr<ZTestBase>> _tests;
  mutex _mutex;

public:
  static ZTestRegistry &instance() {
    static ZTestRegistry reg;
    return reg;
  }

  void addTest(unique_ptr<ZTestBase> test) {
    lock_guard<mutex> lock(_mutex);
    _tests.push_back(move(test));
  }

  vector<unique_ptr<ZTestBase>> takeTests() {
    lock_guard<mutex> lock(_mutex);
    vector<unique_ptr<ZTestBase>> result;
    result.swap(_tests);
    return result;
  }
};
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
    runBeforeAll();
    ZTimer suite_timer;
    suite_timer.start();
    for (auto &test : _sub_tests) {
      ZTimer case_timer;
      case_timer.start();
      ZState result = test->run();
      case_timer.stop();
      runAfterEach();

      ZTestResult test_result;
      test_result.setResult(getName(), result, "", case_timer.getStartTime(),
                            case_timer.getEndTime(),
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

#define ZTEST_F(suite_name, test_name)                                         \
  class suite_name##_##test_name : public ZTestSuite {                         \
  public:                                                                      \
    suite_name##_##test_name()                                                 \
        : ZTestSuite(#suite_name "." #test_name, ZType::z_safe, "") {}         \
    unique_ptr<ZTestBase> clone() const override {                             \
      return make_unique<suite_name##_##test_name>(*this);                     \
    }                                                                          \
    ZState run() override;                                                     \
    static void _register() {                                                  \
      ZTestRegistry::instance().addTest(                                       \
          make_unique<suite_name##_##test_name>());                            \
    }                                                                          \
  };                                                                           \
  namespace {                                                                  \
  struct suite_name##_##test_name##_registrar {                                \
    suite_name##_##test_name##_registrar() {                                   \
      suite_name##_##test_name::_register();                                   \
    }                                                                          \
  } suite_name##_##test_name##_registrar_instance;                             \
  }                                                                            \
  ZState suite_name##_##test_name::run()

// TestContext维护要管理的ZTestBase的队列，并管理测试结果。
class ZTestContext {
private:
  queue<shared_ptr<ZTestBase>> _test_queue;
  vector<shared_ptr<ZTestBase>> _test_list;
  // vector<ZTestResult> _test_result;
  mutex _result_mutex;
  mutex _queue_mutex;
  mutable mutex _list_mutex;
  TestView *_visualizer;
  std::unordered_map<std::string, ZTestResult> _test_result_map;

public:
  vector<weak_ptr<ZTestBase>> getTests() {
    lock_guard<mutex> lock(_queue_mutex);
    vector<weak_ptr<ZTestBase>> tests;
    while (!_test_queue.empty()) {
      auto shared_test =
          _test_list.emplace_back(std::move(_test_queue.front()));
      _test_queue.pop();
      tests.push_back(shared_test);
    }
    return tests;
  }
  // 运行所有 z_unsafe 测试（单线程）
  void runUnsafeOnly() {
    for (auto &test : _test_list) {
      if (test->getType() == ZType::z_unsafe) {
        try {
          ZTimer timer;
          timer.start();
          test->run();
          timer.stop();

          ZTestResult result;
          result.setResult(test->getName(), ZState::z_success, "",
                           timer.getStartTime(), timer.getEndTime(),
                           timer.getElapsedMilliseconds());

          std::lock_guard<std::mutex> lock(_result_mutex);
          _test_result_map[test->getName()] = std::move(result);
        } catch (const std::exception &e) {
          ZTestResult result;
          result.setResult(test->getName(), ZState::z_failed, e.what(), {}, {},
                           0);

          std::lock_guard<std::mutex> lock(_result_mutex);
          _test_result_map[test->getName()] = std::move(result);
        }
      }
    }
  }

  // 并行运行 z_safe 测试
  void runSafeInParallel() {
    std::queue<shared_ptr<ZTestBase>> safe_queue;

    {
      std::lock_guard<std::mutex> lock(_list_mutex);
      for (auto &test : _test_list) {
        if (test->getType() == ZType::z_safe) {
          safe_queue.push(test);
        }
      }
    }

    std::vector<std::thread> workers;
    const unsigned num_workers = std::thread::hardware_concurrency();

    auto worker_task = [this, &safe_queue] {
      while (!safe_queue.empty()) {
        shared_ptr<ZTestBase> test;
        {
          std::lock_guard<std::mutex> lock(_list_mutex);
          if (!safe_queue.empty()) {
            test = safe_queue.front();
            safe_queue.pop();
          }
        }
        if (test) {
          runTest(test); // 已有锁保护
        }
      }
    };

    for (unsigned i = 0; i < num_workers; ++i)
      workers.emplace_back(worker_task);

    for (auto &t : workers)
      if (t.joinable())
        t.join();
  }
  void clearTestQueue() {
    std::lock_guard<std::mutex> q_lock(_queue_mutex);
    std::queue<shared_ptr<ZTestBase>> empty;
    std::swap(_test_queue, empty);
  }

  void setVisualizer(TestView *visualizer) { _visualizer = visualizer; }

  void addTest(unique_ptr<ZTestBase> test_case) {
    lock_guard<mutex> q_lock(_queue_mutex);
    lock_guard<mutex> l_lock(_list_mutex);

    auto shared_test = shared_ptr<ZTestBase>(std::move(test_case));
    _test_list.push_back(shared_test);
    _test_queue.push(std::move(shared_test));
  }
  vector<weak_ptr<ZTestBase>> getTestList() const {
    lock_guard<mutex> lock(_list_mutex); // Now works with mutable
    vector<weak_ptr<ZTestBase>> result;
    for (const auto &test : _test_list) {
      result.emplace_back(test);
    }
    return result;
  }
  void runTest(shared_ptr<ZTestBase> test_case) {
    ZTestResult result;
    ZTimer local_timer;
    auto *test_ptr = test_case.get();
    const string test_name = test_ptr->getName();

    try {
      local_timer.start();
      auto test_state = test_ptr->run();
      local_timer.stop();

      lock_guard<mutex> lock(_result_mutex);
      result.setResult(test_ptr->getName(), test_state, "",
                       local_timer.getStartTime(), local_timer.getEndTime(),
                       local_timer.getElapsedMilliseconds());

      logger.info(result.getResultString(test_name) + "\n");
    } catch (const exception &e) {
      lock_guard<mutex> lock(_result_mutex);
      result.setResult(test_ptr->getName(), ZState::z_failed, e.what(),
                       local_timer.getStartTime(), local_timer.getEndTime(),
                       local_timer.getElapsedMilliseconds());
      logger.error(result.getResultString(test_name) + "\n");
    }
    lock_guard<mutex> lock(_result_mutex);
    _test_result_map[test_name] = std::move(result);
  }
  const ZTestResult &getTestResult(const string &test_name) const {
    return _test_result_map.at(test_name);
  }

  const std::unordered_map<std::string, ZTestResult> &getAllResults() const {
    return _test_result_map;
  }
  void runAll() {
    vector<thread> workers;
    const unsigned num_workers = thread::hardware_concurrency();

    auto worker_task = [this] {
      while (true) {
        shared_ptr<ZTestBase> test;
        {
          lock_guard<mutex> lock(_queue_mutex);
          if (_test_queue.empty())
            return;
          test = _test_queue.front();
          _test_queue.pop();
        }
        runTest(move(test));
      }
    };
    for (unsigned i = 0; i < num_workers; ++i)
      workers.emplace_back(worker_task);
    for (auto &t : workers) {
      if (t.joinable())
        t.join();
    }
  }
  void resetQueue() {
    std::lock_guard<std::mutex> q_lock(_queue_mutex);
    std::lock_guard<std::mutex> l_lock(_list_mutex);

    _test_queue = {};
    for (auto &test : _test_list) {
      _test_queue.push(test);
    }
  }
  ZTestContext() {
    while (!_test_queue.empty())
      _test_queue.pop();
    _test_result_map.clear();
  }

  ~ZTestContext() = default;
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
    ZTestRegistry::instance().addTest(std::move(_suite));
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