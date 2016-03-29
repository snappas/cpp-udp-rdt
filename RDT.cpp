//
// Created by ryan on 3/27/16.
//

#include <sstream>
#include <iostream>
#include <poll.h>
#include "RDT.h"

RDT::RDT(size_t segment_size_): segment_size(segment_size_) {
  ack_received = false;
  sequence_number = 0;

}
bool RDT::listen(std::string listen_address, int listen_port) {
  recv_socket = socket(AF_INET,SOCK_DGRAM, 0);
  std::memset(&recv_addr, 0, sizeof(recv_addr));
  recv_addr.sin_family = AF_INET;
  inet_aton(listen_address.c_str(), &recv_addr.sin_addr);
  recv_addr.sin_port = htons(listen_port);
  int t = 1;
  if (setsockopt(recv_socket, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t))) {
    perror("could not set SO_REUSEADDR");
  }

  bind(recv_socket, (struct sockaddr *)&recv_addr, sizeof(recv_addr));
  return false;
}

bool RDT::connect(std::string destination_address, int destination_port) {
  send_socket = socket(AF_INET,SOCK_DGRAM,0);
  std::memset(&send_addr, 0, sizeof(send_addr));
  send_addr.sin_family = AF_INET;
  inet_aton(destination_address.c_str(), &send_addr.sin_addr);
  send_addr.sin_port = htons(destination_port);
  handshake();
  return false;
}
bool RDT::handshake() {
  while(!ack_received){
    packet syn;
    syn.checksum = "SYN";
    syn.payload = "";
    syn.seq = 0;
    std::stringstream ss;
    ss << syn.checksum << " " << syn.seq << " " << syn.payload;
    sendto(send_socket, ss.str().c_str(), strlen(ss.str().c_str()), 0, (struct sockaddr *) &send_addr, sizeof(send_addr));
    std::cout << "Sent SYN " << syn.seq << std::endl;
    struct pollfd fd;
    fd.fd = recv_socket;
    fd.events = POLLIN;
    int res = poll(&fd, 1, 80);
    if(res == 0){
      std::cout << "timeout" << std::endl;
    }else {
      char buffer[4096];
      recvfrom(recv_socket, buffer, 4096, 0, (struct sockaddr *) &recv_addr, NULL);
      std::stringstream ssin(buffer);
      packet recv;
      ssin >> recv.checksum >> recv.seq >> recv.payload;

      if (recv.checksum.compare(syn.checksum) == 0 && recv.seq == syn.seq) {
        ack_received = true;
        std::cout << "SYN-ACK " << recv.seq << " received" << std::endl;
      }
    }
  }
  return false;
}

bool RDT::send(std::string content) {
  while(content.size() > 0){
    packet pkt;
    std::string segment;
    if(segment_size > content.size()){
      segment = content;
      content = "";
    }else{
      segment.append(content, 0, segment_size);
      content.erase(0, segment_size);
    }

    pkt.payload = segment;
    pkt.seq = sequence_number;
    pkt.checksum = checksum(segment);
    std::stringstream ss;
    ss << pkt.checksum << " " << pkt.seq << " " << pkt.payload;
    ack_received = false;
    while(!ack_received){
      sendto(send_socket, ss.str().c_str(), strlen(ss.str().c_str()), 0, (struct sockaddr *) &send_addr, sizeof(send_addr));
      std::cout << "Sent seq " << pkt.seq << std::endl;
      struct pollfd fd;
      fd.fd = recv_socket;
      fd.events = POLLIN;
      int res = poll(&fd, 1, 80);
      if(res == 0){
        std::cout << "timeout" << std::endl;
      }else{
        char buffer[4096];
        recvfrom(recv_socket, buffer, 4096, 0, (struct sockaddr *) &recv_addr, NULL);
        std::stringstream ssin(buffer);
        packet recv;
        ssin >> recv.checksum >> recv.seq >> recv.payload;
        std::memset(buffer, 0, 4096);

        if(recv.checksum.compare(pkt.checksum) == 0 && recv.seq == pkt.seq && recv.payload.compare("ACK") == 0){
          ack_received = true;
          std::cout << "ACK " << recv.seq << " received"  << std::endl;
        }
      }
    }
    sequence_number = 1 - sequence_number;
  }
  fin();

  return false;
}

//checksum algorithm taken from
// https://github.com/xhacker/RDT-over-UDP/blob/master/common.py
//similar to
// https://tools.ietf.org/html/rfc1071#section-4

