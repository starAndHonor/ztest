#pragma once
// TODO: 增加BENCHMARK
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

#define ZBENCHMARK(suite_name, test_name)                                      \
  class suite_name##_##test_name##_Benchmark : public ZBenchMark {             \
  public:                                                                      \
    suite_name##_##test_name##_Benchmark()                                     \
        : ZBenchMark(#suite_name "." #test_name) {}                            \
    ZState run() override;                                                     \
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
  ZState suite_name##_##test_name##_Benchmark::run()
