#include <gtest/gtest.h>
#include "packet.h"

TEST(basic_test, test_checksum0) {
  EXPECT_EQ("\xFF\xFF", PacketChecksum::checksum(""));
}

TEST(basic_test, test_checksum1) {
  EXPECT_EQ("\x18&", PacketChecksum::checksum("test"));
}

TEST(basic_test, test_checksum2) {
  EXPECT_EQ(PacketChecksum::checksum(" test"), PacketChecksum::checksum(" test"));
}

TEST(basic_test, test_checksum3) {
  EXPECT_NE(PacketChecksum::checksum(" test "), PacketChecksum::checksum(" test"));
}

TEST(basic_test, test_packet_to_json) {
  packet p;
  EXPECT_EQ("{\"AN\":0,\"CS\":\"\",\"OP\":0,\"PL\":\"\",\"SN\":-1}\n", p.to_json());
}

TEST(basic_test, test_packet_to_packet_from_json) {
  packet p;
  packet p2;
  p2.from_json(p.to_json());
  EXPECT_EQ(p2, p);
}
