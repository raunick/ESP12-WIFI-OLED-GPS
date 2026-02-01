/*
 * Blink simples para ESP-01S
 * 
 * O LED azul do ESP-01S geralmente está no GPIO 2 ou no GPIO 1 (TX).
 * Este código tenta usar o LED_BUILTIN padrão.
 */

void setup() {
  // Inicializa o pino do LED como saída
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Abre a serial para debug
  Serial.begin(115200);
  Serial.println("");
  Serial.println("ESP-01S Inicializado!");
}

void loop() {
  // Liga o LED (Nota: No ESP-01S o LED costuma ser LOW = LIGADO)
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("LED ligado");
  delay(1000);
  
  // Desliga o LED
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("LED desligado");
  delay(1000);
}
