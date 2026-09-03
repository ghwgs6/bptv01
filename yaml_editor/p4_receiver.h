#pragma once
#include "esphome.h"
#include <lwip/sockets.h>
#include <fcntl.h>

class P4ReceiverComponent {
 public:
  bool di_states[18] = {false};
  bool di_bypassed[18] = {false};
  uint32_t last_packet_time{0};
  int sock_fd_{-1};
  bool comms_online{false};

  void handle_packet(const char* packet) {
    std::string msg(packet);
    last_packet_time = millis();
    comms_online = true;
    ESP_LOGI("p4_receiver", "Packet received: %s", packet);

    if (msg == "STATUS:OFFLINE") {
      comms_online = false;
      reset_states();
      return;
    }

    if (msg.rfind("DI_", 0) == 0) {
      int pin_num = std::atoi(msg.substr(3, 2).c_str());

      // Check for Bypass Commands
      if (msg.find(":BYPASS_ON") != std::string::npos) {
        if (pin_num >= 1 && pin_num <= 18) {
          di_bypassed[pin_num - 1] = true;
          ESP_LOGI("p4_receiver", "Zone DI %02d -> BYPASS ENABLED", pin_num);
        }
        return;
      }
      if (msg.find(":BYPASS_OFF") != std::string::npos) {
        if (pin_num >= 1 && pin_num <= 18) {
          di_bypassed[pin_num - 1] = false;
          ESP_LOGI("p4_receiver", "Zone DI %02d -> BYPASS DISABLED", pin_num);
        }
        return;
      }

      // Normal State Commands
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

  void reset_states() {
    for (int i = 0; i < 18; i++) {
      di_states[i] = false;
    }
    id(di_01_gw).publish_state(false);
    id(di_02_gw).publish_state(false);
    id(di_03_gw).publish_state(false);
    id(di_04_gw).publish_state(false);
    id(di_05_gw).publish_state(false);
    id(di_06_gw).publish_state(false);
    id(di_07_gw).publish_state(false);
    id(di_08_gw).publish_state(false);
    id(di_09_gw).publish_state(false);
    id(di_10_gw).publish_state(false);
    id(di_11_gw).publish_state(false);
    id(di_12_gw).publish_state(false);
    id(di_13_gw).publish_state(false);
    id(di_14_gw).publish_state(false);
    id(di_15_gw).publish_state(false);
    id(di_16_gw).publish_state(false);
    id(di_17_gw).publish_state(false);
    id(di_18_gw).publish_state(false);
  }

  void setup_socket() {
    if (sock_fd_ >= 0) {
      close(sock_fd_);
      sock_fd_ = -1;
    }

    int sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) return;

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

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    sock_fd_ = sock;
    ESP_LOGI("p4_receiver", "Fast non-blocking UDP socket listening on port 4444!");
  }

  void loop() {
    if (sock_fd_ < 0) {
      setup_socket();
    }

    // Watchdog check: 6 seconds without packet = comms lost
    if (comms_online && last_packet_time > 0 && (millis() - last_packet_time > 6000)) {
      comms_online = false;
      reset_states();
      ESP_LOGW("p4_receiver", "Comms Watchdog Timeout (>6s) - Comms Lost with Transmitter!");
    }

    if (sock_fd_ >= 0) {
      char buffer[256];
      struct sockaddr_in from_addr;
      socklen_t from_len = sizeof(from_addr);

      // Drain all pending datagrams
      while (true) {
        int len = recvfrom(sock_fd_, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&from_addr, &from_len);
        if (len > 0) {
          buffer[len] = '\0';
          handle_packet(buffer);
        } else {
          break;
        }
      }
    }
  }
};

static P4ReceiverComponent *global_p4_receiver = new P4ReceiverComponent();
