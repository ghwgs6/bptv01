#pragma once
#include "esphome.h"
#include <lwip/sockets.h>
#include <fcntl.h>

class UDPReceiverComponent {
 public:
  bool di_states[18] = {false};
  uint32_t last_packet_time{0};
  int sock_fd_{-1};

  void setup_socket() {
    if (sock_fd_ >= 0) {
      close(sock_fd_);
      sock_fd_ = -1;
    }

    int sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
      return;
    }

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(4444);

    if (bind(sock, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
      close(sock);
      return;
    }
    
    // Set non-blocking socket
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    sock_fd_ = sock;
    ESP_LOGI("udp_receiver", "UDP Receiver bound on Port 4444!");
  }

  void loop() {
    if (sock_fd_ < 0) {
      static uint32_t last_retry = 0;
      if (millis() - last_retry > 1000) {
        last_retry = millis();
        setup_socket();
      }
      return;
    }

    char buffer[128];
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    int n = recvfrom(sock_fd_, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliaddr, &len);
    if (n > 0) {
      buffer[n] = '\0';
      std::string msg(buffer);
      last_packet_time = millis();
      ESP_LOGI("udp_receiver", "Received UDP packet: %s", buffer);

      if (msg.rfind("DI_", 0) == 0) {
        int pin_num = std::atoi(msg.substr(3, 2).c_str());
        bool is_on = (msg.find(":ON") != std::string::npos);

        if (pin_num >= 1 && pin_num <= 18) {
          di_states[pin_num - 1] = is_on;
        }

        if (pin_num == 1) id(di_01_gw).publish_state(is_on);
        else if (pin_num == 2) id(di_02_gw).publish_state(is_on);
        else if (pin_num == 3) id(di_03_gw).publish_state(is_on);
        else if (pin_num == 4) id(di_04_gw).publish_state(is_on);
        else if (pin_num == 5) id(di_05_gw).publish_state(is_on);
        else if (pin_num == 6) id(di_06_gw).publish_state(is_on);
        else if (pin_num == 7) id(di_07_gw).publish_state(is_on);
        else if (pin_num == 8) id(di_08_gw).publish_state(is_on);
        else if (pin_num == 9) id(di_09_gw).publish_state(is_on);
        else if (pin_num == 10) id(di_10_gw).publish_state(is_on);
        else if (pin_num == 11) id(di_11_gw).publish_state(is_on);
        else if (pin_num == 12) id(di_12_gw).publish_state(is_on);
        else if (pin_num == 13) id(di_13_gw).publish_state(is_on);
        else if (pin_num == 14) id(di_14_gw).publish_state(is_on);
        else if (pin_num == 15) id(di_15_gw).publish_state(is_on);
        else if (pin_num == 16) id(di_16_gw).publish_state(is_on);
        else if (pin_num == 17) id(di_17_gw).publish_state(is_on);
        else if (pin_num == 18) id(di_18_gw).publish_state(is_on);
      }
    }
  }
};

static UDPReceiverComponent *global_udp_receiver = new UDPReceiverComponent();
