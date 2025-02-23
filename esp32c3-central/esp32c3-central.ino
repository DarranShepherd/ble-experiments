#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define UUID16_SVC_ENVIRONMENTAL_SENSING                      0x181A
#define UUID16_CHR_VOLTAGE                                    0x2B18

const int LED = D6;

// The remote service we wish to connect to.
static BLEUUID serviceUUID((uint16_t)UUID16_SVC_ENVIRONMENTAL_SENSING);
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID((uint16_t)UUID16_CHR_VOLTAGE);

static BLEAdvertisedDevice *myDevice;
static BLERemoteCharacteristic *pRemoteCharacteristic;

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(2500);
  digitalWrite(LED, LOW);

  Serial.println("ESP32C3 Central");

  // Bluetooth initialization
  initBt();
}

void loop() {
// If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToPeripheral()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
  }

  if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(1000);  // Delay a second between loops.
}

// Callback invoked when scanner pick up an advertising data
class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

class ClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {
    Serial.println("onConnect");
  }

  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

void initBt()
{
  BLEDevice::init("");
  BLEScan *bleScan = BLEDevice::getScan();
  bleScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  bleScan->setInterval(100);
  bleScan->setWindow(50);
  bleScan->setActiveScan(true);
  bleScan->start(0);
}


bool connectToPeripheral() {
  Serial.print("Connecting to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new ClientCallbacks());
  pClient->connect(myDevice);

  Serial.println("Connect returned");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  if (!pRemoteCharacteristic->canNotify()) {
    Serial.println("Characteristic can't notify");
    pClient->disconnect();
    return false;
  }

  pRemoteCharacteristic->registerForNotify(notifyCallback);

  Serial.println("Registered for notification");
  connected = true;
  return true;
}

// Callback invoked when a notification is received
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  Serial.println("Notification received. Set LED high, delay, set LED low");
  digitalWrite(LED, HIGH);

  // Do something with the data
  delay(100);

  digitalWrite(LED, LOW);
}
