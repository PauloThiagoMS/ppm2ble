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

const uint8_t hidReportDescriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)

    // --- 4 Eixos (X, Y, Z, Rz) -> 4 Bytes ---
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x80,        //   Logical Minimum (-128)
    0x25, 0x7F,        //   Logical Maximum (127)
    0x75, 0x08,        //   Report Size (8 bits)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)

    // --- 8 Botões -> 1 Byte ---
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (Button 1)
    0x29, 0x08,        //   Usage Maximum (Button 8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1 bit)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)

    0xC0               // End Collection
};


struct GamepadReport
{
    int8_t x;           // Usage X
    int8_t y;           // Usage Y
    int8_t z;           // Usage z
    int8_t rz;           // Usage Rz

    uint8_t buttons;    // 5 bits usados + 3 padding

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

// Converte 1000–2000us → -128 a 127
int8_t ppmToAxis(int value, bool invert)
{
  if (value < 1000) value = 1000;
  if (value > 2000) value = 2000;
  if (invert)
  {
    return (int8_t) map(value, 1000, 2000, 127, -128); 
  }
  else
  {
    return (int8_t) map(value, 1000, 2000, -128, 127);
  }
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

  BLEDevice::init("ESP32-Stick-V3");

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

    report.z = ppmToAxis(ppmArray[1], false);
    report.rz = ppmToAxis(ppmArray[2], true);
    report.x = ppmToAxis(ppmArray[4], false);
    report.y = ppmToAxis(ppmArray[3], true);

    uint8_t btnState = 0;
    if (ppmToButton(ppmArray[5]))    btnState |= (1 << 0); // Botão 1
    if (ppmToButton(ppmArray[6]))    btnState |= (1 << 1); // Botão 2
    if (ppmToButtonMid(ppmArray[7])) btnState |= (1 << 2); // Botão 3
    if (ppmToButtonHi(ppmArray[7]))  btnState |= (1 << 3); // Botão 4
    if (ppmToButton(ppmArray[8]))    btnState |= (1 << 4); // Botão 5
    
    report.buttons = btnState; // Os bits 5, 6 e 7 ficarão como 0 (soltos)

    inputGamepad->setValue((uint8_t*)&report, sizeof(report));
    inputGamepad->notify();

    Serial.printf("X=%d\tY=%d\tZ=%d\tRZ=%d\tBTN=0x%02X\n", report.x, report.y, report.z, report.rz, report.buttons);

    delay(15); // ~60Hz
  }
}
