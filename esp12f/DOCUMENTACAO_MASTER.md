# 📡 ESP-12F: MASTER CONTROL UNIT (v3.3 - CYBER EDITION)

> **Módulo Central** responsável por gerenciar a rede ESP-NOW, exibir telemetria no OLED HUD e controlar múltiplos ESP-01S de forma independente.

---

## 📋 Especificações do Sistema

- **Função**: Master / Sender / Gateway / GPS Tracker
- **Firmware**: v3.3 (Cyber-Tech & Multi-Slave)
- **Protocolo**: ESP-NOW Targeted + WiFi (Web Server)
- **mDNS**: **[http://esp-12f.local](http://esp-12f.local)**

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

---

## 🎮 Funcionalidades (Web & Física)

### 1. Dashboard Web (Cyber-Tech UI)
Redesenhado com estética industrial moderna:
- **Glassmorphism**: Paineis com efeito de vidro e desfoque de fundo.
- **Controle Independente**: Botões separados para **Slave 01** e **Slave 02**.
- **Animações de Pulso**: Indicadores dinâmicos para sinal de GPS e LEDs ativos.

### 2. OLED HUD (Head-Up Display)
O layout foi otimizado em 3 zonas:
- **Zona 1 (Topo)**: URL de acesso (`esp-12f.local`) e barramento visual.
- **Zona 2 (Centro)**: Indicadores HUD (`●` Aceso, `○` Apagado) para **L1** (Node), **L2** (Board) e **S1/S2** (Slaves).
- **Zona 3 (Base)**: Dados dinâmicos de GPS (Lat/Lon e Satélites).

---

## 🔧 Protocolo Multi-Slave (v3.3)

O Master agora utiliza comandos direcionados para evitar que todas as placas acionem ao mesmo tempo:
- **Estrutura de Comando**: Inclui `targetID` (1 ou 2).
- **Estrutura de Status**: Slaves respondem com `senderID` para atualização individual no Dashboard.

---

## 🚀 Como Iniciar

1. Ligue o Master e aguarde o IP aparecer no Monitor Serial.
2. Acesse `esp-12f.local` pelo navegador.
3. Use os botões dedicados para controlar cada placa ESP-01S separadamente.
4. Monitore a telemetria GPS diretamente no visor OLED da placa.

