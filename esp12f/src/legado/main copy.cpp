#include "Org_01.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <DHT.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <Wire.h>

ADC_MODE(ADC_VCC);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 2
#define OLED_SCL 14

#define GPS_RX 12
#define GPS_TX 13
#define DHTPIN 5
#define DHTTYPE DHT11

#define LED_BOARD 4 // NodeMCU board LED (GPIO4 = D2)

#define EEPROM_SIZE 96
#define SSID_ADDR 0
#define PASS_ADDR 32
#define SSID_MAX 32
#define PASS_MAX 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;
DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

// Face States
enum FaceState {
  FACE_NORMAL,
  FACE_HAPPY,
  FACE_SLEEPY,
  FACE_SURPRISED,
  FACE_LOOK
};
FaceState currentFace = FACE_NORMAL;
unsigned long faceStateStart = 0;

// UI Variables
String ui_time = "00:00:00";
String ui_date = "00/00/00";

String ui_temp = "--";
String ui_hum = "--";
String ui_lat = "-0.00";
String ui_lon = "-0.00";
bool hasGPSFix = false;
int batteryLevel = 3;
int gpsBars = 0;
float lastTemp = 0;
unsigned long lastTempChange = 0;
unsigned long noFixSince = 0;
int lookPhase = 0;

// WiFi & Icon State
bool wifiConnected = false;
bool apMode = false;
bool battBlinkState = true;
bool iconBlinkState = true;
unsigned long lastBattBlink = 0;
unsigned long lastIconBlink = 0;
bool showDate = false;
unsigned long lastDateToggle = 0;
bool ledState = false; // LED on/off

// EEPROM Helpers
String readEEPROM(int addr, int maxLen) {
  String val = "";
  for (int i = 0; i < maxLen; i++) {
    char c = EEPROM.read(addr + i);
    if (c == 0 || c == 255)
      break;
    val += c;
  }
  return val;
}

void writeEEPROM(int addr, String data, int maxLen) {
  for (int i = 0; i < maxLen; i++) {
    EEPROM.write(addr + i, i < (int)data.length() ? data[i] : 0);
  }
  EEPROM.commit();
}

