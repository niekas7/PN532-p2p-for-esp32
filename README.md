# RFID Chat (ESP32 + PN532)

Simple peer-to-peer chat demo using an ESP32 (or similar) with a PN532 over I2C. One device acts as the **initiator** and sends lines typed over serial; the other is the **target** and replies with an ACK containing its random ID.

## Hardware
- ESP32 board (tested with ESP32-C3 super mini)
- PN532 NFC module (I2C mode)
- Wiring (default pins in sketch):
  - PN532 SDA -> GPIO 8
  - PN532 SCL -> GPIO 9
  - 3V3 and GND as usual (if doesn't work try 5V)

## Configure
Key defines are at the top of `rfid-chat.ino`:
- `ROLE_INITIATOR` : set to 1 for initiator (sender), 0 for target (reciever)
- `NFC_SDA` / `NFC_SCL` : I2C pins
- `I2C_SPEED` : I2C clock (default 400 kHz)
- `SERIAL_BAUD` : serial console speed (115200)

## Build & Upload
1. Open the folder in the Arduino IDE or PlatformIO.
2. Select your ESP32 board and serial port.
3. Upload `rfid-chat.ino` to two boards:
   - Board A: `ROLE_INITIATOR 1`
   - Board B: `ROLE_INITIATOR 0`

## Usage
- Open a serial monitor at 115200 on each board.
- The initiator waits for user input; type a line and press Enter to send.
- The target prints received data and replies with `ACK:<ID>`.
- If a send fails, the initiator auto-reconnects and retries on next line.

## Notes
- `BUF_SIZE` limits payload to 96 bytes; lines longer than that are truncated before sending.
- `LINE_SIZE` is 80 chars; serial input beyond that is ignored.
- Target uses `tgInitAsTarget` with a 5 s timeout; it re-arms automatically on errors.
