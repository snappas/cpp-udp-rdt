//
// Created by ryan on 4/5/16.
//

#ifndef UDP_RDT_PACKET_H
#define UDP_RDT_PACKET_H

#include <jsoncpp/json/json.h>
#include <string>


namespace PacketOptions {
constexpr uint
    ACK = (1 << 0),
    SYN = (2 << 0),
    FIN = (4 << 0),
    DATA = (8 << 0);
};

namespace PacketChecksum {
static std::string checksum(std::string data) {
  uint sum = 0;
  size_t pos = data.size();
  if (pos & 1) {
    pos -= 1;
    sum = (uint) data[pos];
  } else {
    sum = 0;
  }

  while (pos > 0) {
    pos -= 2;
    sum += ((uint) data[pos + 1] << 8) + (uint) data[pos];
  }

  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);

  uint result;
  result = (~sum) & 0xffff;
  result = result >> 8 | ((result & 0xff) << 8);
  std::string ret;
  ret.push_back((char) (result / 256));
  ret.push_back((char) (result % 256));
  return ret;
}
};

struct packet {
  uint options;
  uint acknum;
  std::string checksum;
  int seqnum;
  std::string payload;

  packet() {
    acknum = 0;
    seqnum = -1;
    options = 0;
  }

  packet(uint options_, std::string checksum_, int seq_, size_t payload_size) {
    checksum = checksum_;
    seqnum = seq_;
    payload = "";
    options = options_;
    acknum = (uint) (seq_ + payload_size);
  }
  packet(uint options_, int seq_, std::string payload_) {
    checksum = PacketChecksum::checksum(payload_);
    seqnum = seq_;
    payload = payload_;
    options = options_;
    acknum = 0;
  }

  std::string to_json() {
    Json::Value root;
    Json::FastWriter writer;
    root["AN"] = acknum;
    root["OP"] = options;
    root["CS"] = checksum;
    root["SN"] = seqnum;
    root["PL"] = payload;
    return writer.write(root);
  }

  void from_json(std::string buffer) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(buffer, root);
    try {
      acknum = root.get("AN", 0).asUInt();
      options = root.get("OP", 0).asUInt();
      seqnum = root.get("SN", 0).asInt();
      checksum = root.get("CS", "").asString();
      payload = root.get("PL", "").asString();
    } catch (std::exception &e) {
      acknum = 0;
      options = 0;
      seqnum = -1;
      checksum = "";
      payload = "";
    }

  }

  bool operator==(const packet &rhs) const {
    return ((options == rhs.options) &&
        (seqnum == rhs.seqnum) &&
        (checksum.compare(rhs.checksum) == 0) &&
        (payload.compare(rhs.payload) == 0) &&
        (acknum == rhs.acknum));
  }

};

class PacketCompare {
 public:
  bool operator()(packet first, packet second) {
    return first.seqnum > second.seqnum;
  }
};
#endif //UDP_RDT_PACKET_H
