//
// Created by ryan on 3/27/16.
//

#ifndef UDP_RDT_SENDER_RDT_H
#define UDP_RDT_SENDER_RDT_H


#include <cstdio>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <jsoncpp/json/json.h>

enum class PacketOptions {
  ACK = (1 << 0),
  SYN = (2 << 0),
  FIN = (3 << 0),
  DAT = (4 << 0)
};

struct packet{
  uint options;
  std::string checksum;
  int seq;
  std::string payload;

  packet(){
    seq = -1;
  }

  packet(std::string checksum_, int seq_, std::string payload_) {
    checksum = checksum_;
    seq = seq_;
    payload = payload_;
  }

  std::string to_json() {
    Json::Value root;
    Json::FastWriter writer;
    root["options"] = options;
    root["checksum"] = checksum;
    root["seq"] = seq;
    root["payload"] = payload;
    return writer.write(root);
  }

  void from_json(char *buffer) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(buffer, root);
    options = root.get("options", 0).asUInt();
    seq = root.get("seq", 0).asInt();
    checksum = root.get("checksum", "").asString();
    payload = root.get("payload", "").asString();
  }

};

class RDT {
 public:
  RDT(size_t segment_size);
  bool listen(std::string listen_address, int listen_port);
  bool connect(std::string destination_address, int destination_port);
  bool send(std::string data);
  std::string recv(std::string destination_address, int destination_port);
 private:
  bool ack_received;
  bool fin_received;
  bool syn_received;
  size_t segment_size;
  sockaddr_in recv_addr;
  sockaddr_in send_addr;
  int recv_socket;
  int send_socket;
  int sequence_number;


  bool handshake();
  std::string checksum(std::string data);
  bool fin();
  void send_pkt(packet pkt);
  packet recv_pkt();
  bool timeout(int socket);
  packet make_ack(packet pkt);
  void establish_destination(std::string destination_address, int destination_port);
};


#endif //UDP_RDT_SENDER_RDT_H
