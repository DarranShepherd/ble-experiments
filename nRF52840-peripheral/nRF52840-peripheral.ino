#include <bluefruit.h>

const int LED = D6;

BLEService        service        = BLEService(UUID16_SVC_ENVIRONMENTAL_SENSING);
BLECharacteristic characteristic = BLECharacteristic(UUID16_CHR_VOLTAGE);

BLEDis bledis;                 // DIS (Device Information Service) helper class instance

uint8_t  bps = 0;              // Counter used to increment the characteristic value sent in each notification

void setup() {
  Serial.begin(115200);

  // Flash LED to show we're working and give time for serial monitor to connect
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(2500);
  digitalWrite(LED, LOW);

  Serial.println("nRF52840 Peripheral");

  // Bluetooth initialization
  initBt();
}

void loop() {

  if ( Bluefruit.connected() ) {
      Serial.println("Set LED high, notify BLE characteristic, set LED low");
      digitalWrite(LED, HIGH);

      // ADC read to get the voltage measurement
      uint8_t data[2] = { 0x0, bps++ };
      characteristic.notify(data, sizeof(data));

      digitalWrite(LED, LOW);
      Serial.println("Periodic event complete, sleeping...");
  } else {
    Serial.println("No central connected");
  }

  delay(10000);
}

void initBt()
{
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  Bluefruit.setName("BLE Peripheral");
  
  bledis.setManufacturer("DMS");
  bledis.setModel("Voltage Sensor");
  bledis.begin();

  setupSvc();

  startAdv();
}

void setupSvc(void)
{
  service.begin();

  characteristic.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  characteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  characteristic.setFixedLen(2);
  characteristic.begin();
  uint8_t data[2] = { 0x0, 0x0 };
  characteristic.write(data, 2);
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include HRM Service UUID
  Bluefruit.Advertising.addService(service);

  // Include Name
  Bluefruit.Advertising.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}