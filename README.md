# TinyRFID
The full project for the TinyRFID PCB design and source code

To Checkout:

```
git clone --recursive https://github.com/Brandon2255p/TinyRFID.git
```

## Firmware

`Firmware/` is a [PlatformIO](https://platformio.org/) project for an ESP32
EM4100 (125 kHz) tag emulator with a browser-based web interface. It supports
a half-bit **sweep** (200–220 µs) or a specific fixed rate per card, and stores
card IDs in the browser. See [`Firmware/README.md`](Firmware/README.md).

```bash
cd Firmware
pio run -t upload
```
