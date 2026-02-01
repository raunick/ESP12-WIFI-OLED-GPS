# 📡 ESP-12F: MASTER CONTROL UNIT (v3.3 - GPS EDITION)

> **Módulo Central** responsável por gerenciar a rede ESP-NOW, exibir status no OLED e controlar o ESP-01S Remoto.

---

## 📋 Especificações do Sistema

- **Função**: Master / Sender / Gateway / GPS Tracker
- **Firmware**: v3.3 (Estável + GPS)
- **Protocolo**: ESP-NOW + WiFi (Web Server) + UART (GPS)
- **Display**: OLED 0.96" (128x64) I2C

## 📍 Pinout & Hardware

| Pino ESP | Função Hardware | Descrição |
| :--- | :--- | :--- |
| **GPIO 2** | **OLED SDA** | Linha de Dados I2C |
| **GPIO 14** | **OLED SCL** | Linha de Clock I2C |
| **GPIO 16** | **LED Local 1** | LED Node Vermelho (Controle Web) |
| **GPIO 4** | **LED Local 2** | LED Board Azul (Active Low) |
| **GPIO 12** | **GPS RX** | Recebe dados do GPS TX (SoftSerial) |
| **GPIO 13** | **GPS TX** | Envia dados para GPS RX (SoftSerial) |
| **USB** | **Serial** | Porta Serial 115200bps (Debug) |

> **Nota:** O OLED foi movido para GPIO 2/14 para liberar GPIO 4 para controle de LED, evitando conflitos de inicialização.

---

## 🎮 Funcionalidades (Web & Física)

O Master hospeda um Web Server acessível via mDNS: **[http://esp-master.local](http://esp-master.local)**

### 1. Dashboard Web (Retro UI)
- **Status de Rede**: Exibe SSID, IP e Sinal.
- **Botões Locais**: Controla LED 1 (GPIO16) e LED 2 (GPIO4).
- **Botão Remoto (Vermelho)**: *"REMOTE: ON/OFF"*
    - Envia comando via ESP-NOW para o ESP-01S.
    - Se o ESP-01S for desconectado, este status pode ficar desatualizado até o próximo "ping".

### 2. Display OLED
O display informa em tempo real:
- **Linha 1**: SSID da Rede + IP Final + RSSI.
- **Linha 2**: Status dos LEDs Locais (L1 e L2).
- **Linha 3**: **Status Remoto** (ESP-01S) ou **Coordenadas GPS** (Lat/Lon).

---

## 🔧 Notas Técnicas (Firmware)

### Correção de Crash (v3.1)
O envio ESP-NOW (`esp_now_send`) foi movido de dentro do callback HTTP para o `loop()` principal. Isso evita o reset aleatório (WDT Reset) ao clicar no botão remoto.

### Correção de OLED (v3.2)
A atualização da tela (`updateOLED`) é feita de forma assíncrona no `loop()`, e não mais dentro da interrupção de recebimento (`OnDataRecv`). Isso evita travamentos da tela quando o tráfego de rede é alto.

### Estrutura de Dados (Bidirecional)
O Master usa duas estruturas `packed` para comunicar com o Slave:
- `struct_cmd`: Envia comandos (Toggle).
- `struct_status`: Recebe o estado atual do Relé do Slave.

### Integração GPS (v3.3)
- Utiliza biblioteca **TinyGPS++** para parse NMEA.
- **SoftwareSerial** nos pinos 12 (RX) e 13 (TX) para não conflitar com a USB.
- Exibe Link para Google Maps no Dashboard se houver sinais de satélites válidos.

---

## 🚀 Como Iniciar

1. Ligue o Master na alimentação USB.
2. Aguarde o OLED iniciar (mostrando IP).
3. Conecte o Slave (ESP-01S).
4. Pressione o botão Vermelho no Web App para testar o pareamento.
