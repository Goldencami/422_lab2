#define BUZZER_PIN 8
#include <SoftwareSerial.h>
#define TTGO_RX 10
#define TTGO_TX 11
SoftwareSerial ttgoSerial(TTGO_RX, TTGO_TX);

bool isSongPlaying = false;

struct Song {
  String name = "undefined";
  int tempo;
  int melody[50];
  int length = 0;
};

void play(Song object) {
  isSongPlaying = true;
  int notes = object.length / 2;
  int wholenote = (60000 * 4) / object.tempo;
  int divider = 0, noteDuration = 0;

  // iterate over the notes of the melody.
  // Remember, the array is twice the number of notes (notes + durations)
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
    // calculates the duration of each note
    divider = object.melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(BUZZER_PIN, object.melody[thisNote], noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(BUZZER_PIN);
  }

  isSongPlaying = false;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  // Wait for song data or status request from TTGO
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');

    if (cmd == "STATUS") {
      // TTGO wants to know if song is playing
      Serial.println(isSongPlaying ? "1" : "0");
    } 
    else {
      Serial.println("Got song!");
      // assume it's song data
      Song toPlay;
      toPlay.tempo = cmd.toInt();        // first line: tempo
      toPlay.length = Serial.parseInt(); // second line: length
      for (int i = 0; i < toPlay.length; i++) {
        toPlay.melody[i] = Serial.parseInt(); // melody data
      }

      Serial.println("READY");
      play(toPlay);
    }
  }
}

