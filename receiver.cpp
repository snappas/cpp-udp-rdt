#include <iostream>
#include <cstring>
#include <sstream>
#include "RDT.h"


int main(int argc, char **argv) {

  if(argc != 5){
    std::cout << "usage: " << argv[0] << " dest_addr dest_port listen_addr listen_port" << std::endl;
    exit(1);
  }

  RDT rdt(100);
  rdt.listen(argv[3], std::atoi(argv[4]));
  while(true){
    std::string received = rdt.recv(argv[1], std::atoi(argv[2]));
    std::cout << "Received " << received.size() << " bytes: " << received << std::endl;
  }


  return 0;
}

