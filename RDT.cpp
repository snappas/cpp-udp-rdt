//
// Created by ryan on 3/27/16.
//

#include <sstream>
#include <iostream>
#include <poll.h>
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
  while (!ack_received) {
    packet syn("SYN", 0, "");
    send_pkt(syn);

    std::cout << "Sent SYN " << syn.seq << std::endl;
    if (!timeout(recv_socket)) {
      packet recv = recv_pkt();
      if (recv.checksum.compare(syn.checksum) == 0 && recv.seq == syn.seq) {
        ack_received = true;
        std::cout << "SYN-ACK " << recv.seq << " received" << std::endl;
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
    packet pkt(checksum(segment), sequence_number, segment);

    ack_received = false;

    while (!ack_received) {
      send_pkt(pkt);
      std::cout << "Sent seq " << pkt.seq << std::endl;
      if (!timeout(recv_socket)) {
        packet recv = recv_pkt();
        std::cout << "recv'd checksum: " << recv.checksum << " seq: " << recv.seq << " payload: " << recv.payload
            << std::endl;
        if (recv.checksum.compare(pkt.checksum) == 0 && recv.seq == pkt.seq && recv.payload.compare("ACK") == 0) {
          ack_received = true;
          std::cout << "ACK " << recv.seq << " received" << std::endl;
        }

      } else {
        std::cout << "timeout" << std::endl;
      }
    }
    sequence_number = 1 - sequence_number;
  }
  fin();

  return false;
}

bool RDT::fin() {
  ack_received = false;
  int timeout_count = 0;
  while (!ack_received) {
    packet fin("FIN", sequence_number, "");
    send_pkt(fin);
    std::cout << "Sent FIN " << fin.seq << std::endl;
    if (!timeout(recv_socket)) {
      packet recv = recv_pkt();
      if (recv.checksum.compare(fin.checksum) == 0 && recv.seq == fin.seq) {
        ack_received = true;
        std::cout << "FIN-ACK " << recv.seq << " received" << std::endl;
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

    if ((checksum(pkt.payload).compare(pkt.checksum) == 0 && pkt.seq == sequence_number)
        || (pkt.checksum.compare("SYN") == 0 || pkt.checksum.compare("FIN") == 0)) {
      send_pkt(ack_pkt);
      if (pkt.checksum.compare("SYN") == 0) {
        sequence_number = 0;
        syn_received = true;
      } else if (pkt.checksum.compare("FIN") == 0) {
        if (syn_received) {
          fin_received = true;
        }
      } else if (pkt.seq == sequence_number) {
        received.append(pkt.payload);
        sequence_number = 1 - sequence_number;
      }

    } else {
      ack_pkt.seq = 1 - sequence_number;
      send_pkt(ack_pkt);
    }
  }
  return received;
}

void RDT::send_pkt(packet pkt) {
  std::string send_string = pkt.to_string();
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
  std::stringstream ss(buffer);
  ss >> recv.checksum >> recv.seq >> recv.payload;
  std::memset(buffer, 0, 4096);
  return recv;
}

packet RDT::make_ack(packet pkt) {
  packet ack_pkt;
  ack_pkt.payload = "ACK";
  ack_pkt.seq = pkt.seq;
  ack_pkt.checksum = pkt.checksum;
  return ack_pkt;
}

bool RDT::timeout(int socket) {
  struct pollfd fd;
  fd.fd = socket;
  fd.events = POLLIN;
  int res = poll(&fd, 1, 80);
  return res == 0;
}


//checksum algorithm taken from
// https://github.com/xhacker/RDT-over-UDP/blob/master/common.py
//similar to
// https://tools.ietf.org/html/rfc1071#section-4

std::string RDT::checksum(std::string data) {
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

















