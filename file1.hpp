
#pragma once
#include "ztest/gui.hpp"
int q() { return 1; }
ZTEST_F(q_qwq, qwq) {
  EXPECT_EQ(6, 6);
  return ZState::z_success;
}
