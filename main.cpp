
#include "./ztest/gui.hpp"
#include <any>
int add(int a, int b) { return a + b; }
double subtract(double a, double b) { return a - b; }
ZTEST_F(ASSERTION, FailedEXPECT_EQ) {
  EXPECT_EQ(6, add(2, 3));
  return ZState::z_success;
}
ZTEST_F(ASSERTION, SuccessEXPECT_EQ) {
  EXPECT_EQ(5, add(2, 3));
  return ZState::z_success;
}
ZTEST_F(ASSERTION, SuccessEXPECT_NEAR) {
  EXPECT_NEAR(2, subtract(5.0, 3.0), 0.001);
  return ZState::z_success;
}

ZTEST_F(ASSERTION, FailedEXPECT_NEAR) {
  EXPECT_NEAR(2, subtract(5.1, 3.0), 0.001);
  return ZState::z_success;
}

ZTEST_F(ASSERTION, FailedASSERT_TRUE) {
  ASSERT_TRUE(false);
  return ZState::z_success;
}
// ZTEST_F(ASSERTION, SuccessASSERT_TRUE) {
//   ASSERT_TRUE(true);
//   return ZState::z_success;
// }
// ZTEST_F(RUN, safe_test_single_case1, safe) {
//   sleep(2);
//   ASSERT_TRUE(true);
//   return ZState::z_success;
// }
// ZTEST_F(RUN, safe_test_single_case2, safe) {
//   sleep(1);
//   ASSERT_TRUE(true);
//   return ZState::z_success;
// }
// ZTEST_F(RUN, safe_test_single_case3, safe) {
//   sleep(3);
//   ASSERT_TRUE(true);
//   return ZState::z_success;
// }
// ZTEST_F(RUN, unsafe_test_single_case1, unsafe) {
//   sleep(1);
//   EXPECT_EQ(false, false);
//   return ZState::z_success;
// }
// ZTEST_F(RUN, unsafe_test_single_case2, unsafe) {
//   sleep(2);
//   EXPECT_EQ(false, false);
//   return ZState::z_success;
// }
// ZTEST_F(RUN, unsafe_test_single_case3, unsafe) {
//   sleep(3);
//   EXPECT_EQ(false, false);
//   return ZState::z_success;
// }
// ZTestDataManager<vector<int>, int> sum_test_data = {
//     {{1, 2}, 3}, {{-1, 1}, 0}, {{10, 20}, 30}};
// ZTEST_P(ArithmeticSuite, SumTest, sum_test_data) {
//   auto &&[inputs, expected] = _data.current();
//   int actual = inputs[0] + inputs[1];
//   EXPECT_EQ_FOREACH(expected, actual);
//   return ZState::z_success;
// }
// ZTestDataManager<tuple<float, int>, float> sum_test_data2 = {
//     {{1.2, 2}, 3.2}, {{-1.0, 1}, 0.0}, {{10.1, 20}, 30.1}};
// ZTEST_P(ArithmeticSuite, SumTestfordiff, sum_test_data2) {
//   auto &&[inputs, expected] = _data.current();
//   float actual = std::get<0>(inputs) + std::get<1>(inputs);
//   EXPECT_EQ_FOREACH(expected, actual);
//   return ZState::z_success;
// }
// ZTEST_P_CSV(MathTests, AdditionTests, "data.csv") {
//   auto inputs = getInput();
//   auto expected = getOutput();
//   double actual = std::get<double>(inputs[0]) + std::get<double>(inputs[1]);
//   EXPECT_EQ(actual, std::get<double>(expected));
//   return ZState::z_success;
// }
// ZBENCHMARK(Vector, PushBack) {
//   std::vector<int> v;
//   for (int i = 0; i < 10000; ++i) {
//     v.push_back(i);
//   }
//   return ZState::z_success;
// }
// ZBENCHMARK(Matrix, PushBack, 20000) {
//   std::vector<int> v;
//   for (int i = 0; i < 1000; ++i) {
//     v.push_back(random());
//   }
//   return ZState::z_success;
// }
int main(int argc, char *argv[]) {
  std::vector<std::string> args(argv + 1, argv + argc);
  ZTestContext context;
  bool runGui = true;
  logger.set_level(ZLogLevel::INFO);
  // Parse CLI args
  for (const auto &arg : args) {
    if (arg == "--no-gui") {
      runGui = false;
    }
  }
  ZDataRegistry::instance().setMaxSize(1);
  if (!runGui) {
    return runFromCLI(args, context);
  }

  return showUI();
}