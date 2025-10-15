#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#define BUZZER_PIN 21
#define QUEUE_LENGTH 10

const char *ssid = "BELL892";
const char *password = "1E7C373CF727";
// const char *ssid = "iPhoneCamila"; // my hotspot
// const char *password = "Nicolas19";

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

struct Song {
  String name = "undefined";
  int tempo;
  int melody[50];
  int length = 0;
};

class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
  }

  void onDisconnect(NimBLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
  }
};

class MyCallbacks : public NimBLECharacteristicCallbacks {
  private:
    String rxValue;

  public:
    void onWrite(NimBLECharacteristic* pCharacteristic) {
      rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();
        Serial.println("*********");
      }
    }

    String getValue() { 
      return rxValue; 
    }
    
    void clearValue() { 
      rxValue = "";
    }
};

Song queue[QUEUE_LENGTH];
int currentIdx = 0; // refering to the current song playing

int currentNote = 0;
unsigned long noteStartTime = 0;
bool isPlaying = false;
bool isPaused = false;
Song currentSong;
int noteDuration = 0;
MyCallbacks myCallbacks;


/*****************/
//    QUEUE
/*****************/
void createPlaylist() {
  for(int i=0; i < QUEUE_LENGTH; i++) {
    if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;
      http.begin("https://iotjukebox.onrender.com/song");
      int httpResponseCode = http.GET();

      if(httpResponseCode > 0) {
        Serial.println("HTTP GET Response: " + String(httpResponseCode));
        String payload = http.getString();
        Serial.println(payload);
        Song genSong = httpGET(payload); // refers to generated song
        queue[i] = genSong;
      }
      else {
        Serial.println("HTTP GET Error: " + String(httpResponseCode));
      }
      http.end(); // Free resources
    }
  }
}

/*****************/
//     SONG
/*****************/
void play(Song object) {
  currentSong = object;
  currentNote = 0;
  noteStartTime = millis();
  isPlaying = true;
  isPaused = false;

  int notes = currentSong.length / 2;
  int wholenote = (60000 * 4) / currentSong.tempo;
  int divider = currentSong.melody[currentNote + 1];

  if (divider > 0) {
    noteDuration = wholenote / divider;
  } else {
    noteDuration = wholenote / abs(divider) * 1.5;
  }

  tone(BUZZER_PIN, currentSong.melody[currentNote], noteDuration * 0.9);
}

void updateSong() {
  if (!isPlaying || isPaused) return;

  if (millis() - noteStartTime >= noteDuration) {
    // Stop current note
    noTone(BUZZER_PIN);

    // Move to next note
    currentNote += 2;
    if (currentNote >= currentSong.length) {
      isPlaying = false; // Song finished
      return;
    }

    int notes = currentSong.length / 2;
    int wholenote = (60000 * 4) / currentSong.tempo;
    int divider = currentSong.melody[currentNote + 1];

    if (divider > 0) {
      noteDuration = wholenote / divider;
    } else {
      noteDuration = wholenote / abs(divider) * 1.5;
    }

    tone(BUZZER_PIN, currentSong.melody[currentNote], noteDuration * 0.9);
    noteStartTime = millis();
  }
}

void togglePause() {
  if (!isPlaying) return;

  if (isPaused) {
    // resume
    isPaused = false;
    noteStartTime = millis(); // restart timer
    tone(BUZZER_PIN, currentSong.melody[currentNote], noteDuration * 0.9);
  } else {
    // pause
    isPaused = true;
    noTone(BUZZER_PIN);
  }
}

Song httpGET(String payload) {
  Song song;
  DynamicJsonDocument doc(1024);

  // Read the JSON document received from the API
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return song;
  }
  // Verify the fields are present
  if (!doc.containsKey("name") || !doc.containsKey("tempo") || !doc.containsKey("melody")) {
    Serial.println("JSON missing required fields!");
    return song;
  }

  song.name = doc["name"].as<String>();
  song.tempo = doc["tempo"].as<int>();
  JsonArray melodyArr = doc["melody"].as<JsonArray>();
  song.length = melodyArr.size();

  // Limit to avoid overflow (since melody[] has 50 elements)
  song.length = min(song.length, 50);

  for (int i = 0; i < song.length; i++) {
    song.melody[i] = melodyArr[i].as<int>();
  }
  
  return song;
}

// void setupBluetooth() {
//   // Initialize NimBLE
//   NimBLEDevice::init("TTGO Service");

//   // Create BLE server
//   pServer = NimBLEDevice::createServer();
//   pServer->setCallbacks(new MyServerCallbacks());

//   // Create BLE service
//   NimBLEService* pService = pServer->createService(SERVICE_UUID);

//   // TX characteristic (notify)
//   pTxCharacteristic = pService->createCharacteristic(
//       CHARACTERISTIC_UUID_TX,
//       NIMBLE_PROPERTY::NOTIFY
//   );
//   pTxCharacteristic->addDescriptor(new NimBLE2902());

//   // RX characteristic (write)
//   NimBLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
//       CHARACTERISTIC_UUID_RX,
//       NIMBLE_PROPERTY::WRITE
//   );
//   pRxCharacteristic->setCallbacks(&myCallbacks);

//   // Start service and advertising
//   pService->start();
//   pServer->getAdvertising()->start();
//   Serial.println("Waiting a client connection to notify...");
// }

void setup() {
  Serial.begin(115200);

  // Connect WiFi first
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  createPlaylist();

  // setupBluetooth();
}

void loop() {
  if(!isPlaying) {
    play(queue[currentIdx]);
    ++currentIdx;
  }
  else if(rxValue == "!B813") { // -> button
    Serial.println("Next song");
    if(currentIdx == (QUEUE_LENGTH-1)) {
      ++currentIdx;
      isPlaying = false;
    }
  }
  else if(rxValue == "!B714") { // <- button
    Serial.println("Previous song");
    if(currentIdx == 0) {
      --currentIdx;
      isPlaying = false;
    }
  }
  else if(rxValue == "!B219") { // 2 button
    Serial.println("Play/Pause song");
  }
  // if(WiFi.status() == WL_CONNECTED) {
  //   HTTPClient http;
  //   http.begin("https://iotjukebox.onrender.com/song");
  //   int httpResponseCode = http.GET();

  //   if(httpResponseCode > 0) {
  //     Serial.println("HTTP GET Response: " + String(httpResponseCode));
  //     String payload = http.getString();
  //     Serial.println(payload);
  //     if(!isPlaying) {
  //       Song toPlay = httpGET(payload);
  //       play(toPlay);
  //     }
  //     else {
  //       // add song to queue
  //     }

  //   }
  //   else {
  //     Serial.println("HTTP GET Error: " + String(httpResponseCode));
  //   }
  //   http.end(); // Free resources
  // }
}