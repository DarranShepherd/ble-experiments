#include <bluefruit.h>

#define SLEEP_TIME  10         // Sleep time [sec]

const int LED = D6;

BLEService        service        = BLEService(UUID16_SVC_ENVIRONMENTAL_SENSING);
BLECharacteristic characteristic = BLECharacteristic(UUID16_CHR_VOLTAGE);

BLEDis bledis;                 // DIS (Device Information Service) helper class instance

bool interruptFlag = true;     // Interrupt flag
uint8_t  bps = 0;              // Counter used to increment the characteristic value sent in each notification

void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(2500);
  digitalWrite(LED, LOW);

  Serial.println("nRF52840 Peripheral");

  // RTC initialization  
  initRTC(32768 * SLEEP_TIME); // SLEEP_MTIME [sec]  

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
  
      delay(100);

      digitalWrite(LED, LOW);
      Serial.println("Periodic event complete, sleeping...");
  } else {
    Serial.println("No central connected");
  }

  delay(10000);

  // if (interruptFlag == true)     // If wake up interrupt
  // {       
  //   Serial.println("Connecting to Central ....");
  //   while (!Bluefruit.connected()) { delay(10); }
  //   Serial.println("Connected to Central");  
    
  //   if (Bluefruit.connected())      // If connected to the central
  //   {
  //     Serial.println("Set LED high, notify BLE characteristic, set LED low");
  //     digitalWrite(LED, HIGH);

  //     // ADC read to get the voltage measurement
  //     uint8_t data[2] = { 0x0, bps++ };
  //     characteristic.notify(data, sizeof(data));
  
  //     digitalWrite(LED, LOW);
  //     Serial.println("Periodic event complete, sleeping...");
  //   }

  //   interruptFlag = false;
  //   // connectedFlag = false;
  // }

  // // Sleep by calling Wait For Interrupt and Set EVent
  // __WFI();
  // __SEV();
  // __WFI(); 
}

// RTC initialization
void initRTC(unsigned long count30) 
{
  // See "6.22 RTC - Real-time counter 6.22.10 Registers"
  NRF_CLOCK->TASKS_LFCLKSTOP = 1;
  NRF_CLOCK->LFCLKSRC = 1;      // select LFXO
  NRF_CLOCK->TASKS_LFCLKSTART = 1;
  while(NRF_CLOCK->LFCLKSTAT != 0x10001)
  
  NRF_RTC2->TASKS_STOP = 1;      // stop counter 
  NRF_RTC2->TASKS_CLEAR = 1;     // clear counter
  NRF_RTC2->PRESCALER = 0;       // set counter prescaler, fout=32768/(1+PRESCALER)　32768Hz
  NRF_RTC2->CC[0] = count30;     // set value for TRC compare register 0
  NRF_RTC2->INTENSET = 0x10000;  // enable interrupt CC[0] compare match event
  NRF_RTC2->EVTENCLR = 0x10000;  // clear counter when CC[0] event
  NVIC_SetPriority(RTC2_IRQn,3); // Higher priority than SoftDeviceAPI
  NVIC_EnableIRQ(RTC2_IRQn);     // enable interrupt  
  NRF_RTC2->TASKS_START = 1;     // start Timer
}

// RTC interrupt handler
extern "C" void RTC2_IRQHandler(void)
{
  if ((NRF_RTC2->EVENTS_COMPARE[0] != 0) && ((NRF_RTC2->INTENSET & 0x10000) != 0)) {
    NRF_RTC2->EVENTS_COMPARE[0] = 0;  // clear compare register 0 event
    NRF_RTC2->TASKS_CLEAR = 1;        // clear counter

    interruptFlag = true;
  }
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