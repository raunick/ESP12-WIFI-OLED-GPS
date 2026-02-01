#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <espnow.h>

/*
 * =====================================================================
 * ESP-12F MASTER CONTROLLER v3.2 (ASYNC OLED)
 * =====================================================================
 * FIXES:
 * - v3.1: Moved ESP-NOW Send to Loop (Anti-Crash)
 * - v3.2: Moved OLED Update to Loop (Anti-Freeze/Corruption)
 * =====================================================================
 */

const char *ssid = "RAUL";
const char *password = "24681357";

// Hardware PINOUT
#define OLED_SDA 2
#define OLED_SCL 14
#define LED_NODE_PIN 16
#define LED_BOARD_PIN 4
#define GPS_RX_PIN 12 // Connect to GPS TX
#define GPS_TX_PIN 13 // Connect to GPS RX
#define GPS_BAUD 9600
#define SCREEN_ADDRESS 0x3C

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

Adafruit_SSD1306 display(128, 64, &Wire, -1);
ESP8266WebServer server(80);
TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

// State
bool stateNode = false;
bool stateBoard = false;
bool remoteRelayState = false;

// Flags for Loop Logic
bool shouldSendRemote = false;
bool oledNeedsUpdate = true; // Start true to draw initial screen

// Structs (Packed)
typedef struct __attribute__((packed)) struct_status {
  bool relayState;
} struct_status;
struct_status incomingStatus;

typedef struct __attribute__((packed)) struct_cmd {
  int command;
} struct_cmd;
struct_cmd myCommand;

void updateOLED() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(WiFi.SSID());
  display.print(" ");
  display.print(WiFi.localIP().toString());
  display.print(" ");
  display.print(WiFi.RSSI());

  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  // Status Grid
  display.setCursor(0, 16);
  display.print("IO16:");
  display.print(stateNode ? "ON" : "OFF");
  display.setCursor(60, 16);
  display.print("IO04:");
  display.print(stateBoard ? "ON" : "OFF");

  display.drawLine(0, 28, 128, 28, SSD1306_WHITE);

  // Remote Section
  display.setCursor(0, 32);
  display.setTextSize(1);
  display.print("ESP01S:");
  display.setCursor(0, 50);

  if (remoteRelayState) {
    display.print(" [ON] ");
  } else {
    display.print(" [OFF] ");
  }

  // GPS Section
  display.setCursor(64, 32);
  display.print("GPS:");
  if (gps.location.isValid()) {
    display.setCursor(64, 42);
    display.print(gps.location.lat(), 2);
    display.setCursor(64, 52);
    display.print(gps.location.lng(), 2);
  } else {
    display.setCursor(64, 42);
    display.print("NO FIX");
  }

  display.display();
}

// Callback: Receive Status (Runs in ISR context!)
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  if (len != sizeof(struct_status))
    return;
  memcpy(&incomingStatus, incomingData, sizeof(incomingStatus));

  remoteRelayState = incomingStatus.relayState;
  oledNeedsUpdate = true; // Schedule update for main loop
}

void handleRoot() {
  String html =
      "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' "
      "content='width=device-width,initial-scale=1'><title>MASTER "
      "CONTROL</title><style>";
  html += "body{background-color:#000;color:#0f0;font-family:'Courier "
          "New',monospace;margin:0;padding:20px;text-align:center;}";
  html += ".panel{border:2px solid "
          "#0f0;padding:20px;max-width:400px;margin:20px auto;}";
  html +=
      ".btn{display:block;width:100%;color:#0f0;background:#000;border:1px "
      "solid #0f0;padding:15px;margin:10px 0;font-size:16px;cursor:pointer;}";
  html += ".btn:hover{background:#0f0;color:#000;}";
  html += ".active{background:#0f0;color:#000;}";
  html += ".danger{border-color:#f00;color:#f00;}";
  html += "</style></head><body>";

  html += "<pre>MASTER 12F COMMANDER</pre>";

  html += "<div class='panel'>";
  html += "<p>> LOCAL CONTROLS</p>";
  html += "<a href='/t16'><button class='btn " +
          String(stateNode ? "active" : "") +
          "'>[GPIO 16] NODE LED</button></a>";
  html += "<a href='/t4'><button class='btn " +
          String(stateBoard ? "active" : "") +
          "'>[GPIO 04] BOARD LED</button></a>";

  html += "<hr style='border:1px dashed #0f0'>";

  html += "<p>> REMOTE LINK (ESP-01S)</p>";
  String remoteText = remoteRelayState ? "REMOTE IS: ON (CLICK OFF)"
                                       : "REMOTE IS: OFF (CLICK ON)";

  html += "<a href='/remoteToggle'><button class='btn danger'>" + remoteText +
          "</button></a>";

  html += "<hr style='border:1px dashed #0f0'>";
  html += "<p>> GPS STATUS</p>";
  if (gps.location.isValid()) {
    html += "<p>LAT: " + String(gps.location.lat(), 6) + "</p>";
    html += "<p>LNG: " + String(gps.location.lng(), 6) + "</p>";
    html += "<p>SATS: " + String(gps.satellites.value()) + "</p>";
    html +=
        "<a href='https://maps.google.com/?q=" + String(gps.location.lat(), 6) +
        "," + String(gps.location.lng(), 6) +
        "' target='_blank'><button class='btn'>VIEW ON MAP</button></a>";
  } else {
    html += "<p>SEARCHING FOR SATELLITES...</p>";
  }

  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD);

  pinMode(LED_NODE_PIN, OUTPUT);
  pinMode(LED_BOARD_PIN, OUTPUT);
  digitalWrite(LED_NODE_PIN, LOW);
  digitalWrite(LED_BOARD_PIN, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    display.clearDisplay();
    display.display();
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(200);

  MDNS.begin("esp-master");

  if (esp_now_init() != 0)
    return;
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  server.on("/", handleRoot);
  server.on("/t16", []() {
    stateNode = !stateNode;
    digitalWrite(LED_NODE_PIN, stateNode ? HIGH : LOW);
    server.sendHeader("Location", "/");
    server.send(303);
    oledNeedsUpdate = true;
  });
  server.on("/t4", []() {
    stateBoard = !stateBoard;
    digitalWrite(LED_BOARD_PIN, stateBoard ? LOW : HIGH);
    server.sendHeader("Location", "/");
    server.send(303);
    oledNeedsUpdate = true;
  });
  server.on("/remoteToggle", []() {
    shouldSendRemote = true; // Schedule send
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();
  oledNeedsUpdate = true;
}

void loop() {
  server.handleClient();
  MDNS.update();

  // 0. Feed GPS
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      oledNeedsUpdate = true;
    }
  }

  // 1. Process Remote Command
  if (shouldSendRemote) {
    shouldSendRemote = false;
    myCommand.command = 1;
    esp_now_send(broadcastAddress, (uint8_t *)&myCommand, sizeof(myCommand));
  }

  // 2. Process OLED Update (Thread Safe)
  if (oledNeedsUpdate) {
    oledNeedsUpdate = false;
    updateOLED();
  }
}
