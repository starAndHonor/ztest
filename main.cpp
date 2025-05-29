#include "./ztest/gui.hpp"
int add(int a, int b) {
  sleep(1);
  return a + b;
}
int subtract(int a, int b) { return a - b; }
ZTEST_F(BasicMath, FailedAdditionTest) {
  EXPECT_EQ(6, add(2, 3));
  return ZState::z_success;
}
ZTEST_F(BasicMath, FailedSubtractionTest) {
  EXPECT_EQ(2, subtract(5, 3));
  return ZState::z_success;
}

ZTEST_F(BasicMath, didsfSubtractionTest) {
  EXPECT_EQ(2, subtract(5, 3));
  return ZState::z_success;
}
ZTEST_F(BasicMath, sadfgret) {
  EXPECT_EQ(4, add(5, 3));
  return ZState::z_success;
}
ZTEST_F(BasicMath, C) {
  EXPECT_EQ(2, subtract(5, 3));
  return ZState::z_success;
}
ZTEST_F(Performance, MemoryAlloc, unsafe) {
  const size_t MB100 = 100 * 1024 * 1024;
  auto ptr = std::make_unique<char[]>(MB100);
  ASSERT_TRUE(ptr != nullptr);
  EXPECT_EQ(3, subtract(5, 3));
  EXPECT_EQ(6, add(2, 3));
  return ZState::z_success;
}
void createSingleTestCase() {
  // Use TestBuilder to construct test
  auto test =
      TestFactory::createTest("AdditionTest",                // Test name
                              ZType::z_safe,                 // Execution type
                              "Test addition functionality", // Description
                              add, 2, 3 // Function and arguments
                              )
          .setExpectedOutput(5) // Set expected result
          .beforeAll([]() {     // Setup hook
            std::cout << "Setting up single test..." << std::endl;
          })
          .afterEach([]() { // Teardown hook
            std::cout << "Cleaning up after test..." << std::endl;
          })
          .withDescription("Verify basic addition")
          .registerTest()
          .build(); // Register with test
}
void createTestSuite() {
  // Create individual test cases
  auto test1 = TestFactory::createTest("Addition", ZType::z_safe, "", add, 2, 2)
                   .setExpectedOutput(4)
                   .build();

  auto test2 =
      TestFactory::createTest("Subtraction", ZType::z_safe, "", subtract, 5, 3)
          .setExpectedOutput(2)
          .build();

  // Create test suite
  auto suite =
      ZTestSuiteFactory::createSuite("MathOperations", ZType::z_safe,
                                     "Test math operations")
          .addTest(std::move(test1))
          .addTest(std::move(test2))
          .beforeAll([]() {
            std::cout << "Global setup for all tests in suite" << std::endl;
          })
          .afterEach(
              []() { std::cout << "Cleanup after each test" << std::endl; })
          .addToRegistry()
          .build(); // Register the
}
ZBENCHMARK(Vector, PushBack) {
  std::vector<int> v;
  for (int i = 0; i < 10000; ++i) {
    v.push_back(i);
  }
  return ZState::z_success;
}
int main(int argc, char *argv[]) {
  std::vector<std::string> args(argv + 1, argv + argc);
  ZTestContext context;
  bool runGui = true;

  // Parse CLI args
  for (const auto &arg : args) {
    if (arg == "--no-gui") {
      runGui = false;
    }
  }

  createSingleTestCase();
  createTestSuite();

  if (!runGui) {
    return runFromCLI(args, context);
  }

  return showUI();
}