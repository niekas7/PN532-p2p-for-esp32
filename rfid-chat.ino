#include <Wire.h>
#include <esp_random.h>
#include "PN532_I2C.h"
#include <PN532.h>

// ================= CONFIG =================
#define ROLE_INITIATOR 0     // 1 = initiator, 0 = target
#define NFC_SDA 8
#define NFC_SCL 9
#define I2C_SPEED 400000
#define SERIAL_BAUD 115200
#define BUF_SIZE 96
#define LINE_SIZE 80
// ==========================================

PN532_I2C pn532_i2c(Wire);
PN532 nfc(pn532_i2c);

uint8_t rxBuf[BUF_SIZE];
uint8_t txBuf[BUF_SIZE];

char lineBuf[LINE_SIZE];
size_t lineLen = 0;

uint32_t myId;
bool linkActive = false;

// =====================================================
// Utilities
// =====================================================
void clearLine() {
  memset(lineBuf, 0, sizeof(lineBuf));
  lineLen = 0;
}

bool readSerialLine() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      lineBuf[lineLen] = '\0';
      return (lineLen > 0);
    }

    if (lineLen < LINE_SIZE - 1)
      lineBuf[lineLen++] = c;
  }
  return false;
}

void resetLink() {
  nfc.inRelease();
  linkActive = false;
}

// =====================================================
// INITIATOR MODE
// =====================================================
bool connectToTarget() {
  if (nfc.inJumpForDEP(0x00, true)) {
    Serial.println("Connected to target");
    linkActive = true;
    return true;
  }
  return false;
}

void initiatorLoop() {

  if (!linkActive) {
    if (!connectToTarget()) {
      delay(200);
      return;
    }
  }

  if (!readSerialLine())
    return;

  size_t len = strlen(lineBuf);
  if (len > BUF_SIZE)
    len = BUF_SIZE;

  memcpy(txBuf, lineBuf, len);

  uint8_t respLen = BUF_SIZE;
  bool ok = nfc.inDataExchange(txBuf, len, rxBuf, &respLen);

  if (!ok) {
    Serial.println("Send failed — reconnecting");
    resetLink();
  } else {
    Serial.print("TX: ");
    Serial.write(txBuf, len);
    Serial.println();

    if (respLen) {
      Serial.print("RX: ");
      Serial.write(rxBuf, respLen);
      Serial.println();
    }
  }

  clearLine();
}

// =====================================================
// TARGET MODE
// =====================================================
bool armTarget() {
  int8_t r = nfc.tgInitAsTarget(5000);
  if (r > 0) {
    Serial.println("Target ready");
    return true;
  }
  return false;
}

void targetLoop() {

  static bool armed = false;

  if (!armed) {
    armed = armTarget();
    if (!armed) {
      delay(200);
      return;
    }
  }

  int16_t len = nfc.tgGetData(rxBuf, BUF_SIZE);

  if (len > 0) {

    Serial.print("RX: ");
    Serial.write(rxBuf, len);
    Serial.println();

    char ack[32];
    int ackLen = snprintf(ack, sizeof(ack),
                          "ACK:%08lX", (unsigned long)myId);

    if (!nfc.tgSetData((uint8_t*)ack, ackLen)) {
      Serial.println("ACK failed — rearming");
      armed = false;
    }

  } else if (len < 0) {
    armed = false;
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(SERIAL_BAUD);
  delay(200);

  Wire.begin(NFC_SDA, NFC_SCL);
  Wire.setClock(I2C_SPEED);

  pn532_i2c.begin();
  nfc.begin();

  nfc.SAMConfig();
  nfc.setPassiveActivationRetries(0x10);

  myId = esp_random();

  Serial.println();
  Serial.println(ROLE_INITIATOR ? "MODE: INITIATOR" : "MODE: TARGET");
  Serial.printf("ID: %08lX\n", (unsigned long)myId);
}

// =====================================================
// MAIN LOOP
// =====================================================
void loop() {
#if ROLE_INITIATOR
  initiatorLoop();
#else
  targetLoop();
#endif
}