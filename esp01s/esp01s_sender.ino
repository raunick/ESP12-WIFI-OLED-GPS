#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <espnow.h>

// ==========================================
// CONFIGURAÇÃO COMUM (WIFI & ESP-NOW)
// ==========================================
const char* ssid     = "RAUL";
const char* password = "24681357";

// Endereço de Broadcast para ESP-NOW (Envia para todos os ESPs próximos)
// Em produção, seria melhor usar o MAC específico do ESP12F
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Estrutura deve ser IDÊNTICA no Sender e Receiver
typedef struct struct_message {
  bool relayState;
} struct_message;

struct_message myData;
// ==========================================

ESP8266WebServer server(80);

// Configuração do Hardware ESP-01S + Relé v1.0
// Geralmente Relé no GPIO 0.
// IMPORTANTE: Alguns módulos relé ligam com LOW, outros com HIGH.
#define RELAY_PIN 0 
bool relayState = false; // Estado inicial

// Callback quando dados são enviados (feedback de sucesso/falha)
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Status de envio: ");
  if (sendStatus == 0){
    Serial.println("Sucesso");
  }
  else{
    Serial.println("Falha");
  }
}

void handleRoot() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:sans-serif;text-align:center;padding:50px;background:#222;color:white;}";
  html += ".btn{display:block;width:100%;padding:20px;margin:20px 0;font-size:2em;border:none;border-radius:10px;cursor:pointer;}";
  html += ".ON{background:#2ecc71;color:white;} .OFF{background:#e74c3c;color:white;}";
  html += "</style></head><body>";
  html += "<h1>ESP-01S Relay</h1>";
  html += "<p>ESP-NOW Sender</p>";
  
  // Botão único de Toggle
  html += "<a href='/toggle'><button class='btn " + String(relayState ? "ON" : "OFF") + "'>";
  html += relayState ? "LIGADO" : "DESLIGADO";
  html += "</button></a>";
  
  html += "<p><small>IP: " + WiFi.localIP().toString() + "</small></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleToggle() {
  relayState = !relayState;
  
  // Controle do Relé (Ajuste HIGH/LOW conforme seu módulo)
  // Módulos comuns: LOW = LIGADO, HIGH = DESLIGADO
  digitalWrite(RELAY_PIN, relayState ? LOW : HIGH); 
  
  // Enviar estado via ESP-NOW
  myData.relayState = relayState;
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
  Serial.println("Relé alterado para: " + String(relayState ? "ON" : "OFF"));
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Inicia Desligado (Assumindo Active LOW)

  Serial.println("\n\nCUIDADO: Use este código no módulo USB primeiro.");
  Serial.println("Ao colocar no módulo Relé, o GPIO 0 controla o relé.");

  // 1. Configurar WiFi em Station Mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");
  Serial.println("IP: " + WiFi.localIP().toString());

  // 2. Inicializar ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  // 3. Registrar Peer (Broacast ou MAC específico)
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  // 4. mDNS e Web Server
  if (MDNS.begin("esp01s")) {
    Serial.println("mDNS iniciado: http://esp01s.local");
  }

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update();
}
