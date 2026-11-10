# TinyRFID
The full project for the TinyRFID PCB design and source code

To Checkout:

```
git clone --recursive https://github.com/Brandon2255p/TinyRFID.git
```

## Firmware

- `Firmware/EM4100_Tx` — original DigiSpark/ATmega EM4100 (125 kHz) tag
  emulator (git submodule).
- `Firmware/ESP32_Web` — ESP32 upgrade with a browser-based web interface.
  Keeps the same half-bit **sweep** behavior (200–220 µs) or lets you set a
  specific fixed rate per card, and stores card IDs in the browser. See
  [`Firmware/ESP32_Web/README.md`](Firmware/ESP32_Web/README.md).
