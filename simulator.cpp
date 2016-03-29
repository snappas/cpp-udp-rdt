#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <sstream>
#include <random>


void simulate(char *buffer, int socket, sockaddr_in addr, double loss_percent, double corrupt_percent);
char *corrupt(char *buffer);
int main(int argc, char **argv) {

  if(argc != 5){
    std::cout << "usage: " << argv[0] << "send_dest_port recv_listen_port recv_dest_port send_listen_port" << std::endl;
    exit(1);
  }

  int send_from_port = std::atoi(argv[1]);
  int recv_to_port = std::atoi(argv[2]);
  int recv_from_port = std::atoi(argv[3]);
  int send_to_port = std::atoi(argv[4]);

  int send_from_socket = socket(AF_INET,SOCK_DGRAM | SOCK_NONBLOCK, 0);
  int recv_from_socket = socket(AF_INET,SOCK_DGRAM | SOCK_NONBLOCK, 0);


  int out_socket = socket(AF_INET,SOCK_DGRAM , 0);

  double loss_pcnt = 0.25;
  double corrupt_pcnt = 0.25;

  sockaddr_in recv_from_addr;
  std::memset(&recv_from_addr, 0, sizeof(recv_from_addr));
  recv_from_addr.sin_port = htons(recv_from_port);

  sockaddr_in send_from_addr;
  std::memset(&send_from_addr, 0, sizeof(send_from_addr));
  send_from_addr.sin_port = htons(send_from_port);

  sockaddr_in recv_to_addr;
  std::memset(&recv_to_addr, 0, sizeof(recv_to_addr));
  inet_aton("127.0.0.1", &recv_to_addr.sin_addr);
  recv_to_addr.sin_port = htons(recv_to_port);

  sockaddr_in send_to_addr;
  std::memset(&send_to_addr, 0, sizeof(send_to_addr));
  inet_aton("127.0.0.1", &send_to_addr.sin_addr);
  send_to_addr.sin_port = htons(send_to_port);

  bind(recv_from_socket, (struct sockaddr *)&recv_from_addr, sizeof(recv_from_addr));
  bind(send_from_socket, (struct sockaddr *)&send_from_addr, sizeof(send_from_addr));



  while(true){
    socklen_t sock_len;
    char buffer[4096];
    ssize_t val;
    val = recvfrom(recv_from_socket, &buffer, 4096, 0, (struct sockaddr *)&recv_from_addr, &sock_len);
    if(val != -1){
      simulate(buffer, out_socket, send_to_addr, loss_pcnt, corrupt_pcnt);
    }
    val = recvfrom(send_from_socket, &buffer, 4096, 0, (struct sockaddr *)&send_from_addr, &sock_len);
    if(val != -1){
      simulate(buffer, out_socket, recv_to_addr, loss_pcnt, corrupt_pcnt);
    }

    std::memset(buffer, 0, 4096);
  }
  return 0;
}
void simulate(char *buffer, int socket, sockaddr_in addr, double loss_percent, double corrupt_percent) {
  std::random_device generator;
  std::uniform_real_distribution<double> dist(0, 1);
  double random = dist(generator);
  if(random <= loss_percent && loss_percent != 0.0){
    std::cout << "Dropped" << std::endl;
    return;
  }
  if(random <= loss_percent + corrupt_percent && corrupt_percent != 0.0){
    buffer = corrupt(buffer);
    std::cout << "Corrupted: " << buffer << std::endl;
  }else{
    std::cout << "Forwarded" << std::endl;
  }
  sendto(socket, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, sizeof(addr));

}
char *corrupt(char *buffer) {
  int len = (int) (strlen(buffer) - 1);
  std::random_device generator;
  std::uniform_int_distribution<int> i(0,len);
  std::uniform_int_distribution<int> c(0,95);
  int index = i(generator);
  char character = (char) c(generator);
  if (len > 0) {
    buffer[index] = character;
  }

  return buffer;
}

