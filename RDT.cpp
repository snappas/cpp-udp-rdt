//
// Created by ryan on 3/27/16.
//

#include <sstream>
#include <iostream>
#include <poll.h>
#include <random>
#include "RDT.h"

RDT::RDT(size_t segment_size_) : segment_size(segment_size_) {
  ack_received = false;
  sequence_number = 0;
}

bool RDT::listen(std::string listen_address, int listen_port) {
  recv_socket = socket(AF_INET, SOCK_DGRAM, 0);
  std::memset(&recv_addr, 0, sizeof(recv_addr));
  recv_addr.sin_family = AF_INET;
  inet_aton(listen_address.c_str(), &recv_addr.sin_addr);
  recv_addr.sin_port = htons(listen_port);
  int t = 1;
  if (setsockopt(recv_socket, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t))) {
    perror("could not set SO_REUSEADDR");
  }

  bind(recv_socket, (struct sockaddr *) &recv_addr, sizeof(recv_addr));
  return false;
}

bool RDT::connect(std::string destination_address, int destination_port) {
  establish_destination(destination_address, destination_port);
  handshake();
  return false;
}

void RDT::establish_destination(std::string destination_address, int destination_port) {
  send_socket = socket(AF_INET, SOCK_DGRAM, 0);
  std::memset(&send_addr, 0, sizeof(send_addr));
  send_addr.sin_family = AF_INET;
  inet_aton(destination_address.c_str(), &send_addr.sin_addr);
  send_addr.sin_port = htons(destination_port);
}

bool RDT::handshake() {
  std::random_device random;
  std::uniform_int_distribution<int> dist(1, 2147483647);
  sequence_number = dist(random) % 65536;
  while (!ack_received) {
    packet syn(PacketOptions::SYN, sequence_number, "");
    send_pkt(syn);

    std::cout << "Sent SYN " << syn.seqnum << std::endl;
    if (!timeout(recv_socket)) {
      packet recv = recv_pkt();
      if (is_ack(recv, syn) && valid_packet(recv, syn)) {
        ack_received = true;
        std::cout << "SYN-ACK " << recv.seqnum << " received" << std::endl;
      }
    } else {
      std::cout << "timeout" << std::endl;
    }
  }
  return false;
}

bool RDT::send(std::string content) {
  while (content.size() > 0) {
    std::string segment;
    if (segment_size > content.size()) {
      segment = content;
      content = "";
    } else {
      segment.append(content, 0, segment_size);
      content.erase(0, segment_size);
    }
    packet pkt(PacketOptions::DATA, sequence_number, segment);

    ack_received = false;

    while (!ack_received) {
      send_pkt(pkt);
      std::cout << "Sent seq " << pkt.seqnum << std::endl;
      if (!timeout(recv_socket)) {
        packet recv = recv_pkt();
        std::cout << recv.to_json();
        if (is_ack(recv, pkt) && valid_packet(recv, pkt)) {
          ack_received = true;
          std::cout << "ACK " << recv.seqnum << " received" << std::endl;
        }

      } else {
        std::cout << "timeout" << std::endl;
      }
    }
    sequence_number += segment.size();
    sequence_number %= 65536;
  }
  fin();

  return false;
}

bool RDT::fin() {
  ack_received = false;
  int timeout_count = 0;
  while (!ack_received) {
    packet fin(PacketOptions::FIN, sequence_number, "");
    send_pkt(fin);
    std::cout << "Sent FIN " << fin.seqnum << std::endl;
    if (!timeout(recv_socket)) {
      packet recv = recv_pkt();
      if (is_ack(recv, fin) && valid_packet(recv, fin)) {
        ack_received = true;
        std::cout << "FIN-ACK " << recv.seqnum << " received" << std::endl;
      }
    } else {
      std::cout << "timeout" << std::endl;
      timeout_count++;
      if (timeout_count == 5) {
        break;
      }
    }
  }
  return false;
}

std::string RDT::recv(std::string destination_address, int destination_port) {
  establish_destination(destination_address, destination_port);
  std::string received;
  syn_received = false;
  fin_received = false;
  sequence_number = 0;
  while (!fin_received || !syn_received) {
    packet pkt = recv_pkt();
    packet ack_pkt = make_ack(pkt);
    if (valid_packet(ack_pkt, pkt)) {
      //checksum valid
      if (pkt.options & PacketOptions::SYN) {
        sequence_number = pkt.seqnum;
        syn_received = true;
      } else if (pkt.options & PacketOptions::FIN) {
        if (syn_received) {
          fin_received = true;
        }
      } else {
        if (pkt.seqnum == sequence_number) {
          //got expected sequence number
          received.append(pkt.payload);
          sequence_number += pkt.payload.size();
          sequence_number %= 65536;

        } else {
          //ack_pkt.seq = sequence_number;
        }
      }


    } else {
      //invalid checksum
      ack_pkt.seqnum = sequence_number;
    }
    ack_pkt.acknum = (uint) sequence_number;
    send_pkt(ack_pkt);
  }
  return received;
}

void RDT::send_pkt(packet pkt) {
  std::string send_string = pkt.to_json();
  sendto(send_socket,
         &send_string[0],
         send_string.size(),
         0,
         (struct sockaddr *) &send_addr,
         sizeof(send_addr));
}

packet RDT::recv_pkt() {
  packet recv;
  char buffer[4096];
  recvfrom(recv_socket, buffer, 4096, 0, (struct sockaddr *) &recv_addr, NULL);
  recv.from_json(buffer);
  std::memset(buffer, 0, 4096);
  return recv;
}

packet RDT::make_ack(packet pkt) {
  packet
      ack_pkt(pkt.options | PacketOptions::ACK, PacketChecksum::checksum(pkt.payload), pkt.seqnum, pkt.payload.size());
  return ack_pkt;
}

bool RDT::timeout(int socket) {
  struct pollfd fd;
  fd.fd = socket;
  fd.events = POLLIN;
  int res = poll(&fd, 1, 80);
  return res == 0;
}

bool RDT::is_ack(packet response, packet request) {
  return ((response.options & PacketOptions::ACK) != 0 &&
      (response.options & request.options) != 0);
}

bool RDT::valid_packet(packet response, packet request) {
  return ((response.checksum.compare(request.checksum) == 0) &&
      response.seqnum == request.seqnum &&
      response.seqnum > -1);
}





















