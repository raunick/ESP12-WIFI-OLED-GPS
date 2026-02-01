#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <espnow.h>

/*
 * =====================================================================
 * ESP-01S "SMART SLAVE" v3.1 (DASHBOARD FIX)
 * =====================================================================
 * Features:
 * - Shows Master MAC + Local MAC
 * - Clear LED Status
 * - Packed Structs (safe)
 * =====================================================================
 */

const char *ssid = "RAUL";
const char *password = "24681357";

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#define RELAY_PIN 0
#define LED_BUILTIN_PIN 2

ESP8266WebServer server(80);

// State
bool relayState = false;
String masterMacStr = "WAITING...";
unsigned long lastMasterMsg = 0;

// Structs (Packed)
typedef struct __attribute__((packed)) struct_sent {
  bool relayState;
} struct_sent;
struct_sent myDataSent;

typedef struct __attribute__((packed)) struct_received {
  int command;
} struct_received;
struct_received myDataReceived;

void applyRelayState() {
  digitalWrite(RELAY_PIN, relayState ? LOW : HIGH);
  digitalWrite(LED_BUILTIN_PIN, relayState ? LOW : HIGH);
}

void broadcastStatus() {
  myDataSent.relayState = relayState;
  esp_now_send(broadcastAddress, (uint8_t *)&myDataSent, sizeof(myDataSent));
}

// Callback: Receive Command from Master
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  // 1. Capture Master MAC
  char macAddr[18];
  sprintf(macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);
  masterMacStr = String(macAddr);

  // 2. Process Command
  if (len == sizeof(myDataReceived)) {
    memcpy(&myDataReceived, incomingData, sizeof(myDataReceived));
    if (myDataReceived.command == 1) {
      relayState = !relayState;
    }
    applyRelayState();
    broadcastStatus();
    lastMasterMsg = millis();
  }
}

void handleRoot() {
  String html =
      "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' "
      "content='width=device-width,initial-scale=1'><title>ESP01S "
      "DASH</title><style>";
  html += "body{background-color:#000;color:#0f0;font-family:'Courier "
          "New',monospace;margin:0;padding:20px;text-align:center;}";
  html +=
      ".panel{border:2px solid #0f0;padding:20px;max-width:400px;margin:20px "
      "auto;box-shadow:0 0 15px #0f0 inset;}";
  html += ".btn{display:block;width:100%;background:#000;color:#0f0;border:1px "
          "solid #0f0;padding:20px;margin:20px "
          "0;font-size:18px;cursor:pointer;font-weight:bold;}";
  html += ".info{text-align:left;font-size:12px;border-bottom:1px dashed "
          "#333;padding:5px;}";
  html += ".active{background:#0f0;color:#000;}";
  html += "</style></head><body>";

  html += "<pre>ESP-01S INFORMER</pre>";

  html += "<div class='panel'>";
  html += "<h3>[ NETWORK INFO ]</h3>";
  html +=
      "<div class='info'>MY IP     : " + WiFi.localIP().toString() + "</div>";
  html += "<div class='info'>MY MAC    : " + WiFi.macAddress() + "</div>";
  html += "<div class='info'>MASTER MAC: " + masterMacStr + "</div>";

  html += "<br>";
  html +=
      "<a href='/toggle'><button class='btn " +
      String(relayState ? "active" : "") + "'> " +
      String(relayState ? "LED IS ON (CLICK OFF)" : "LED IS OFF (CLICK ON)") +
      "</button></a>";

  html += "<hr style='border:1px dashed #0f0'>";
  html += "STATUS: " + String(relayState ? "[ACTIVE]" : "[STANDBY]");
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_BUILTIN_PIN, OUTPUT);

  relayState = false;
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_BUILTIN_PIN, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN_PIN, !digitalRead(LED_BUILTIN_PIN));
    delay(250);
    Serial.print(".");
  }
  Serial.println("\nCONNECTED!");
  applyRelayState();

  if (esp_now_init() != 0)
    return;
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  MDNS.begin("esp-led");
  server.on("/", handleRoot);
  server.on("/toggle", []() {
    relayState = !relayState;
    applyRelayState();
    broadcastStatus();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update();
}
