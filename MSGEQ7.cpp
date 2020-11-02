#include "MSGEQ7.h"

void MSGEQ7::begin(void) {
  pinMode(_resetPin, OUTPUT);
  pinMode(_strobePin, OUTPUT);
  pinMode(_dataLeftPin, INPUT);
  pinMode(_dataRightPin, INPUT);
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

    _dataLeft[i] = analogRead(_dataLeftPin);
    _dataRight[i] = analogRead(_dataRightPin);

    delayMicroseconds(18);
  }
}

uint16_t MSGEQ7::get(uint8_t side, uint8_t band) {
  if (band >= MAX_BAND) {
    return 0;
  }

  if (side == LEFT) {
    return _dataLeft[band];
  } else if (side == RIGHT) {
    return _dataRight[band];
  } else {
    return 0;
  }
}
