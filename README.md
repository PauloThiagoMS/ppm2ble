# PPM2BT 🎮

Este projeto transforma o sinal **PPM (Pulse Position Modulation)** de um rádio controle RC (via porta trainer ou receptor) em um **Gamepad Bluetooth HID** de alta performance. 

Utilizando o **ESP32**, você obtém um adaptador ultra compacto, ideal para ser embutido dentro do rádio ou montado em um dongle externo. É perfeito para simuladores de voo (FPV) como Liftoff, Velocidrone e jogos mobile.

---

## 🚀 Funcionalidades

* **📡 Conectividade Sem Fio:** Transforma rádios em controladores modernos Bluetooth.
* **📱 Compatibilidade Multiplataforma:** Reconhecido nativamente por **Windows 10/11**, **Android** e **iOS**.
* **⚡ Baixa Latência:** Taxa de atualização otimizada de **~100Hz (10ms)** para resposta imediata.
* **🔐 Segurança BLE:** Implementa *Bonding* e criptografia para garantir pareamento estável no Windows.
* **🎮 Gerenciamento de Canais:** Suporta 4 eixos analógicos e até 8 botões digitais.
* **🔋 Gerenciamento de Energia(Auto-Shutdown):** Se não detectar uma conexão Bluetooth ativa por mais de **60 segundos**, ele entrará em modo Deep Sleep(**5µA**)

---

---

## 🛠️ Requisitos de Hardware

* **Microcontrolador:** ESP32.
* **Entrada:** Cabo Trainer do seu Rádio ou receptor (Saída PPM).
* **Conexões:**
    * **VCC:** 3.3V ou 5V (O pino VCC do ESP32 aceita 5V).
    * **GND:** Comum.
    - **PPM Signal:** Conectado ao **GPIO 2** (Pino D2).

---

## 💻 Bibliotecas Necessárias

Instale as seguintes bibliotecas através do Gerenciador de Bibliotecas do Arduino IDE:

1.  **ESP32_ppm** (por fanfanlatulipe26) - Para decodificação do sinal PPM.
2.  **ESP32 BLE Arduino** (Nativa do core ESP32).

---

## ⚙️ Instalação e Uso

### 1. Circuito
Conecte o sinal PPM do rádio ao pino **GPIO 2** do ESP32. Certifique-se de compartilhar o terra (GND) entre o rádio e o ESP32.

### 2. Configuração
No código, você pode ativar o modo de depuração para conferir se os canais estão corretos:
```cpp
#define SERIAL_DEBUG // Descomente para ver os valores no Serial Monitor

```

### 3. Upload

No Arduino IDE, selecione a placa **ESP32 Dev Module** e faça o upload.

### 4. Pareamento

1. No Windows/Android, procure por **"PPM2BT-GAMEPAD"**.
2. Aceite a solicitação de pareamento e aguarde a instalação dos drivers HID.

---

## 🗺️ Mapeamento de Canais (Mode 2)

| Canal PPM | Função HID | Comportamento Comum |
| --- | --- | --- |
| **Canal 1** | Eixo Z | Throttle (Aceleração) |
| **Canal 2** | Eixo Rz | Yaw (Guinada) |
| **Canal 3** | Eixo Y | Pitch (Arfagem) |
| **Canal 4** | Eixo X | Roll (Rolagem) |
| **Canal 5** | Botão 1 | Chave de Armar |
| **Canal 6** | Botão 2 | Chave de Modo de Voo |
| **Canal 7** | Botão 3/4 | Chave de 3 posições (Mid/Hi) |
| **Canal 8** | Botão 5 | Chave Auxiliar |

---

## ⚠️ Solução de Problemas (Windows)

Se o dispositivo parear mas não responder no simulador:

1. Pressione `Win + R` e digite `joy.cpl` para testar os eixos nativamente.
2. Se houver erro de conexão, vá em **"Dispositivos e Impressoras"**, remova o **"PPM2BT-GAMEPAD"** e reinicie o Bluetooth do computador. O código utiliza `BLESecurity` para resolver falhas de autenticação.

---

## 💡 Recomendações Técnicas

* **Estabilidade:** Adicione um capacitor de **100µF a 470µF** entre os pinos VCC e GND para filtrar ruídos e picos de corrente do rádio Bluetooth.
* **Calibração:** Sempre calibre os sticks dentro do simulador ou através do comando `joy.cpl` no Windows para garantir a centralização perfeita dos eixos.
* **Segurança:** O uso do pino **VCC** em vez do pino 3.3V é altamente recomendado ao usar 4 pilhas, para que o regulador de tensão proteja o chip contra sobretensão.
* **Eficiência:** Remoção física (dessoldagem) dos LEDs integrados** na placa (especialmente o LED vermelho de Power).

---

## 📂 Galeria e Exemplos Visuais
Para auxiliar na montagem e configuração, você encontrará imagens detalhadas na pasta 👉 **[Assets](./assets)**, incluindo:

* **🖼️ Esquemas de Ligação:** Diagramas de fiação entre o Rádio Controle e o ESP32.
* **📍 Guia de Pinagem:** Identificação dos pinos de energia e sinal.
* **📸 Fotos da Montagem:** Exemplos de como o ESP32 foi acomodado dentro do rádio.

---

## 📄 Licença

Este projeto está sob a licença **MIT**. Sinta-se livre para clonar, modificar e distribuir.
