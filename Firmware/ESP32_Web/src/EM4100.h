#ifndef EM4100_H
#define EM4100_H

#include <Arduino.h>

// EM4100 125kHz tag emulator.
//
// Ported from the original TinyRFID DigiSpark firmware
// (Firmware/EM4100_Tx, Manchester.cpp). The bit-level encoding and the
// Manchester half-bit timing are kept identical so readers see the same
// signal; only the hosting platform (ESP32) and control path (web UI)
// change.
//
// A frame is: 9 sync '1' bits, 10 data nibbles (40 bits) each followed by
// an even row-parity bit, 4 column-parity bits, and a '0' stop bit.
class EM4100
{
  public:
    EM4100();

    // Configure the output pin that drives the coil / antenna.
    void setTxPin(uint8_t pin);

    // Set the Manchester half-bit interval in microseconds. The original
    // firmware swept this from 200..220us; the full bit is 2 * delayUs.
    void setDelayUs(uint16_t delayUs);
    uint16_t getDelayUs() const { return delay1; }

    // Transmit a single EM4100 frame. `data` must point to 5 bytes (40 bits).
    void transmitEM4100(const uint8_t *data);

  private:
    uint8_t  TxPin;
    uint16_t delay1;
    uint16_t delay2;
    bool     reverseManchester;

    void sendZero();
    void sendOne();
    void sendEvenParity(int numHighBits);
    void sendLowHigh();
    void sendHighLow();
};

#endif
