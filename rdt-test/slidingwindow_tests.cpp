#include <gtest/gtest.h>
#include "SlidingWindow.h"

TEST(test_sliding_window, test1) {
  SlidingWindow w(3);
  packet p1;
  p1.seqnum = 1;
  packet p2;
  p2.seqnum = 2;
  packet p3;
  p3.seqnum = 3;
  w.enqueue_data(p1);
  w.enqueue_data(p2);
  w.enqueue_data(p3);
  EXPECT_EQ(p1, w.top_data());
}

TEST(test_sliding_window, test2) {

}
