/** http */
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
int main() {
  int32_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
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
  if (listen(server_fd, 10) < 0) {
    std::cerr << "listen 失败" << std::endl;
    return -1;
  }
  std::cout << "HTTP 服务器启动，访问 http://localhost:8080 试试看..."
            << std::endl;
  while (true) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
      std::cerr << "accept 失败" << std::endl;
      continue;
    }
    char buffer[1024] = {0};
    recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    std::string body = "helloword";
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/plain; charset=utf-8\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: keep-alive\r\n";
    response << "\r\n";
    response << body;
    std::string resp_str = response.str();
    send(client_fd, resp_str.c_str(), resp_str.size(), 0);
    close(client_fd);
  }
  close(server_fd);
  return 0;
}