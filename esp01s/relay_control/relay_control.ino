#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

// ==========================================
// CONFIGURAÇÃO WIFI
// ==========================================
const char *ssid = "RAUL";
const char *password = "24681357";

// Porta diferente para evitar conflitos (8081)
ESP8266WebServer server(8081);

const int relayPin = 0; // GPIO 0 para o relé
bool relayStatus = false;

// Página Web principal com design moderno
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html +=
      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta charset='UTF-8'>";
  html += "<title>Controle de Relé ESP-01S</title>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, "
          "sans-serif; background-color: #1a1a2e; color: #ffffff; text-align: "
          "center; padding-top: 50px; }";
  html +=
      ".container { background-color: #16213e; display: inline-block; padding: "
      "40px; border-radius: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }";
  html += "h1 { color: #0f3460; margin-bottom: 30px; color: #e94560; }";
  html += ".status { font-size: 1.2em; margin-bottom: 20px; }";
  html += ".btn { display: inline-block; padding: 15px 30px; font-size: 1.1em; "
          "cursor: pointer; text-decoration: none; border-radius: 10px; "
          "transition: 0.3s; margin: 10px; border: none; font-weight: bold; }";
  html += ".btn-on { background-color: #4CAF50; color: white; }";
  html += ".btn-off { background-color: #f44336; color: white; }";
  html += ".btn:hover { transform: scale(1.05); opacity: 0.9; }";
  html += "</style></head><body>";

  html += "<div class='container'>";
  html += "<h1>ESP-01S Relay Control</h1>";
  html += "<div class='status'>Status do Relé: <b>" +
          String(relayStatus ? "LIGADO" : "DESLIGADO") + "</b></div>";
  html += "<a href='/on' class='btn btn-on'>LIGAR</a>";
  html += "<a href='/off' class='btn btn-off'>DESLIGAR</a>";
  html += "</div>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleOn() {
  relayStatus = true;
  digitalWrite(relayPin, LOW); // Nível lógico baixo ativa o relé
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleOff() {
  relayStatus = false;
  digitalWrite(relayPin, HIGH); // Nível lógico alto desativa o relé
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // Inicia desligado

  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("Porta: 8081");

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);

  server.begin();
  Serial.println("Servidor HTTP iniciado");

  // Configuração do mDNS (acesso via http://esp-rele.local:8081)
  if (MDNS.begin("esp-rele")) {
    Serial.println("mDNS iniciado: http://esp-rele.local:8081");
  }
}

void loop() {
  MDNS.update();
  server.handleClient();
}
