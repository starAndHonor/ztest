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
ZTEST_F(Performance, MemoryAlloc) {
  const size_t MB100 = 100 * 1024 * 1024;
  auto ptr = std::make_unique<char[]>(MB100);
  ASSERT_TRUE(ptr != nullptr);
  EXPECT_EQ(3, subtract(5, 3));
  EXPECT_EQ(6, add(2, 3));
  return ZState::z_success;
}

int main() { return showUI(); }