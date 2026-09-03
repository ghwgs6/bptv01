#pragma once
#include "esphome.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

// Target Peers:
// Broadcast MAC Address: FF:FF:FF:FF:FF:FF
// Board 2 Receiver MAC Address: 8C:94:DF:6D:43:DC
// 7" ESP32-P4 Display MAC Address: 80:F1:B2:D3:63:6D
static uint8_t receiver_mac_broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t receiver_mac_board2[] = {0x8C, 0x94, 0xDF, 0x6D, 0x43, 0xDC};
static uint8_t receiver_mac_display[] = {0x80, 0xF1, 0xB2, 0xD3, 0x63, 0x6D};

class ESPNowSenderComponent : public esphome::Component {
 public:
  void setup() override {
    if (esp_now_init() != ESP_OK) {
      ESP_LOGE("esp_now", "Error initializing ESP-NOW");
      return;
    }

    // Add Broadcast Peer
    esp_now_peer_info_t peerBroadcast = {};
    memcpy(peerBroadcast.peer_addr, receiver_mac_broadcast, 6);
    peerBroadcast.channel = 0;  
    peerBroadcast.encrypt = false;
    esp_now_add_peer(&peerBroadcast);

    // Add Board 2 Peer
    esp_now_peer_info_t peerBoard2 = {};
    memcpy(peerBoard2.peer_addr, receiver_mac_board2, 6);
    peerBoard2.channel = 0;  
    peerBoard2.encrypt = false;
    esp_now_add_peer(&peerBoard2);

    // Add 7" Display Peer
    esp_now_peer_info_t peerDisplay = {};
    memcpy(peerDisplay.peer_addr, receiver_mac_display, 6);
    peerDisplay.channel = 0;  
    peerDisplay.encrypt = false;
    esp_now_add_peer(&peerDisplay);

    ESP_LOGI("esp_now", "ESP-NOW Sender Initialized! Broadcast & Target Peers registered.");
  }

  void send_state(const char* state_str) {
    // Broadcast & Unicast to peers
    esp_now_send(receiver_mac_broadcast, (uint8_t *) state_str, strlen(state_str));
    esp_now_send(receiver_mac_board2, (uint8_t *) state_str, strlen(state_str));
    esp_now_send(receiver_mac_display, (uint8_t *) state_str, strlen(state_str));
    ESP_LOGI("esp_now", "Sent ESP-NOW packet: %s", state_str);
  }
};

static ESPNowSenderComponent *global_esp_now_sender = nullptr;

inline void send_esp_now_packet(const char* msg) {
  if (global_esp_now_sender) {
    global_esp_now_sender->send_state(msg);
  }
}