std::string RDT::checksum(std::string data){
  uint sum = 0;
  size_t pos = data.size();
  if(pos & 1){
    pos -= 1;
    sum = (uint) data[pos];
  }else{
    sum = 0;
  }

  while(pos > 0){
    pos -= 2;
    sum += ((uint) data[pos+1] << 8) + (uint) data[pos];
  }

  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);

  uint result;
  result = (~ sum) & 0xffff;
  result = result >> 8 | ((result & 0xff) << 8);
  std::string ret;
  ret.push_back((char) (result / 256));
  ret.push_back((char) (result % 256));
  return ret;
}

bool RDT::fin() {
  ack_received = false;
  int timeout_count = 0;
  while(!ack_received){
    packet fin;
    fin.checksum = "FIN";
    fin.payload = "";
    fin.seq = sequence_number;
    std::stringstream ss;
    ss << fin.checksum << " " << fin.seq << " " << fin.payload;
    sendto(send_socket, ss.str().c_str(), strlen(ss.str().c_str()), 0, (struct sockaddr *) &send_addr, sizeof(send_addr));

    std::cout << "Sent FIN " << fin.seq << std::endl;
    struct pollfd fd;
    fd.fd = recv_socket;
    fd.events = POLLIN;
    int res = poll(&fd, 1, 80);
    if(res == 0){
      std::cout << "timeout" << std::endl;
      timeout_count++;
      if(timeout_count == 5){
        break;
      }
    }else {
      char buffer[4096];
      recvfrom(recv_socket, buffer, 4096, 0, (struct sockaddr *) &recv_addr, NULL);
      std::stringstream ssin(buffer);
      packet recv;
      ssin >> recv.checksum >> recv.seq >> recv.payload;

      if (recv.checksum.compare(fin.checksum) == 0 && recv.seq == fin.seq) {
        ack_received = true;
        std::cout << "FIN-ACK " << recv.seq << " received" << std::endl;
      }
    }
  }
  return false;
}


std::string RDT::recv(std::string destination_address, int destination_port) {
  send_socket = socket(AF_INET,SOCK_DGRAM,0);
  std::memset(&send_addr, 0, sizeof(send_addr));
  send_addr.sin_family = AF_INET;
  inet_aton(destination_address.c_str(), &send_addr.sin_addr);
  send_addr.sin_port = htons(destination_port);
  std::string received;
  syn_received = false;
  fin_received = false;
  sequence_number = 0;
  while(fin_received == false || syn_received == false){
    packet pkt;
    socklen_t recv_len;
    char buffer[4096];
    /*
     * problem here if sender packet > 4096
     * need to keep rcving before acking
     */
    ssize_t val = recvfrom(recv_socket, &buffer, 4096, 0, (struct sockaddr *)&recv_addr, &recv_len);
    if(val == -1){
      perror("recvfrom");
    }

    std::stringstream ss(buffer);
    ss >> pkt.checksum >> pkt.seq >> pkt.payload;
    std::memset(buffer, 0, 4096);

    packet send_pkt;
    send_pkt.payload = "ACK";
    send_pkt.seq = pkt.seq;
    send_pkt.checksum = pkt.checksum;
    std::stringstream sendss;
    if((checksum(pkt.payload).compare(pkt.checksum) == 0 && pkt.seq == sequence_number)
        || (pkt.checksum.compare("SYN") == 0 || pkt.checksum.compare("FIN") == 0)){
      sendss << send_pkt.checksum << " " << send_pkt.seq << " " << send_pkt.payload;
      sendto(send_socket, sendss.str().c_str(), strlen(sendss.str().c_str()), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
      if(pkt.checksum.compare("SYN") == 0){
        sequence_number = 0;
        syn_received = true;
      } else if(pkt.checksum.compare("FIN") == 0){
        if(syn_received){
          fin_received = true;
        }
      } else if(pkt.seq == sequence_number){
        received.append(pkt.payload);
        //std::cout << pkt.payload << std::endl;
        sequence_number = 1 - sequence_number;
      }

    }else{
      send_pkt.seq = 1 - sequence_number;
      sendss << send_pkt.checksum << " " << send_pkt.seq << " " << send_pkt.payload;
      sendto(send_socket, sendss.str().c_str(), strlen(sendss.str().c_str()), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
    }
  }
  return received;
}








