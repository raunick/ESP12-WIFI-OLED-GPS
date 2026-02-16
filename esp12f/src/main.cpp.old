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
bool remoteRelayState1 = false;
bool remoteRelayState2 = false;

// Flags for Loop Logic
int targetToSend = 0;        // 0 = none, 1 = Slave 1, 2 = Slave 2
bool oledNeedsUpdate = true; // Start true to draw initial screen

// Structs (Packed)
typedef struct __attribute__((packed)) struct_status {
  int senderID;
  bool relayState;
} struct_status;
struct_status incomingStatus;

typedef struct __attribute__((packed)) struct_cmd {
  int targetID; // 1 for Slave A, 2 for Slave B
  int command;
} struct_cmd;
struct_cmd myCommand;

void updateOLED() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // TOP INFO BAR
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("HTTP://ESP-12F.LOCAL");
  display.drawFastHLine(0, 13, 128, SSD1306_WHITE);

  // CENTER STATUS AREA
  // LED 1 (Node)
  display.setCursor(0, 18);
  display.print("L1:");
  if (stateNode)
    display.fillCircle(22, 20, 4, SSD1306_WHITE);
  else
    display.drawCircle(22, 20, 4, SSD1306_WHITE);

  // LED 2 (Board)
  display.setCursor(35, 18);
  display.print("L2:");
  if (stateBoard)
    display.fillCircle(57, 20, 4, SSD1306_WHITE);
  else
    display.drawCircle(57, 20, 4, SSD1306_WHITE);

  display.drawFastHLine(0, 27, 128, SSD1306_WHITE);

  // LED 3 (Remote Slaves)
  display.setCursor(70, 18);
  display.print("S1:");
  if (remoteRelayState1)
    display.fillCircle(90, 20, 3, SSD1306_WHITE);
  else
    display.drawCircle(90, 20, 3, SSD1306_WHITE);

  display.setCursor(100, 18);
  display.print("S2:");
  if (remoteRelayState2)
    display.fillCircle(120, 20, 3, SSD1306_WHITE);
  else
    display.drawCircle(120, 20, 3, SSD1306_WHITE);

  // BOTTOM DATA AREA
  if (gps.location.isValid()) {
    display.setCursor(0, 32);
    display.print("GPS SATS:");
    display.print(gps.satellites.value());
    display.setCursor(0, 44);
    display.print("LAT:");
    display.print(gps.location.lat(), 4);
    display.setCursor(0, 54);
    display.print("LON:");
    display.print(gps.location.lng(), 4);
  } else {
    display.setCursor(35, 38);
    display.print("WAITING GPS");
    static int anim = 0;
    anim = (anim + 1) % 4;
    for (int i = 0; i < anim; i++)
      display.print(".");

    display.setCursor(0, 54);
    display.print("R1:");
    display.print(remoteRelayState1 ? "ON" : "OFF");
    display.print(" R2:");
    display.print(remoteRelayState2 ? "ON" : "OFF");
  }

  display.display();
}

