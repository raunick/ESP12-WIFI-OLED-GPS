# Documentação: ESP8266 IoT Development Board

Esta placa é uma plataforma completa para desenvolvimento IoT, integrando conectividade WiFi, sensores de temperatura/umidade e display OLED.

## 📋 Especificações Principais

- **Microcontrolador**: ESP-12F (Baseado no ESP8266EX)
- **Conversor USB-Serial**: CH340C
- **Alimentação**: 5V via Micro USB (Regulador 3.3V AMS1117 integrado)
- **Display**: OLED 0.96 polegadas (Chip SSD1306)
- **Sensor**: DHT11 (Temperatura e Umidade)
- **Interface**: Micro-USB

## 📍 Pinagem e Periféricos (Baseado no Esquema)

| Componente | Pino ESP (GPIO) | Função | Observação |
| :--- | :--- | :--- | :--- |
| **LED Status** | GPIO4 | Saída | LED D1 (Verde/Azul) |
| **OLED SDA** | GPIO2 | I2C | Pino de dados do Display |
| **OLED SCL** | GPIO14 | I2C | Pino de clock do Display |
| **DHT11** | GPIO12 | Entrada | Sensor de Temp/Umidade |
| **Botão Flash** | GPIO0 | Entrada | Usado para modo de gravação / Input |
| **Botão Reset** | RST | Entrada | Reinicia o módulo |
| **UART TX** | TXD0 (GPIO1) | Serial | Transmissão de dados (Debug) |
| **UART RX** | RXD0 (GPIO3) | Serial | Recepção de dados (Debug) |
| **GPS RX**  | GPIO12 | Serial Software | Leitura NMEA do Módulo GPS |
| **GPS TX**  | GPIO13 | Serial Software | Envio de comandos p/ GPS |

## 🛠 Configuração PlatformIO (`platformio.ini`)

Para usar esta placa no PlatformIO, utilize a seguinte base:

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
monitor_speed = 115200
lib_deps = 
	adafruit/Adafruit SSD1306 @ ^2.5.7
	adafruit/Adafruit GFX Library @ ^1.11.5
	adafruit/DHT sensor library @ ^1.4.4
	mikalhart/TinyGPSPlus @ ^1.0.3
```

## 📝 Notas de Hardware

1.  **I2C do Display**: O display OLED utiliza o endereço I2C `0x3C`.
2.  **Lógica do LED**: O LED de status no pino GPIO4 é **Active Low** (LOW=ON, HIGH=OFF).
3.  **DHT11 vs GPS**: O pino GPIO12, originalmente mapeado para DHT11, agora é utilizado para **GPS RX**. Se usar GPS, remova o DHT11 ou remapeie.

---
*Documentação gerada com base nos arquivos esquemáticos fornecidos.*
