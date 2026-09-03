#pragma once
#include "esphome.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <string.h>

static int gateway_udp_socket = -1;

class ESPNowReceiverComponent : public esphome::Component {
 public:
  static void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
    char packet[256];
    int copy_len = len < 255 ? len : 255;
    memcpy(packet, incomingData, copy_len);
    packet[copy_len] = '\0';

    ESP_LOGI("esp_now", "Received ESP-NOW packet: %s", packet);

    // Forward UDP state packet to network (Port 4444)
    if (gateway_udp_socket < 0) {
      gateway_udp_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
      if (gateway_udp_socket >= 0) {
        int broadcastEnable = 1;
        setsockopt(gateway_udp_socket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
      }
    }

    if (gateway_udp_socket >= 0) {
      struct sockaddr_in dest_addr;
      memset(&dest_addr, 0, sizeof(dest_addr));
      dest_addr.sin_family = AF_INET;
      dest_addr.sin_port = htons(4444);

      // 1. Broadcast to entire subnet
      dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
      sendto(gateway_udp_socket, packet, strlen(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

      // 2. Direct unicast to standard display IP (192.168.3.30)
      dest_addr.sin_addr.s_addr = inet_addr("192.168.3.30");
      sendto(gateway_udp_socket, packet, strlen(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    }

    // Parse packet string e.g. "DI_01:ON" or "DI_01:OFF"
    std::string msg(packet);
    if (msg.rfind("DI_", 0) == 0) {
      int pin_num = std::atoi(msg.substr(3, 2).c_str());
      bool is_on = (msg.find(":ON") != std::string::npos);

      ESP_LOGI("esp_now", "Updating Receiver DI %02d -> State: %s", pin_num, is_on ? "ON" : "OFF");

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

  void setup() override {
    if (esp_now_init() != ESP_OK) {
      ESP_LOGE("esp_now", "Error initializing ESP-NOW");
      return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    ESP_LOGI("esp_now", "ESP-NOW Receiver Component Initialized!");
  }
};
