#include <ESP32_ppm.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>

#define PPM_PIN 2

ppmReader myPPM_RX;
int* ppmArray;

BLEHIDDevice *hid;
BLECharacteristic *inputGamepad;
BLEServer *server;
bool deviceConnected = false;

/* ================= HID Descriptor ================= */

const uint8_t hidReportDescriptor[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x05,        // Usage (Game Pad)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        // Report ID (1)

  // ===== Sticks Analógicos (4 eixos) =====
  0x09, 0x30,        // Usage (X)
  0x09, 0x31,        // Usage (Y)
  0x09, 0x33,        // Usage (Rx)
  0x09, 0x34,        // Usage (Ry)
  0x15, 0x81,        // Logical Minimum (-127)
  0x25, 0x7F,        // Logical Maximum (127)
  0x75, 0x08,        // Report Size (8 bits)
  0x95, 0x04,        // Report Count (4 axes)
  0x81, 0x02,        // Input (Data,Var,Abs)

  // ===== Botões (5) =====
  0x05, 0x09,        // Usage Page (Buttons)
  0x19, 0x01,        // Usage Minimum (Button 1)
  0x29, 0x05,        // Usage Maximum (Button 5)
  0x15, 0x00,        // Logical Minimum (0)
  0x25, 0x01,        // Logical Maximum (1)
  0x75, 0x01,        // Report Size (1 bit)
  0x95, 0x05,        // Report Count (5 buttons)
  0x81, 0x02,        // Input (Data,Var,Abs)

  // ===== Padding (completar 1 byte) =====
  0x75, 0x03,        // Report Size (3 bits)
  0x95, 0x01,        // Report Count (1)
  0x81, 0x03,        // Input (Const,Var,Abs)

  0xC0               // End Collection
};


struct GamepadReport {
  uint8_t reportId;

  int8_t lx;   // Stick esquerdo X  (Usage X)
  int8_t ly;   // Stick esquerdo Y  (Usage Y)

  int8_t rx;   // Stick direito X   (Usage Rx)
  int8_t ry;   // Stick direito Y   (Usage Ry)

  uint8_t buttons; // bits 0..4 usados
} __attribute__((packed));

/* ================= Callbacks ================= */

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("BLE conectado");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("BLE desconectado");
    BLEDevice::startAdvertising();
  }
};

/* ================= Helpers ================= */

// Converte 1000–2000us → -127 a +127
int8_t ppmToAxis(int value)
{
  if (value < 1000) value = 1000;
  if (value > 2000) value = 2000;

  return (int8_t) map(value, 1000, 2000, -127, 127);
}

// Canal → botão digital
bool ppmToButton(int value)
{
  return value > 1500;
}

bool ppmToButtonMid(int value)
{
  return (value > 1333 && value <= 1666);
}

bool ppmToButtonHi(int value)
{
  return value > 1666;
}

/* ================= Setup ================= */

void setup() {
  Serial.begin(115200);
  Serial.println("Start");

  /* ---- PPM ---- */

  myPPM_RX.RX_minimum_space = 3000;   // CRÍTICO para C3
  ppmArray = myPPM_RX.begin(PPM_PIN);

  if (ppmArray != nullptr) {
    myPPM_RX.start();
    Serial.println("PPM iniciado");
  } else {
    Serial.println("Erro PPM");
  }

  /* ---- BLE ---- */

  BLEDevice::init("PPM-BLE-Gamepad");

  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  hid = new BLEHIDDevice(server);

  hid->manufacturer()->setValue("ESP32");
  hid->pnp(0x02, 0xe502, 0xa111, 0x0110);
  hid->hidInfo(0x00, 0x01);

  hid->reportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));
  inputGamepad = hid->inputReport(1);

  hid->setBatteryLevel(100);
  hid->startServices();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();

  advertising->setAppearance(0x03C4);
  advertising->addServiceUUID(hid->hidService()->getUUID());

  advertising->setScanResponse(true);

  // ESSENCIAL para Windows
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);

  BLEDevice::startAdvertising();

  Serial.println("BLE pronto");
}

/* ================= Loop ================= */

void loop() {

  if (deviceConnected && ppmArray != nullptr) {

    GamepadReport report;
    report.reportId = 1;

    report.rx = ppmToAxis(ppmArray[1]);
    report.ry = ppmToAxis(ppmArray[2]);
    report.lx = ppmToAxis(ppmArray[4]);
    report.ly = ppmToAxis(ppmArray[3]);

    report.buttons = 0;

    if (ppmToButton(ppmArray[5]))     report.buttons |= (1 << 0);
    if (ppmToButton(ppmArray[6]))     report.buttons |= (1 << 1);
    if (ppmToButtonMid(ppmArray[7]))  report.buttons |= (1 << 2);
    if (ppmToButtonHi(ppmArray[7]))   report.buttons |= (1 << 3);
    if (ppmToButton(ppmArray[8]))     report.buttons |= (1 << 4);

    inputGamepad->setValue((uint8_t*)&report, sizeof(report));
    inputGamepad->notify();

    Serial.printf("LX=%d\tLY=%d\tRX=%d\tRY=%d\tBTN=0x%02X\n", report.lx, report.ly, report.rx, report.ry, report.buttons);

    delay(20); // ~50Hz
  }
}
