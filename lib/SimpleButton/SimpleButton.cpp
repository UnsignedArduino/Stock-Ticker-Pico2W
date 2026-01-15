//
// Created by ckyiu on 1/14/2026.
//

#include "SimpleButton.h"


SimpleButton::SimpleButton(const uint8_t pin,
                           const uint16_t debounce /* = 50 */) {
  this->pin = pin;
  this->debounceTime = debounce;
  this->state = HIGH;
  this->ignoreUntil = 0;
  this->hasChangedFlag = false;
}

void SimpleButton::begin(const bool useInternalPullup /* = true */) const {
  if (useInternalPullup) {
    pinMode(this->pin, INPUT_PULLUP);
  } else {
    pinMode(this->pin, INPUT);
  }
}

void SimpleButton::end() const {
  pinMode(this->pin, INPUT);
}

bool SimpleButton::read() {
  // ignore pin changes until after this delay time
  if (static_cast<int32_t>(millis() - this->ignoreUntil) < 0) {
    // ignore any changes during this period
  } else if (digitalRead(this->pin) != this->state) {
    this->ignoreUntil = millis() + this->debounceTime;
    this->state = !this->state;
    this->hasChangedFlag = true;
  }
  return this->state;
}

bool SimpleButton::hasChanged() {
  if (this->hasChangedFlag) {
    this->hasChangedFlag = false;
    return true;
  }
  return false;
}

bool SimpleButton::pressed() {
  this->read();
  return this->state == PRESSED && this->hasChanged();
}

bool SimpleButton::released() {
  this->read();
  return this->state == RELEASED && this->hasChanged();
}

bool SimpleButton::toggled() {
  this->read();
  return this->hasChanged();
}
