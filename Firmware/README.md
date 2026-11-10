# TinyRFID Firmware — ESP32 + Web Interface

The TinyRFID firmware: an ESP32 EM4100 (125 kHz) tag emulator that serves a
small web app so you can store card IDs **in your browser** and transmit any
of them on demand. This is a PlatformIO project.

The low-level EM4100 encoding and Manchester half-bit timing match the
established EM4100 protocol so readers see a standard signal.

## Features

- **Rate sweep** — the Manchester half-bit delay is swept across a range
  (default **200–220 µs**, 20 frames per step) so readers with slightly
  different clock tolerances still read the card.
- **Per-card fixed rate** — alternatively, pin a card to one exact half-bit
  delay instead of sweeping.
- **Browser-stored cards** — card IDs live in the browser's `localStorage`
  (nothing is persisted on the device). Add, edit, delete, import and export
  them. The selected card is POSTed to the device to start transmitting.

## Hardware

- Any ESP32 dev board.
- A coil/antenna driven from `EM4100_TX_PIN` (default **GPIO4**, set in
  `platformio.ini`).

## Build & flash (PlatformIO)

```bash
cd Firmware
pio run -t upload
pio device monitor
```

## Use

1. After boot the ESP32 starts a WiFi access point:
   - SSID: `TinyRFID`
   - Password: `tinyrfid123`
2. Connect to it and open <http://192.168.4.1/>.
3. Add a card (name + 10-hex-character EM4100 ID, e.g. `307FA2535A`), choose
   **Sweep** or **Fixed rate**, then tap **Transmit**. Tap **Stop transmit**
   to stop.

Change the AP SSID/password in `src/main.cpp`.

## HTTP API

| Method | Path            | Body (form-urlencoded)                                   | Purpose                         |
|--------|-----------------|----------------------------------------------------------|---------------------------------|
| GET    | `/`             | —                                                        | Web UI                          |
| GET    | `/api/status`   | —                                                        | Current transmit state (JSON)   |
| POST   | `/api/transmit` | `id`, `mode=sweep\|fixed`, `sweepMin`, `sweepMax`, `fixedDelay`, `repeat` | Start transmitting a card |
| POST   | `/api/stop`     | —                                                        | Stop transmitting               |
