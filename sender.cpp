#include <iostream>
#include <chrono>
#include "RDT.h"


int main(int argc, char **argv) {

  if(argc != 5){
    std::cout << "usage: " << argv[0] << "dest_addr dest_port listen_addr listen_port" << std::endl;
    exit(1);
  }

  std::string content;
  int content_size = 1024;
  for(int i = 0; i < content_size; i++){
    content.push_back('b');
  }

  size_t segment_size = 64;

  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();

  RDT rdt(segment_size);
  rdt.listen(argv[3], std::atoi(argv[4]));
  rdt.connect(argv[1], std::atoi(argv[2]));
  rdt.send(content);

  end = std::chrono::system_clock::now();

  std::chrono::duration<double> elapsed_seconds = end-start;

  std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
  std::cout << "throughput: " << ( content_size / elapsed_seconds.count()) / 1024.0 << " Kbytes/sec" << std::endl;

  return 0;
}

