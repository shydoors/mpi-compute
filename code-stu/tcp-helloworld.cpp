/** TCP/IP */
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
int main() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    std::cerr << "socket 创建失败" << std::endl;
    return -1;
  }
  int32_t opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);
  if (bind(server_fd, (sockaddr *)&address, sizeof(address)) < 0) {
    std::cerr << "bind 失败" << std::endl;
    return -1;
  }
  if (listen(server_fd, 3) < 0) {
    std::cerr << "listen 失败" << std::endl;
    return -1;
  }
  std::cout << "服务器正在监听 0.0.0.0:8080 ..." << std::endl;
  sockaddr_in client_addr{};
  socklen_t client_len = sizeof(client_addr);
  int client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_len);
  if (client_fd < 0) {
    std::cerr << "accept 失败" << std::endl;
    return -1;
  }
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
  std::cout << "客户端连接来自 " << client_ip << ":"
            << ntohs(client_addr.sin_port) << std::endl;
  const char *msg = "helloword";
  send(client_fd, msg, strlen(msg), 0);
  close(client_fd);
  close(server_fd);
  std::cout << "已发送 helloword，服务器关闭。" << std::endl;
  return 0;
}