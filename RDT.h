//
// Created by ryan on 3/27/16.
//

#ifndef UDP_RDT_SENDER_RDT_H
#define UDP_RDT_SENDER_RDT_H


#include <cstdio>
#include <cstring>
#include <string>
#include <arpa/inet.h>

struct packet{
  std::string checksum;
  int seq;
  std::string payload;
  packet(){
    seq = -1;
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

};


#endif //UDP_RDT_SENDER_RDT_H
