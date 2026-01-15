//
// Created by ckyiu on 1/14/2026.
//

#ifndef STOCK_TICKER_PICO2W_SIMPLEBUTTON_H
#define STOCK_TICKER_PICO2W_SIMPLEBUTTON_H

#include <Arduino.h>


// Inspired by https://github.com/madleech/Button
class SimpleButton {
  public:
    explicit SimpleButton(uint8_t pin, uint16_t debounce = 50);

    void begin(bool useInternalPullup = true) const;

    void end() const;

    bool read();

    bool pressed();

    bool released();

    bool toggled();

    bool hasChanged();

    static constexpr bool PRESSED = LOW;
    static constexpr bool RELEASED = HIGH;

  protected:
    uint8_t pin;
    uint16_t debounceTime;
    bool state;
    uint32_t ignoreUntil;
    bool hasChangedFlag;
};


#endif //STOCK_TICKER_PICO2W_SIMPLEBUTTON_H
