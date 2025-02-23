#include <bluefruit.h>

const int LED = D6;

BLEClientService        service        = BLEClientService(UUID16_SVC_ENVIRONMENTAL_SENSING);
BLEClientCharacteristic characteristic = BLEClientCharacteristic(UUID16_CHR_VOLTAGE);

void setup() {
  Serial.begin(115200);
  while (!Serial.available()) { delay(10); }

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // Bluetooth initialization
  initBt();
}

void loop() {
  // Wait for response from peripheral
  Serial.println("Connecting to Peripheral");
  while (!Bluefruit.connected())
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Serial.println("Connected to Peripheral");
}

void initBt()
{
  Bluefruit.configCentralBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin(1,1); // 1 peripheral, 1 central
  Bluefruit.setName("BLE Central");

  service.begin();
  characteristic.setNotifyCallback(data_notify_callback);
  characteristic.begin();

  
  // Connection callback discovers service and characteristic and enables notification.
  Bluefruit.Central.setConnectCallback(connect_callback);

  // Scanner settings. Scan callback connects to peripheral.
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setIntervalMS(100, 50);     // interval:100mS  window:50mS  
  Bluefruit.Scanner.filterUuid(service.uuid);
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);                   // Continuous Scanning
}

// Callback invoked when scanner pick up an advertising data
void scan_callback(ble_gap_evt_adv_report_t* report)
{
  Bluefruit.Central.connect(report);
}

// Callback invoked when a connection is established
void connect_callback(uint16_t conn_handle)
{
  if ( !service.discover(conn_handle) )
  {
    Serial.println("Service not found. Disconnecting.");
    Bluefruit.disconnect(conn_handle);
    return;
  }
  Serial.println("Service found.");
  
  if ( !characteristic.discover() )
  {
    Serial.println("Characteristic not found. Disconnecting.");
    Bluefruit.disconnect(conn_handle);
    return;
  }
  Serial.println("Characteristic found.");
  
  if ( !characteristic.enableNotify() )
  {
    Serial.println("Couldn't enable notify for characteristic.");
    Bluefruit.disconnect(conn_handle);
    return;
  }
  Serial.println("Characteristic notification enabled.");
}

// Callback invoked when a notifyication is received
void data_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len)
{
  Serial.println("Notification received. Set LED high, delay, set LED low");
  digitalWrite(LED, HIGH);

  // Do something with the data
  delay(500);

  digitalWrite(LED, LOW);
}