// Callback: Receive Status
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  if (len != sizeof(struct_status))
    return;
  memcpy(&incomingStatus, incomingData, sizeof(incomingStatus));

  if (incomingStatus.senderID == 1)
    remoteRelayState1 = incomingStatus.relayState;
  else if (incomingStatus.senderID == 2)
    remoteRelayState2 = incomingStatus.relayState;

  oledNeedsUpdate = true;
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='utf-8'><meta name='viewport' "
          "content='width=device-width,initial-scale=1'>";
  html += "<title>CYBER-CMD v3.3</title>";
  html += "<style>";
  html += ":root{--cyan:#00f3ff;--bg:#0a0a0b;--panel:rgba(255,255,255,0.05);--"
          "red:#ff0055;}";
  html += "body{background:var(--bg);color:#eee;font-family:'Segoe "
          "UI',Roboto,sans-serif;margin:0;padding:20px;display:flex;flex-"
          "direction:column;align-items:center;min-height:100vh;}";
  html += ".header{text-align:center;margin-bottom:30px;border-bottom:1px "
          "solid var(--cyan);padding-bottom:10px;width:100%;max-width:400px;}";
  html += ".header "
          "h1{margin:0;font-size:1.2rem;letter-spacing:4px;color:var(--cyan);"
          "text-transform:uppercase;}";
  html += ".card{background:var(--panel);backdrop-filter:blur(10px);border:1px "
          "solid "
          "rgba(255,255,255,0.1);border-radius:15px;padding:20px;width:100%;"
          "max-width:400px;margin-bottom:15px;box-sizing:border-box;}";
  html += ".status-box{display:flex;justify-content:space-between;align-items:"
          "center;margin-bottom:10px;}";
  html += ".btn{display:block;width:100%;padding:15px;margin:10px 0;border:1px "
          "solid "
          "var(--cyan);background:transparent;color:var(--cyan);font-weight:"
          "bold;text-transform:uppercase;cursor:pointer;border-radius:8px;"
          "transition:0.3s;}";
  html += ".btn:hover{background:var(--cyan);color:var(--bg);box-shadow:0 0 "
          "15px var(--cyan);}";
  html += ".btn.active{background:var(--cyan);color:var(--bg);}";
  html += ".btn.danger{border-color:var(--red);color:var(--red);}";
  html += ".btn.danger:hover{background:var(--red);color:#fff;box-shadow:0 0 "
          "15px var(--red);}";
  html +=
      ".gps-data{font-family:monospace;color:var(--cyan);font-size:0.9rem;}";
  html += "@keyframes pulse{0%{opacity:1;}50%{opacity:0.6;}100%{opacity:1;}}";
  html += ".live{animation:pulse 2s infinite;}";
  html += "</style></head><body>";

  html += "<div class='header'><h1>Cyber-Master</h1><small>" +
          WiFi.localIP().toString() + " | " + String(WiFi.RSSI()) +
          "dBm</small></div>";

  html += "<div class='card'>";
  html += "<h3>LOCAL CONTROLS</h3>";
  html += "<a href='/t16' style='text-decoration:none'><button class='btn " +
          String(stateNode ? "active" : "") + "'>NODE LED " +
          String(stateNode ? "[ON]" : "[OFF]") + "</button></a>";
  html += "<a href='/t4' style='text-decoration:none'><button class='btn " +
          String(stateBoard ? "active" : "") + "'>BOARD LED " +
          String(stateBoard ? "[ON]" : "[OFF]") + "</button></a>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>REMOTES (ESP-01S)</h3>";

  // Slave 1
  html += "<div class='status-box'><span>Slave 01: " +
          String(remoteRelayState1 ? "ON" : "OFF") + "</span></div>";
  html +=
      "<a href='/r1' style='text-decoration:none'><button class='btn danger'>" +
      String(remoteRelayState1 ? "OFF SLAVE 1" : "ON SLAVE 1") +
      "</button></a>";

  html += "<hr style='border:1px solid rgba(255,255,255,0.05); margin:15px 0'>";

  // Slave 2
  html += "<div class='status-box'><span>Slave 02: " +
          String(remoteRelayState2 ? "ON" : "OFF") + "</span></div>";
  html +=
      "<a href='/r2' style='text-decoration:none'><button class='btn danger'>" +
      String(remoteRelayState2 ? "OFF SLAVE 2" : "ON SLAVE 2") +
      "</button></a>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>GPS TELEMETRY</h3>";
  if (gps.location.isValid()) {
    html += "<div class='gps-data'>";
    html += "<p class='live'>🛰 SIGNAL LOCKED (" +
            String(gps.satellites.value()) + " SAT)</p>";
    html += "<p>LAT: " + String(gps.location.lat(), 6) + "</p>";
    html += "<p>LNG: " + String(gps.location.lng(), 6) + "</p>";
    html +=
        "<a href='https://maps.google.com/?q=" + String(gps.location.lat(), 6) +
        "," + String(gps.location.lng(), 6) +
        "' target='_blank' style='text-decoration:none'><button "
        "class='btn'>OPEN IN GOOGLE MAPS</button></a>";
    html += "</div>";
  } else {
    html += "<p class='live'>🛰 SEARCHING SATELLITES...</p>";
  }
  html += "</div>";

  html += "</body></html>";
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

  MDNS.begin("esp-12f");

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
  server.on("/r1", []() {
    targetToSend = 1;
    server.sendHeader("Location", "/");
    server.send(303);
  });
  server.on("/r2", []() {
    targetToSend = 2;
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
  if (targetToSend > 0) {
    myCommand.targetID = targetToSend;
    myCommand.command = 1;
    esp_now_send(broadcastAddress, (uint8_t *)&myCommand, sizeof(myCommand));
    targetToSend = 0; // Clear flag
  }

  // 2. Process OLED Update (Thread Safe)
  if (oledNeedsUpdate) {
    oledNeedsUpdate = false;
    updateOLED();
  }
}
