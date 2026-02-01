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
| **LED Status (Board)** | GPIO 4 | Saída | LED Azul (Active Low) |
| **OLED SDA** | GPIO 2 | I2C | Pino de dados do Display |
| **OLED SCL** | GPIO 14 | I2C | Pino de clock do Display |
| **LED Status (Node)** | GPIO 16 | Saída | LED Vermelho |
| **GPS RX**  | GPIO 12 | Serial Software | Leitura NMEA do Módulo GPS |
| **GPS TX**  | GPIO 13 | Serial Software | Envio de comandos p/ GPS |
| **Botão Flash** | GPIO 0 | Entrada | Usado para modo de gravação |
| **Botão Reset** | RST | Entrada | Reinicia o módulo |

## 🛠 Configuração de Software (v3.3 Cyber Edition)

- **Firmware Base**: Cyber-Master v3.3
- **mDNS**: `esp-12f.local`
- **Interface**: Dashboard Glassmorphism & OLED HUD Layout.

## 📝 Notas de Hardware

1.  **I2C do Display**: O display OLED utiliza o endereço I2C `0x3C`.
2.  **Lógica do LED**: O LED azul no pino GPIO 4 é **Active Low** (LOW=ON, HIGH=OFF).
3.  **GPS vs DHT11**: O pino GPIO 12 é agora dedicado ao **GPS RX**. O DHT11 original (mapeado para GPIO 12) foi desativado no firmware v3.3 para permitir o rastreamento via satélite.

---
*Documentação gerada com base nos arquivos esquemáticos fornecidos.*
