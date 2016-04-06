#include <gtest/gtest.h>

TEST(basic_test, test_eq) {
  EXPECT_EQ(1, 1);
}

TEST(basic_test, test_neq) {
  EXPECT_NE(1, 0);
}

