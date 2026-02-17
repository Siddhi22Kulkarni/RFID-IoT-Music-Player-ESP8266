#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

/* ===== RFID ===== */
#define SS_PIN   D8
#define RST_PIN  D0
MFRC522 mfrc522(SS_PIN, RST_PIN);

/* ===== DFPLAYER ===== */
SoftwareSerial dfSerial(D2, D1);   // RX, TX
DFRobotDFPlayerMini dfPlayer;

/* ===== CARD DATA ===== */
#define TOTAL_CARDS 3
#define UID_SIZE 4

byte cards[TOTAL_CARDS][UID_SIZE] = {
  {0xE3, 0x58, 0x7C, 0x2E},  // Card 1 → Song 1
  {0x72, 0x22, 0xD7, 0x5C},  // Card 2 → Song 2
  {0x83, 0x25, 0x47, 0xFB}   // Card 3 → Song 3
};

/* ===== FUNCTIONS ===== */
byte getCardNumber(byte *uid, byte uidSize) {
  if (uidSize != UID_SIZE) return 0;

  for (byte c = 0; c < TOTAL_CARDS; c++) {
    bool match = true;
    for (byte i = 0; i < UID_SIZE; i++) {
      if (uid[i] != cards[c][i]) {
        match = false;
        break;
      }
    }
    if (match) return c + 1;   // Card numbers start from 1
  }
  return 0;
}

void setup() {
  Serial.begin(9600);
  delay(500);

  /* RFID INIT */
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID Ready");

  /* DFPLAYER INIT */
  dfSerial.begin(9600);
  delay(1500);   // DFPlayer power-up delay

  Serial.println("Initializing DFPlayer...");
  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer not detected!");
    while (true) {
      yield();   // keep watchdog happy
    }
  }

  dfPlayer.volume(20); // 0–30
  Serial.println("DFPlayer Ready");
  Serial.println("System Ready – Scan RFID Card");
}

void loop() {
  yield();  // ESP8266 watchdog

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  /* PRINT UID */
  Serial.print("UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    Serial.print(" ");
  }

  /* GET CARD NUMBER */
  byte cardNo = getCardNumber(mfrc522.uid.uidByte,
                              mfrc522.uid.size);

  if (cardNo > 0) {
    Serial.print(" | Card ");
    Serial.print(cardNo);
    Serial.println(" detected");
    
    Serial.print("Playing Song ");
    Serial.println(cardNo);

    dfPlayer.playMp3Folder(cardNo); // 0001.mp3, 0002.mp3, 0003.mp3
  } else {
    Serial.println(" | Unknown Card");
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(1500); // prevent repeated reads
}