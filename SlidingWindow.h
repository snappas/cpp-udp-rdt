//
// Created by ryan on 4/6/16.
//

#ifndef UDP_RDT_SLIDINGWINDOW_H
#define UDP_RDT_SLIDINGWINDOW_H
#include <queue>
#include "packet.h"


class SlidingWindow {
 public:
  SlidingWindow(size_t window_size_) : window_size(window_size_) { };
  bool enqueue_data(packet p) {
    if (data_buffer.size() < window_size) {
      data_buffer.push(p);
      return true;
    } else {
      return false;
    }
  }

  bool enqueue_ack(packet p) {
    if (ack_buffer.size() < window_size) {
      ack_buffer.push(p);
      return true;
    } else {
      return false;
    }
  }


  packet top_data() {
    return data_buffer.top();
  }

  packet top_ack() {
    return ack_buffer.top();
  }

 private:
  size_t window_size;
  std::priority_queue<packet, std::vector<packet>, PacketCompare> data_buffer;
  std::priority_queue<packet, std::vector<packet>, PacketCompare> ack_buffer;
};
#endif //UDP_RDT_SLIDINGWINDOW_H
