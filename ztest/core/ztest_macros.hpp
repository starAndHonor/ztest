#pragma once
#include <cmath>
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
#define EXPECT_NEAR(expected, actual, epsilon)                                 \
  do {                                                                         \
    auto &&_z_expected = (expected);                                           \
    auto &&_z_actual = (actual);                                               \
    auto &&_z_epsilon = (epsilon);                                             \
    if (std::abs(_z_expected - _z_actual) > _z_epsilon) {                      \
      std::ostringstream _z_ess;                                               \
      _z_ess << _z_expected;                                                   \
      std::ostringstream _z_ass;                                               \
      _z_ass << _z_actual;                                                     \
      std::ostringstream _z_eps;                                               \
      _z_eps << _z_epsilon;                                                    \
      std::ostringstream _z_diff;                                              \
      _z_diff << std::abs(_z_expected - _z_actual);                            \
      throw ZTestFailureException(                                             \
          this->getName(),                                                     \
          ("Expected value near " + _z_ess.str() + " ± " + _z_eps.str())       \
              .c_str(),                                                        \
          ("Actual value was " + _z_ass.str() + " (diff = " + _z_diff.str() +  \
           ")")                                                                \
              .c_str());                                                       \
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
// TODO: 修正HOOKS运行的位置
#define BEFOREALL(func) addBeforeAll([this]() { func; })
#define AFTEREACH(func) addAfterEach([this]() { func; })
#define AFTERALL(func) addAfterAll([this]() { func; })

#define ZTEST_F(...) ZTEST_IMPL(__VA_ARGS__, ZTEST_F3, ZTEST_F2)(__VA_ARGS__)
#define ZTEST_IMPL(_1, _2, _3, NAME, ...) NAME
#define ZTEST_F2(suite_name, test_name) ZTEST_F3(suite_name, test_name, safe)
#define ZTEST_F3(suite_name, test_name, type)                                  \
  class suite_name##_##test_name : public ZTestBase {                          \
  public:                                                                      \
    suite_name##_##test_name()                                                 \
        : ZTestBase(#suite_name "." #test_name, ZType::z_##type, "") {}        \
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
  } __attribute__((used)) suite_name##_##test_name##_registrar_instance;       \
  }                                                                            \
  ZState suite_name##_##test_name::run()

#define ZBENCHMARK(...)                                                        \
  ZBENCHMARK_IMPL(__VA_ARGS__, ZBENCHMARK3, ZBENCHMARK2)(__VA_ARGS__)
#define ZBENCHMARK_IMPL(_1, _2, _3, NAME, ...) NAME
#define ZBENCHMARK2(suite_name, test_name)                                     \
  ZBENCHMARK3(suite_name, test_name, 1000)
#define ZBENCHMARK3(suite_name, test_name, iterations)                         \
  class suite_name##_##test_name##_Benchmark : public ZBenchMark {             \
  public:                                                                      \
    suite_name##_##test_name##_Benchmark()                                     \
        : ZBenchMark(#suite_name "." #test_name) {                             \
      withIterations(iterations);                                              \
    }                                                                          \
    ZState run_single_case() override;                                         \
    std::unique_ptr<ZTestBase> clone() const override {                        \
      return std::make_unique<suite_name##_##test_name##_Benchmark>(*this);    \
    }                                                                          \
    static void _register() {                                                  \
      ZTestRegistry::instance().addTest(                                       \
          std::make_unique<suite_name##_##test_name##_Benchmark>());           \
    }                                                                          \
  };                                                                           \
  namespace {                                                                  \
  struct suite_name##_##test_name##_Benchmark_registrar {                      \
    suite_name##_##test_name##_Benchmark_registrar() {                         \
      suite_name##_##test_name##_Benchmark::_register();                       \
    }                                                                          \
  } __attribute__((                                                            \
      used)) suite_name##_##test_name##_Benchmark_registrar_instance;          \
  }                                                                            \
  ZState suite_name##_##test_name##_Benchmark::run_single_case()

#define ZTEST_P(suite, test, data_manager)                                     \
  class suite##_##test                                                         \
      : public ZTestParameterized<                                             \
            typename std::decay_t<decltype(data_manager)>::Input,              \
            typename std::decay_t<decltype(data_manager)>::Output> {           \
  public:                                                                      \
    using ParamType = std::decay_t<decltype(data_manager)>;                    \
    suite##_##test()                                                           \
        : ZTestParameterized(#suite "." #test, ZType::z_param, "",             \
                             data_manager) {}                                  \
    unique_ptr<ZTestBase> clone() const override {                             \
      return make_unique<suite##_##test>(*this);                               \
    }                                                                          \
    ZState run_single_case() override;                                         \
    static void _register() {                                                  \
      ZTestRegistry::instance().addTest(make_unique<suite##_##test>());        \
    }                                                                          \
  };                                                                           \
  namespace {                                                                  \
  struct suite##_##test##_registrar {                                          \
    suite##_##test##_registrar() { suite##_##test::_register(); }              \
  } suite##_##test##_instance;                                                 \
  }                                                                            \
  ZState suite##_##test::run_single_case()

#define EXPECT_EQ_FOREACH(expected_member, actual_member)                      \
  do {                                                                         \
    const auto &expected_val = expected_member;                                \
    const auto &actual_val = actual_member;                                    \
    EXPECT_EQ(expected_val, actual_val);                                       \
  } while (0)
#define ZTEST_P_CSV(suite, test, csv_file_path)                                \
  class suite##_##test                                                         \
      : public ZTestParameterized<std::vector<CSVCell>, CSVCell> {             \
  public:                                                                      \
    suite##_##test()                                                           \
        : ZTestParameterized(                                                  \
              #suite "." #test, ZType::z_param, "",                            \
              *ZDataRegistry::instance().load<ZTestCSVDataManager>(            \
                  csv_file_path)) {}                                           \
    unique_ptr<ZTestBase> clone() const override {                             \
      return make_unique<suite##_##test>(*this);                               \
    }                                                                          \
    ZState run_single_case() override;                                         \
    static void _register() {                                                  \
      ZTestRegistry::instance().addTest(make_unique<suite##_##test>());        \
    }                                                                          \
    std::vector<CSVCell> getInput() const { return _data.current().first; }    \
    CSVCell getOutput() const { return _data.current().second; }               \
                                                                               \
  private:                                                                     \
    using Base = ZTestParameterized<std::vector<CSVCell>, CSVCell>;            \
    using Base::_data;                                                         \
  };                                                                           \
  namespace {                                                                  \
  struct suite##_##test##_registrar {                                          \
    suite##_##test##_registrar() { suite##_##test::_register(); }              \
  } suite##_##test##_instance;                                                 \
  }                                                                            \
  ZState suite##_##test::run_single_case()