// Captive Portal HTML
const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>
<style>body{font-family:sans-serif;background:#1a1a2e;color:#eee;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}
.card{background:#16213e;padding:30px;border-radius:12px;width:280px;box-shadow:0 8px 32px rgba(0,0,0,.5)}
h2{margin:0 0 20px;text-align:center;color:#0ff}
input{width:100%;padding:10px;margin:8px 0;border:1px solid #333;border-radius:6px;background:#0f3460;color:#eee;box-sizing:border-box}
button{width:100%;padding:12px;margin-top:12px;border:none;border-radius:6px;background:#e94560;color:#fff;font-size:16px;cursor:pointer}
button:hover{background:#ff6b6b}</style></head>
<body><div class='card'><h2>ESP12F WiFi</h2>
<form action='/save' method='POST'>
<input name='ssid' placeholder='SSID' required>
<input name='pass' type='password' placeholder='Senha' required>
<button type='submit'>Conectar</button></form></div></body></html>)rawliteral";

// WiFi Setup
void setupWiFi() {
  EEPROM.begin(EEPROM_SIZE);
  String ssid = readEEPROM(SSID_ADDR, SSID_MAX);
  String pass = readEEPROM(PASS_ADDR, PASS_MAX);

  if (ssid.length() > 0) {
    Serial.printf("WiFi: Connecting to '%s'...\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      apMode = false;
      Serial.printf("\nWiFi OK! IP: %s\n", WiFi.localIP().toString().c_str());

      // Start mDNS
      if (MDNS.begin("12f")) {
        Serial.println(F("mDNS: http://12f.local"));
      }

      // Dashboard
      server.on("/", []() {
        String html =
            F("<!DOCTYPE html><html><head><meta name='viewport' "
              "content='width=device-width,initial-scale=1'>"
              "<meta http-equiv='refresh' content='5'>"
              "<style>body{font-family:sans-serif;background:#1a1a2e;color:#"
              "eee;display:flex;justify-content:center;padding:20px;margin:0}"
              ".card{background:#16213e;padding:24px;border-radius:12px;width:"
              "300px;box-shadow:0 8px 32px rgba(0,0,0,.5)}"
              "h2{margin:0 0 16px;text-align:center;color:#0ff}"
              ".row{display:flex;justify-content:space-between;padding:8px "
              "0;border-bottom:1px solid #333}"
              ".label{color:#888}.val{color:#0ff;font-weight:bold}"
              "a.btn{display:block;text-align:center;padding:14px;margin-top:"
              "16px;border-radius:8px;text-decoration:none;font-size:16px;font-"
              "weight:bold}"
              ".on{background:#e94560;color:#fff}.off{background:#0f3460;color:"
              "#0ff}</style></head>"
              "<body><div class='card'><h2>ESP12F</h2>");
        html += "<div class='row'><span class='label'>Temp</span><span "
                "class='val'>" +
                ui_temp + "&deg;C</span></div>";
        html += "<div class='row'><span class='label'>Umid</span><span "
                "class='val'>" +
                ui_hum + "%</span></div>";
        html += "<div class='row'><span class='label'>Hora</span><span "
                "class='val'>" +
                ui_time + "</span></div>";
        html += "<div class='row'><span class='label'>Data</span><span "
                "class='val'>" +
                ui_date + "</span></div>";
        html += "<div class='row'><span class='label'>Lat</span><span "
                "class='val'>" +
                ui_lat + "</span></div>";
        html += "<div class='row'><span class='label'>Lon</span><span "
                "class='val'>" +
                ui_lon + "</span></div>";
        html += "<div class='row'><span class='label'>GPS</span><span "
                "class='val'>" +
                String(hasGPSFix ? "Fix" : "...") + "</span></div>";
        html += "<a class='btn " + String(ledState ? "on" : "off") +
                "' href='/led'>LED: " + String(ledState ? "ON" : "OFF") +
                "</a>";
        html += F("</div></body></html>");
        server.send(200, "text/html", html);
      });

      // LED toggle
      server.on("/led", []() {
        ledState = !ledState;
        digitalWrite(LED_BOARD, ledState ? LOW : HIGH);
        server.sendHeader("Location", "/");
        server.send(302);
      });

      server.begin();
      return;
    }
    Serial.println("\nWiFi: Connection failed.");
  }

  // Start AP mode
  apMode = true;
  wifiConnected = false;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP12F-Setup", "");
  Serial.printf("AP Mode: Connect to 'ESP12F-Setup' -> 192.168.4.1\n");

  server.on("/", []() { server.send_P(200, "text/html", PORTAL_HTML); });

  server.on("/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    writeEEPROM(SSID_ADDR, ssid, SSID_MAX);
    writeEEPROM(PASS_ADDR, pass, PASS_MAX);
    server.send(
        200, "text/html",
        "<h2 style='color:#0f0;text-align:center'>Salvo! Reiniciando...</h2>");
    delay(1500);
    ESP.restart();
  });

  server.on("/led", []() {
    ledState = !ledState;
    digitalWrite(LED_BOARD, ledState ? LOW : HIGH);
    server.send(200, "text/html",
                ledState
                    ? "<h2 style='color:#0f0;text-align:center'>LED ON</h2>"
                    : "<h2 style='color:#f00;text-align:center'>LED OFF</h2>");
  });

  server.begin();
}

// UI Bitmaps

static const unsigned char PROGMEM image_Pin_star_bits[] = {
    0x92, 0x54, 0x38, 0xfe, 0x38, 0x54, 0x92};
static const unsigned char PROGMEM image_weather_humidity_bits[] = {
    0x04, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x1e, 0x00, 0x1f,
    0x00, 0x3f, 0x80, 0x3f, 0x80, 0x7e, 0xc0, 0x7f, 0x40, 0xff, 0x60,
    0xff, 0xe0, 0x7f, 0xc0, 0x7f, 0xc0, 0x3f, 0x80, 0x0f, 0x00};
static const unsigned char PROGMEM image_weather_temperature_bits[] = {
    0x1c, 0x00, 0x22, 0x02, 0x2b, 0x05, 0x2a, 0x02, 0x2b, 0x38, 0x2a,
    0x60, 0x2b, 0x40, 0x2a, 0x40, 0x2a, 0x60, 0x49, 0x38, 0x9c, 0x80,
    0xae, 0x80, 0xbe, 0x80, 0x9c, 0x80, 0x41, 0x00, 0x3e, 0x00};
static const unsigned char PROGMEM image_passport_left_bits[] = {
    0x3c, 0x40, 0x98, 0xa4, 0xa4, 0x98, 0x80, 0x80, 0xa0, 0x90, 0x88, 0xa4,
    0x90, 0x88, 0xa4, 0x90, 0x88, 0xa4, 0x90, 0x88, 0xa4, 0x90, 0x88, 0xa4,
    0x90, 0x88, 0x84, 0x80, 0x40, 0x60, 0x70, 0x78, 0x7c, 0x5c, 0x4c, 0x4c,
    0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c};
static const unsigned char PROGMEM image_passport_left__copy__bits[] = {
    0xf0, 0x08, 0x64, 0x94, 0x94, 0x64, 0x04, 0x04, 0x14, 0x24, 0x44, 0x94,
    0x24, 0x44, 0x94, 0x24, 0x44, 0x94, 0x24, 0x44, 0x94, 0x24, 0x44, 0x94,
    0x24, 0x44, 0x84, 0x04, 0x08, 0x18, 0x38, 0x78, 0xf8, 0xe8, 0xc8, 0xc8,
    0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8};
static const unsigned char PROGMEM image_clock_alarm_bits[] = {
    0x79, 0x3c, 0xb3, 0x9a, 0xed, 0x6e, 0xd0, 0x16, 0xa0, 0x0a, 0x41,
    0x04, 0x41, 0x04, 0x81, 0x02, 0xc1, 0x06, 0x82, 0x02, 0x44, 0x04,
    0x48, 0x04, 0x20, 0x08, 0x10, 0x10, 0x2d, 0x68, 0x43, 0x84};
static const unsigned char PROGMEM image_calendar_bits[] = {
    0x09, 0x20, 0x76, 0xdc, 0xff, 0xfe, 0xff, 0xfe, 0x80, 0x02, 0x86,
    0xda, 0x86, 0xda, 0x80, 0x02, 0xb6, 0xda, 0xb6, 0xda, 0x80, 0x02,
    0xb6, 0xc2, 0xb6, 0xc2, 0x80, 0x02, 0x7f, 0xfc, 0x00, 0x00};
static const unsigned char PROGMEM image_earth_bits[] = {
    0x07, 0xc0, 0x1e, 0x70, 0x27, 0xf8, 0x61, 0xe4, 0x43, 0xe4, 0x87,
    0xca, 0x9f, 0xf6, 0xdf, 0x82, 0xdf, 0x82, 0xe3, 0xc2, 0x61, 0xf4,
    0x70, 0xf4, 0x31, 0xf8, 0x1b, 0xf0, 0x07, 0xc0, 0x00, 0x00};
static const unsigned char PROGMEM image_passport_left__copy___copy__bits[] = {
    0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c,
    0x5c, 0x7c, 0x78, 0x70, 0x60, 0x40, 0x80, 0x84, 0x88, 0x90, 0xa4, 0x88,
    0x90, 0xa4, 0x88, 0x90, 0xa4, 0x88, 0x90, 0xa4, 0x88, 0x90, 0xa4, 0x88,
    0x90, 0xa0, 0x80, 0x80, 0x98, 0xa4, 0xa4, 0x98, 0x40, 0x3c};
static const unsigned char PROGMEM image_weather_sun_bits[] = {
    0x01, 0x00, 0x21, 0x08, 0x10, 0x10, 0x03, 0x80, 0x8c, 0x62, 0x48,
    0x24, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x48, 0x24, 0x8c, 0x62,
    0x03, 0x80, 0x10, 0x10, 0x21, 0x08, 0x01, 0x00, 0x00, 0x00};
static const unsigned char PROGMEM image_weather_cloud_rain_bits[] = {
    0x00, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x08, 0x20, 0x00, 0x10, 0x10, 0x00,
    0x30, 0x08, 0x00, 0x40, 0x0e, 0x00, 0x80, 0x01, 0x00, 0x80, 0x00, 0x80,
    0x40, 0x00, 0x80, 0x3f, 0xff, 0x00, 0x01, 0x10, 0x00, 0x22, 0x22, 0x00,
    0x44, 0x84, 0x00, 0x91, 0x28, 0x00, 0x22, 0x40, 0x00, 0x00, 0x80, 0x00};
static const unsigned char PROGMEM image_weather_cloud_lightning_bolt_bits[] = {
    0x00, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x08, 0x20, 0x00, 0x10, 0x10, 0x00,
    0x30, 0x08, 0x00, 0x40, 0x0e, 0x00, 0x80, 0x81, 0x00, 0x81, 0x00, 0x80,
    0x43, 0x00, 0x80, 0x26, 0x3f, 0x00, 0x0f, 0x80, 0x00, 0x01, 0x80, 0x00,
    0x03, 0x00, 0x00, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
static const unsigned char PROGMEM image_satellite_dish_bits[] = {
    0x00, 0x00, 0x00, 0x78, 0x30, 0x04, 0x2c, 0x32, 0x63, 0x0a, 0xa8,
    0xea, 0x92, 0xa2, 0x90, 0xe0, 0x89, 0x10, 0x8a, 0x48, 0x44, 0x08,
    0x43, 0x24, 0x20, 0xc4, 0x30, 0x3c, 0x0c, 0x10, 0x03, 0xe0};
static const unsigned char PROGMEM image_weather_cloud_sunny_bits[] = {
    0x00, 0x20, 0x00, 0x02, 0x02, 0x00, 0x00, 0x70, 0x00, 0x01, 0x8c, 0x00,
    0x09, 0x04, 0x80, 0x02, 0x02, 0x00, 0x02, 0x02, 0x00, 0x07, 0x82, 0x00,
    0x08, 0x44, 0x80, 0x10, 0x2c, 0x00, 0x30, 0x30, 0x00, 0x60, 0x1e, 0x00,
    0x80, 0x03, 0x00, 0x80, 0x01, 0x00, 0x80, 0x01, 0x00, 0x7f, 0xfe, 0x00};
static const unsigned char PROGMEM image_weather_wind_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x03, 0x88, 0x04, 0x44, 0x04,
    0x44, 0x00, 0x44, 0x00, 0x88, 0xff, 0x32, 0x00, 0x00, 0xad, 0x82,
    0x00, 0x60, 0x00, 0x10, 0x00, 0x10, 0x01, 0x20, 0x00, 0xc0};
static const unsigned char PROGMEM image_Bluetooth_Idle_bits[] = {
    0x20, 0xb0, 0x68, 0x30, 0x30, 0x68, 0xb0, 0x20};
static const unsigned char PROGMEM image_BLE_beacon_bits[] = {
    0x44, 0x92, 0xaa, 0x92, 0x54, 0x10, 0x10, 0x7c};
static const unsigned char PROGMEM
    image_passport_left__copy___copy___copy__bits[] = {
        0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8,
        0xe8, 0xf8, 0x78, 0x38, 0x18, 0x08, 0x04, 0x84, 0x44, 0x24, 0x94, 0x44,
        0x24, 0x94, 0x44, 0x24, 0x94, 0x44, 0x24, 0x94, 0x44, 0x24, 0x94, 0x44,
        0x24, 0x14, 0x04, 0x04, 0x64, 0x94, 0x94, 0x64, 0x08, 0xf0};
static const unsigned char PROGMEM image_mouth_bits[] = {0x82, 0x82, 0x82, 0x44,
                                                         0x38};
static const unsigned char PROGMEM image_Battery_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f,
    0xf0, 0x10, 0x08, 0x32, 0xa8, 0x32, 0xa8, 0x10, 0x08, 0x0f, 0xf0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

bool flashState = false;
bool eyeState = true;
unsigned long lastBlink = 0;
unsigned long blinkInterval = 3000;

// Helper: Draw GPS signal bars at position
void drawGPSBars(int x, int y) {
  for (int i = 0; i < 4; i++) {
    int barH = 2 + (i * 2);
    int barY = y + (8 - barH);
    if (i < gpsBars) {
      display.fillRect(x + (i * 4), barY, 3, barH, 1);
    } else {
      display.drawRect(x + (i * 4), barY, 3, barH, 1);
    }
  }
}

// Helper: Draw animated face
void drawFace() {
  int eyeLX = 54, eyeRX = 76, eyeY = 6;

  // Adjust eye position for LOOK animation
  if (currentFace == FACE_LOOK) {
    int offset = 0;
    if (lookPhase == 1)
      offset = -3; // look left
    else if (lookPhase == 3)
      offset = 3; // look right
    eyeLX += offset;
    eyeRX += offset;
  }

  switch (currentFace) {
  case FACE_HAPPY:
    // ^  ^ eyes (arc shapes)
    display.drawLine(eyeLX - 3, eyeY + 1, eyeLX, eyeY - 2, 1);
    display.drawLine(eyeLX, eyeY - 2, eyeLX + 3, eyeY + 1, 1);
    display.drawLine(eyeRX - 3, eyeY + 1, eyeRX, eyeY - 2, 1);
    display.drawLine(eyeRX, eyeY - 2, eyeRX + 3, eyeY + 1, 1);
    // Big smile
    display.drawLine(60, 12, 63, 15, 1);
    display.drawLine(63, 15, 67, 15, 1);
    display.drawLine(67, 15, 70, 12, 1);
    break;

  case FACE_SLEEPY:
    // Half-closed eyes (lines lower)
    display.drawLine(eyeLX - 3, eyeY + 2, eyeLX + 3, eyeY + 2, 1);
    display.drawLine(eyeLX - 2, eyeY + 1, eyeLX + 2, eyeY + 1, 1);
    display.drawLine(eyeRX - 3, eyeY + 2, eyeRX + 3, eyeY + 2, 1);
    display.drawLine(eyeRX - 2, eyeY + 1, eyeRX + 2, eyeY + 1, 1);
    // Small flat mouth
    display.drawLine(62, 13, 68, 13, 1);
    // "z z z" bubble
    display.setFont(NULL);
    display.setTextSize(1);
    display.setCursor(80, 0);
    display.print("z");
    if (flashState) {
      display.setCursor(85, 0);
      display.print("z");
    }
    break;

  case FACE_SURPRISED:
    // Big round eyes
    display.drawCircle(eyeLX, eyeY, 4, 1);
    display.fillCircle(eyeLX, eyeY, 2, 1);
    display.drawCircle(eyeRX, eyeY, 4, 1);
    display.fillCircle(eyeRX, eyeY, 2, 1);
    // Small "O" mouth
    display.drawCircle(65, 13, 2, 1);
    break;

  case FACE_LOOK:
  case FACE_NORMAL:
  default:
    // Normal eyes with blink
    if (eyeState) {
      display.fillCircle(eyeLX, eyeY, 3, 1);
      display.fillCircle(eyeRX, eyeY, 3, 1);
    } else {
      display.drawLine(eyeLX - 3, eyeY, eyeLX + 3, eyeY, 1);
      display.drawLine(eyeRX - 3, eyeY, eyeRX + 3, eyeY, 1);
    }
    // Normal mouth
    display.drawBitmap(62, 10, image_mouth_bits, 7, 5, 1);
    break;
  }
}

void draw(void) {
  display.clearDisplay();

  // weather_humidity
  display.drawBitmap(9, 45, image_weather_humidity_bits, 11, 16, 1);

  // weather_temperature
  display.drawBitmap(9, 24, image_weather_temperature_bits, 16, 16, 1);

  // passport_borders
  display.drawBitmap(0, 18, image_passport_left_bits, 6, 46, 1);
  display.drawBitmap(122, 18, image_passport_left__copy__bits, 6, 46, 1);
  display.drawBitmap(0, -29, image_passport_left__copy___copy__bits, 6, 46, 1);
  display.drawBitmap(122, -29, image_passport_left__copy___copy___copy__bits, 6,
                     46, 1);

  // Separator line
  display.drawLine(5, 18, 122, 18, 1);

  // Clock / Calendar (alternates)
  if (showDate) {
    display.drawBitmap(65, 24, image_calendar_bits, 15, 16, 1);
  } else {
    display.drawBitmap(65, 24, image_clock_alarm_bits, 15, 16, 1);
  }

  if (hasGPSFix) {
    display.drawBitmap(65, 45, image_earth_bits, 15, 16, 1);
  } else {
    if (flashState) {
      display.drawBitmap(67, 44, image_satellite_dish_bits, 15, 16, 1);
    }
  }

  // Bluetooth Icon (blinks when WiFi not connected)
  // if (wifiConnected || iconBlinkState) {
  //  display.drawBitmap(115, 0, image_Bluetooth_Idle_bits, 5, 8, 1);
  //}
  // Pin_star (LED indicator - only visible when LED is on)
  if (ledState) {
    display.fillRect(104, 1, 7, 7, 1);
    display.drawBitmap(104, 1, image_Pin_star_bits, 7, 7, 0);
  }
  // BLE Beacon (blinks when WiFi not connected)
  if (wifiConnected || iconBlinkState) {
    display.drawBitmap(113, 0, image_BLE_beacon_bits, 7, 8, 1);
  }

  // Battery (blink speed depends on level)
  if (battBlinkState) {
    display.drawBitmap(106, 5, image_Battery_bits, 16, 16, 1);
  }

  // Dynamic Weather Icon
  int hum = ui_hum.toInt();
  if (hum > 70) {
    display.drawBitmap(8, 0, image_weather_cloud_rain_bits, 17, 16, 1);
  } else if (hum > 50) {
    display.drawBitmap(8, 0, image_weather_cloud_sunny_bits, 17, 16, 1);
  } else {
    display.drawBitmap(8, 0, image_weather_sun_bits, 15, 16, 1);
  }

  // Animated Face
  drawFace();

  // Text Values
  display.setFont(&Org_01);
  display.setTextColor(1);
  display.setTextSize(2);
  display.setTextWrap(false);

  // Humidity Value
  display.setCursor(27, 56);
  display.print(ui_hum + "%");

  // Temperature Value
  display.setCursor(27, 35);
  display.print(ui_temp);

  // Date / Time display (alternates every 3s)
  if (showDate) {
    // Date: day (big), month (small top), year (small bottom)
    display.setTextSize(2);
    display.setCursor(86, 35);
    display.print(ui_date.substring(0, 2));

    display.setTextSize(1);
    display.setCursor(109, 30);
    display.print(ui_date.substring(3, 5));

    display.setCursor(109, 37);
    display.print(ui_date.substring(6, 8));
  } else {
    // Time: hours (big), minutes (small top), seconds (small bottom)
    display.setTextSize(2);
    display.setCursor(86, 35);
    display.print(ui_time.substring(0, 2));

    display.setTextSize(1);
    display.setCursor(109, 30);
    display.print(ui_time.substring(3, 5));

    display.setCursor(109, 37);
    display.print(ui_time.substring(6, 8));
  }

  display.setTextSize(1);

  // Latitude
  display.setCursor(84, 49);
  display.print(ui_lat);

  // Longitude
  display.setCursor(84, 59);
  display.print(ui_lon);

  display.display();
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  Wire.begin(OLED_SDA, OLED_SCL);
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.clearDisplay();
  display.display();
  noFixSince = millis();

  setupWiFi();

  // Init LEDs
  pinMode(LED_BOARD, OUTPUT);
  digitalWrite(LED_BOARD, HIGH); // OFF (active LOW)

  Serial.println(F("System Initialized."));
}

unsigned long lastUpdate = 0;
unsigned long lastFlash = 0;
unsigned long lastLookStep = 0;
unsigned long lookTriggerTime = 0;
bool lookActive = false;

void loop() {
  // Handle web server (both AP and STA modes)
  server.handleClient();
  if (wifiConnected)
    MDNS.update();

  // GPS Processing
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  unsigned long now = millis();

  // Battery blink timing (full=no blink, medium=1s, low=250ms)
  unsigned long battInterval = 0;
  if (batteryLevel == 0)
    battInterval = 250; // fast blink: low
  else if (batteryLevel <= 2)
    battInterval = 1000; // slow blink: medium/charging
  // level 3 = full, no blink (always on)

  if (battInterval > 0 && now - lastBattBlink > battInterval) {
    lastBattBlink = now;
    battBlinkState = !battBlinkState;
  } else if (battInterval == 0) {
    battBlinkState = true; // always on when full
  }

  // WiFi icon blink (500ms when not connected)
  if (!wifiConnected && now - lastIconBlink > 500) {
    lastIconBlink = now;
    iconBlinkState = !iconBlinkState;
  } else if (wifiConnected) {
    iconBlinkState = true; // always on when connected
  }

  // Blinking Logic (face)
  if (currentFace == FACE_NORMAL || currentFace == FACE_LOOK) {
    if (now - lastBlink > (eyeState ? blinkInterval : 150)) {
      lastBlink = now;
      eyeState = !eyeState;
      if (eyeState)
        blinkInterval = random(2000, 6000);
      draw();
    }
  }

  // Look Around Animation Steps
  if (currentFace == FACE_LOOK && now - lastLookStep > 400) {
    lastLookStep = now;
    lookPhase++;
    if (lookPhase > 4) {
      lookPhase = 0;
      currentFace = FACE_NORMAL;
    }
    draw();
  }

  // Flashing logic (500ms - satellite dish, sleepy z's)
  if (now - lastFlash > 500) {
    lastFlash = now;
    flashState = !flashState;
    draw();
  }

  // Date/Time toggle (every 5s)
  if (now - lastDateToggle > 5000) {
    lastDateToggle = now;
    showDate = !showDate;
    draw();
  }

  // Value Updates (every 1 second)
  if (now - lastUpdate > 1000) {
    lastUpdate = now;

    // Check WiFi status
    if (!apMode) {
      wifiConnected = (WiFi.status() == WL_CONNECTED);
    }

    // Read Sensors
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h))
      ui_hum = String((int)h);
    if (!isnan(t)) {
      if (lastTemp != 0 && abs(t - lastTemp) > 3.0 &&
          (now - lastTempChange < 10000)) {
        currentFace = FACE_SURPRISED;
        faceStateStart = now;
      }
      lastTemp = t;
      lastTempChange = now;
      ui_temp = String((int)t);
    }

    // Battery level from VCC
    uint16_t vcc = ESP.getVcc();
    if (vcc > 3200)
      batteryLevel = 3;
    else if (vcc > 3000)
      batteryLevel = 2;
    else if (vcc > 2800)
      batteryLevel = 1;
    else
      batteryLevel = 0;

    // GPS Time (UTC-3 Brasília)
    if (gps.time.isValid() && gps.time.age() < 2000) {
      int hour = (gps.time.hour() - 3 + 24) % 24; // UTC-3
      char timeStr[12];
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", hour,
               gps.time.minute(), gps.time.second());
      ui_time = String(timeStr);
    } else {
      ui_time = "00:00:00";
    }

    // GPS Date
    if (gps.date.isValid() && gps.date.age() < 2000) {
      char dateStr[12];
      snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%02d", gps.date.day(),
               gps.date.month(), gps.date.year() % 100);
      ui_date = String(dateStr);
    } else {
      ui_date = "00/00/00";
    }

    // GPS Location
    bool hadFix = hasGPSFix;
    hasGPSFix = gps.location.isValid() && gps.location.age() < 5000;

    if (hasGPSFix) {
      ui_lat = String(gps.location.lat(), 2);
      ui_lon = String(gps.location.lng(), 2);
      if (!hadFix) {
        currentFace = FACE_HAPPY;
        faceStateStart = now;
      }
      noFixSince = now;
    } else {
      ui_lat = "-0.00";
      ui_lon = "-0.00";
      if (now - noFixSince > 30000 && currentFace == FACE_NORMAL) {
        currentFace = FACE_SLEEPY;
        faceStateStart = now;
      }
    }

    // GPS signal bars
    int sats = gps.satellites.value();
    if (sats >= 7)
      gpsBars = 4;
    else if (sats >= 5)
      gpsBars = 3;
    else if (sats >= 3)
      gpsBars = 2;
    else if (sats >= 1)
      gpsBars = 1;
    else
      gpsBars = 0;

    // Face state timeouts
    if (currentFace == FACE_HAPPY && now - faceStateStart > 5000)
      currentFace = FACE_NORMAL;
    if (currentFace == FACE_SURPRISED && now - faceStateStart > 3000)
      currentFace = FACE_NORMAL;
    if (currentFace == FACE_SLEEPY && hasGPSFix) {
      currentFace = FACE_HAPPY;
      faceStateStart = now;
    }

    // Random Look Around
    if (currentFace == FACE_NORMAL && !lookActive && random(0, 15) == 0) {
      currentFace = FACE_LOOK;
      lookPhase = 0;
      lastLookStep = now;
      lookActive = true;
    }
    if (currentFace != FACE_LOOK)
      lookActive = false;

    Serial.printf("Face:%d Bat:%d(%dmV) WiFi:%s Sat:%d T:%s\n", currentFace,
                  batteryLevel, vcc,
                  wifiConnected ? "OK" : (apMode ? "AP" : "X"), sats,
                  ui_time.c_str());

    draw();
  }
}
