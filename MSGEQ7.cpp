#include "MSGEQ7.h"

void MSGEQ7::begin(void) {
  pinMode(_resetPin, OUTPUT);
  pinMode(_strobePin, OUTPUT);
  pinMode(_dataPin, INPUT);
  reset();
}

void MSGEQ7::reset(void) {
  digitalWrite(_strobePin, LOW);
  digitalWrite(_resetPin, HIGH);
  delayMicroseconds(1);
  digitalWrite(_resetPin, LOW);
  delayMicroseconds(72);
}

void MSGEQ7::read(bool bReset) {
  if (bReset) {
    reset();
  }

  for (int i = 0; i < MAX_BAND; i++) {
    digitalWrite(_strobePin, HIGH);
    delayMicroseconds(18);
    digitalWrite(_strobePin, LOW);

    delayMicroseconds(36);

    _data[i] = analogRead(_dataPin);

    delayMicroseconds(18);
  }
}

uint16_t MSGEQ7::get(uint8_t band) {
  if (band >= MAX_BAND) {
    return 0;
  }

  return _data[band];
}
