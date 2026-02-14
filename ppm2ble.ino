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
    0x05, 0x01,    // UsagePage(Generic Desktop[0x0001])
    0x09, 0x04,    // UsageId(Joystick[0x0004])
    0xA1, 0x01,    // Collection(Application)
    0x85, 0x01,    //     ReportId(1)
    0x09, 0x01,    //     UsageId(Pointer[0x0001])
    0xA1, 0x00,    //     Collection(Physical)
    0x09, 0x30,    //         UsageId(X[0x0030])
    0x09, 0x31,    //         UsageId(Y[0x0031])
    0x09, 0x32,    //         UsageId(Z[0x0032])
    0x09, 0x33,    //         UsageId(Rz[0x0033])
    0x15, 0x80,    //         LogicalMinimum(-128)
    0x25, 0x7F,    //         LogicalMaximum(127)
    0x95, 0x04,    //         ReportCount(4)
    0x75, 0x08,    //         ReportSize(8)
    0x81, 0x02,    //         Input(Data, Variable, Absolute, NoWrap, Linear, PreferredState, NoNullPosition, BitField)
    0xC0,          //     EndCollection()
    0x05, 0x09,    //     UsagePage(Button[0x0009])
    0x19, 0x01,    //     UsageIdMin(Button 1[0x0001])
    0x29, 0x05,    //     UsageIdMax(Button 5[0x0005])
    0x15, 0x00,    //     LogicalMinimum(0)
    0x25, 0x01,    //     LogicalMaximum(1)
    0x95, 0x05,    //     ReportCount(5)
    0x75, 0x01,    //     ReportSize(1)
    0x81, 0x02,    //     Input(Data, Variable, Absolute, NoWrap, Linear, PreferredState, NoNullPosition, BitField)
    0x95, 0x01,    //     ReportCount(1)
    0x75, 0x03,    //     ReportSize(3)
    0x81, 0x03,    //     Input(Constant, Variable, Absolute, NoWrap, Linear, PreferredState, NoNullPosition, BitField)
    0xC0,          // EndCollection()
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

    report.x = ppmToAxis(ppmArray[1], false);
    report.y = ppmToAxis(ppmArray[2], true);
    report.z = ppmToAxis(ppmArray[4], false);
    report.rz = ppmToAxis(ppmArray[3], false);

    report.buttons = 0;

    if (ppmToButton(ppmArray[5]))     report.buttons |= (1 << 0);
    if (ppmToButton(ppmArray[6]))     report.buttons |= (1 << 1);
    if (ppmToButtonMid(ppmArray[7]))  report.buttons |= (1 << 2);
    if (ppmToButtonHi(ppmArray[7]))   report.buttons |= (1 << 3);
    if (ppmToButton(ppmArray[8]))     report.buttons |= (1 << 4);

    inputGamepad->setValue((uint8_t*)&report, sizeof(report));
    inputGamepad->notify();

    Serial.printf("X=%d\tY=%d\tZ=%d\tRZ=%d\tBTN=0x%02X\n", report.x, report.y, report.z, report.rz, report.buttons);

    delay(20); // ~50Hz
  }
}
