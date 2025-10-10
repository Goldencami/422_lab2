#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#define SERIAL_TX 17
#define SERIAL_RX 16

const char* ssid = "BELL892";
const char* password = "1E7C373CF727";

String BASE_URL = "https://iotjukebox.onrender.com";
String STUDENT_ID = "40239038";
String DEVICE1_NAME = "iPhoneCamila";

struct Song {
  String name;
  int tempo;
  int melody[50];
  int length;
};

Song httpGET(String endpoint) {
  Song randomSong;
  if (WiFi.status() != WL_CONNECTED) return randomSong; // don't continue if there's no wifi

  HTTPClient http;
  http.begin(BASE_URL + endpoint);

  int httpResponseCode = http.GET();
  String payload = "";

  if (httpResponseCode > 0) {
    Serial.println("HTTP GET Response: " + String(httpResponseCode));
    payload = http.getString();
    Serial.println(payload);
  }
  else {
    Serial.println("HTTP GET Error: " + String(httpResponseCode));
    return randomSong;
  }

  http.end(); // Free resources

  JSONVar songObj = JSON.parse(payload);
  if (JSON.typeof(songObj) == "undefined" || !songObj.hasOwnProperty("melody")) {
    Serial.println("Parsing input failed!");
    return randomSong;
  }

  randomSong.name = (const char*) songObj["name"]; // converts JSON string into a string C++ can accept
  randomSong.tempo = atoi((const char*) songObj["tempo"]); // converts string to an integer

  JSONVar melodyArr = songObj["melody"];
  randomSong.length = songObj["melody"].length();

  for (int i = 0; i < randomSong.length; i++) {
    randomSong.melody[i] = atoi((const char*) melodyArr[i]);
  }

  return randomSong;
}

Song getSong(String song_name) {
  return httpGET("/song?name=" + song_name);
}

Song getPreferedSong(String student_id, String device) {
  Song emptySong;
  if (WiFi.status() != WL_CONNECTED) return emptySong;

  HTTPClient http;
  http.begin(BASE_URL + "/preference?id=" + student_id + "&key=" + device);

  int httpResponseCode = http.GET(); // will only return the name of the prefered song of the linked device
  String payload = "";

  if (httpResponseCode > 0) {
    payload = http.getString();
  }
  else {
    Serial.println("HTTP GET Error: " + String(httpResponseCode));
    return emptySong;
  }
  http.end(); // Free resources

  JSONVar songObj = JSON.parse(payload);
  if (JSON.typeof(songObj) == "undefined") {
    Serial.println("Parsing input failed!");
    return emptySong;
  }

  String song_name = (const char*) songObj["name"]; // converts JSON string into a string C++ can accept
  return httpGET("/song?name=" + song_name);
}

void postDevice(String student_id, String device, String song_name) {
  if (WiFi.status() != WL_CONNECTED) return ;

  String endpoint = "/preference?id=" + student_id + "&key=" + device + "&value=" + song_name;

  HTTPClient http;
  http.begin(BASE_URL + endpoint);
  int httpResponseCode = http.POST(""); // Send HTTP POST request

  if (httpResponseCode > 0) {
    Serial.println("HTTP POST Response: " + String(httpResponseCode));
  }
  else {
    Serial.println("HTTP POST Error: " + String(httpResponseCode));
    return;
  }
  
  http.end(); // Free resources
}

bool isNanoReady() {
  Serial2.println("STATUS"); // ask Nano
  unsigned long start = millis();
  while (!Serial2.available()) {
    if (millis() - start > 1000) return false; // timeout
  }
  String status = Serial2.readStringUntil('\n');
  return status == "0"; // 0 = not playing
}


bool sendSongToNano(Song song) {
  Serial2.println(song.tempo);
  Serial2.println(song.length);
  for (int i = 0; i < song.length; i++)
    Serial2.println(song.melody[i]);

  // Wait for acknowledgment from Nano
  unsigned long start = millis();
  while (!Serial2.available()) {
    if (millis() - start > 1000) return false; // timeout
  }
  String ack = Serial2.readStringUntil('\n');
  return ack == "READY";
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, SERIAL_RX, SERIAL_TX);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  Serial.println("TTGO connected to WiFi!");
  Serial.println("POST DEVICE");
  postDevice(STUDENT_ID, DEVICE1_NAME, "harrypotter");
}

void loop() {
  if (isNanoReady()) {
    Serial.println("Nano is free!");
    Song toPlay = getPreferedSong(STUDENT_ID, DEVICE1_NAME);
    if (sendSongToNano(toPlay))
      Serial.println("Song successfully sent to Nano!");
    else
      Serial.println("Failed to send song. Retry next loop.");
  } else {
    Serial.println("Nano is busy, waiting...");
  }

  delay(1000);
}