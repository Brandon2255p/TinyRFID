#include "EM4100.h"

// Sync stream length (9 high bits) — EM4100 header.
#define SYNC_PULSE_DEF 9

EM4100::EM4100()
{
  TxPin = 0;
  delay1 = 200;
  delay2 = 200;
  reverseManchester = false;
}

void EM4100::setTxPin(uint8_t pin)
{
  TxPin = pin;
  pinMode(TxPin, OUTPUT);
}

void EM4100::setDelayUs(uint16_t delayUs)
{
  delay1 = delayUs;
  delay2 = delayUs;
}

/*
Send data in the EM4100 Protocol.

Start with a sync stream of 9 high bits, then 5 data bytes (40 bits) grouped
into nibbles with even row parity. It ends with a 4-bit column parity nibble
and a stop bit (0). All parity is even parity.
*/
void EM4100::transmitEM4100(const uint8_t *data)
{
  const int numDataBytes = 5;

  for (int8_t i = 0; i < SYNC_PULSE_DEF; i++) // send capture / sync pulses
    sendOne();

  uint8_t parityBit0Count = 0;
  uint8_t parityBit1Count = 0;
  uint8_t parityBit2Count = 0;
  uint8_t parityBit3Count = 0;

  for (uint8_t i = 0; i < numDataBytes; i++)
  {
    uint8_t d = data[i];

    // High nibble
    uint8_t numHighBits = 0;
    if ((d & 0x80) == 0) sendZero(); else { numHighBits++; parityBit0Count++; sendOne(); }
    if ((d & 0x40) == 0) sendZero(); else { numHighBits++; parityBit1Count++; sendOne(); }
    if ((d & 0x20) == 0) sendZero(); else { numHighBits++; parityBit2Count++; sendOne(); }
    if ((d & 0x10) == 0) sendZero(); else { numHighBits++; parityBit3Count++; sendOne(); }
    sendEvenParity(numHighBits);

    // Low nibble
    numHighBits = 0;
    if ((d & 0x08) == 0) sendZero(); else { numHighBits++; parityBit0Count++; sendOne(); }
    if ((d & 0x04) == 0) sendZero(); else { numHighBits++; parityBit1Count++; sendOne(); }
    if ((d & 0x02) == 0) sendZero(); else { numHighBits++; parityBit2Count++; sendOne(); }
    if ((d & 0x01) == 0) sendZero(); else { numHighBits++; parityBit3Count++; sendOne(); }
    sendEvenParity(numHighBits);
  }

  // Column parity nibble
  (parityBit0Count % 2 == 0) ? sendZero() : sendOne();
  (parityBit1Count % 2 == 0) ? sendZero() : sendOne();
  (parityBit2Count % 2 == 0) ? sendZero() : sendOne();
  (parityBit3Count % 2 == 0) ? sendZero() : sendOne();

  // Stop bit
  sendZero();
}

void EM4100::sendEvenParity(int numHighBits)
{
  bool evenHighBits = numHighBits % 2 == 0;
  if (evenHighBits)
    sendZero();
  else
    sendOne();
}

void EM4100::sendZero()
{
  if (reverseManchester)
    sendLowHigh();
  else
    sendHighLow();
}

void EM4100::sendOne()
{
  if (reverseManchester)
    sendHighLow();
  else
    sendLowHigh();
}

void EM4100::sendLowHigh()
{
  delayMicroseconds(delay1);
  digitalWrite(TxPin, LOW);
  delayMicroseconds(delay2);
  digitalWrite(TxPin, HIGH);
}

void EM4100::sendHighLow()
{
  delayMicroseconds(delay1);
  digitalWrite(TxPin, HIGH);
  delayMicroseconds(delay2);
  digitalWrite(TxPin, LOW);
}
