#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <espnow.h>

/*
 * =====================================================================
 * ESP-01S "SMART SLAVE" v3.2 (LED EFFECTS & MODERN UI)
 * =====================================================================
 * Features:
 * - Blink LED on connection.
 * - Keep LED ON for 3s after connection, then OFF.
 * - Modern Glassmorphism CSS for Dashboard.
 * - Hybrid ESP-NOW + Web Server.
 * =====================================================================
 */

const char *ssid = "RAUL";
const char *password = "24681357";

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#define RELAY_PIN 0
#define LED_STATUS_PIN 2 // Blue Onboard LED

ESP8266WebServer server(80);

// UNIQUE IDENTITY
#define SLAVE_ID 2 // <-- CHANGE TO 2 FOR THE SECOND DEVICE

// State
bool relayState = false;
String masterMacStr = "AGUARDANDO...";

// Structs (Packed)
typedef struct __attribute__((packed)) struct_sent {
  int senderID;
  bool relayState;
} struct_sent;
struct_sent myDataSent;

typedef struct __attribute__((packed)) struct_received {
  int targetID;
  int command;
} struct_received;
struct_received myDataReceived;

void applyRelayState() {
  // Lógica para placa Relé V5.0 (Active LOW)
  digitalWrite(RELAY_PIN, relayState ? LOW : HIGH);
  digitalWrite(LED_STATUS_PIN,
               relayState ? LOW : HIGH); // Sincroniza LED azul da placa
  Serial.print("Relay State: ");
  Serial.println(relayState ? "ON (LOW)" : "OFF (HIGH)");
}

void broadcastStatus() {
  myDataSent.senderID = SLAVE_ID;
  myDataSent.relayState = relayState;
  esp_now_send(broadcastAddress, (uint8_t *)&myDataSent, sizeof(myDataSent));
}

// Callback: Receive Command from Master
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  // Mensagem de Debug Serial
  Serial.print("Comando recebido do Master: ");

  if (len == sizeof(myDataReceived)) {
    memcpy(&myDataReceived, incomingData, sizeof(myDataReceived));

    // FILTER BY TARGET ID
    if (myDataReceived.targetID == SLAVE_ID) {
      Serial.print("CMD TARGETED TO ME (CMD=");
      Serial.print(myDataReceived.command);
      Serial.println(")");

      if (myDataReceived.command == 1) {
        relayState = !relayState;
        Serial.print("Novo estado do Relé: ");
        Serial.println(relayState ? "LIGADO" : "DESLIGADO");
      }
      applyRelayState();
      broadcastStatus();
    } else {
      Serial.print("IGNORED (TARGETED TO ID: ");
      Serial.print(myDataReceived.targetID);
      Serial.println(")");
    }
  } else {
    Serial.println("Tamanho de pacote INVÁLIDO");
  }

  // Captura o MAC do Master para exibir no site
  char macAddr[18];
  sprintf(macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);
  masterMacStr = String(macAddr);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='utf-8'><meta name='viewport' "
          "content='width=device-width,initial-scale=1'>";
  html += "<title>CYBER-SLAVE v3.3</title>";
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
  html += ".info-data{font-family:monospace;color:#888;font-size:0.8rem;line-"
          "height:1.5;}";
  html += "</style></head><body>";

  html += "<div class='header'><h1>Cyber-Slave #" + String(SLAVE_ID) +
          "</h1><small>" + WiFi.localIP().toString() + "</small></div>";

  html += "<div class='card'>";
  html += "<h3>RELAY CONTROL</h3>";
  String stateText = relayState ? "[ ACTIVE ]" : "[ INACTIVE ]";
  html +=
      "<p style='color:var(--cyan); font-weight:bold'>" + stateText + "</p>";
  String btnText = relayState ? "DEACTIVATE LOAD" : "ACTIVATE LOAD";
  html += "<a href='/toggle' style='text-decoration:none'><button class='btn " +
          String(relayState ? "danger" : "") + "'>" + btnText + "</button></a>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>NODE TELEMETRY</h3>";
  html += "<div class='info-data'>";
  html += "ID: SLAVE_" + String(SLAVE_ID) + "<br>";
  html += "IP: " + WiFi.localIP().toString() + "<br>";
  html += "MAC: " + WiFi.macAddress() + "<br>";
  html += "MASTER LINK: " + masterMacStr;
  html += "</div></div>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);

  // Inicia desligado
  relayState = false;
  applyRelayState();
  digitalWrite(LED_STATUS_PIN, HIGH); // Apagado (Active LOW)

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_STATUS_PIN, !digitalRead(LED_STATUS_PIN)); // Pisca rápido
    delay(100);
    Serial.print(".");
  }

  // LÓGICA PEDIDA: Conectado -> Fica ACESO por 3 segundos -> APAGA
  digitalWrite(LED_STATUS_PIN, LOW); // Liga (Blue LED)
  Serial.println("\nCONECTADO!");
  delay(3000);
  digitalWrite(LED_STATUS_PIN, HIGH); // Apaga

  if (esp_now_init() == 0) {
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(OnDataRecv);
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  }

  MDNS.begin("esp-01s-" + String(SLAVE_ID));
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
