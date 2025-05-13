#pragma once
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
  } __attribute__((used)) suite_name##_##test_name##_registrar_instance;       \
  }                                                                            \
  ZState suite_name##_##test_name::run()
