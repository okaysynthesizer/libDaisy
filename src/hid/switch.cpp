#include "hid/switch.h"
using namespace daisy;

void Switch::Init(dsy_gpio_pin pin,
                  float        update_rate,
                  Type         t,
                  Polarity     pol,
                  Pull         pu)
{
    last_update_ = System::GetNow();
    updated_     = false;
    state_       = 0x00;
    t_           = t;
    // Flip may seem opposite to logical direction,
    // but here 1 is pressed, 0 is not.
    flip_         = pol == POLARITY_INVERTED ? true : false;
    hw_gpio_.pin  = pin;
    hw_gpio_.mode = DSY_GPIO_MODE_INPUT;
    switch(pu)
    {
        case PULL_UP: hw_gpio_.pull = DSY_GPIO_PULLUP; break;
        case PULL_DOWN: hw_gpio_.pull = DSY_GPIO_PULLDOWN; break;
        case PULL_NONE: hw_gpio_.pull = DSY_GPIO_NOPULL; break;
        default: hw_gpio_.pull = DSY_GPIO_PULLUP; break;
    }
    dsy_gpio_init(&hw_gpio_);
}
void Switch::Init(dsy_gpio_pin pin, float update_rate)
{
    Init(pin, update_rate, TYPE_MOMENTARY, POLARITY_INVERTED, PULL_UP);
}

void Switch::Debounce()
{
    // eurorack-bingo-drums: sample every call instead of throttling to
    // 1kHz. ProcessAllControls() already calls Debounce() once per audio
    // callback (~83us @ 48kHz/block-size-4), so the stock 1ms throttle
    // discarded ~11 of every 12 calls and stretched the 8-tap debounce
    // chain (first "pressed" read to state_==0xff) out to ~8ms worst case.
    // Removing the throttle lets the same 8-tap chain complete in ~0.7ms —
    // still 8 consecutive matching reads (same bounce-rejection sample
    // count), just compressed into less wall-clock time. Trade-off: a
    // switch bouncing for longer than ~0.7ms could now slip through where
    // it wouldn't have before; dial this back (e.g. throttle to every Nth
    // call) if that turns out to be a problem in practice.
    uint32_t now = System::GetNow();
    last_update_  = now;
    updated_      = true;

    // shift over, and introduce new state.
    state_
        = (state_ << 1)
          | (flip_ ? !dsy_gpio_read(&hw_gpio_) : dsy_gpio_read(&hw_gpio_));
    // Set time at which button was pressed
    if(state_ == 0x7f)
        rising_edge_time_ = System::GetNow();
}
