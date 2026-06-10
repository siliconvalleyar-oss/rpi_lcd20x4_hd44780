#include "HD44780.hpp"
#include <bcm2835.h>
#include <cstdarg>
#include <cstring>
#include <cstdio>

HD44780::HD44780(uint8_t cols, uint8_t rows, Pins pins)
    : _cols(cols), _rows(rows), _pins(pins), _initialized(false),
      _backlight(true), _displayOn(true), _cursorOn(false), _blinkOn(false)
{
}

HD44780::~HD44780()
{
}

bool HD44780::begin()
{
    bcm2835_gpio_fsel(_pins.RS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(_pins.EN, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(_pins.D4, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(_pins.D5, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(_pins.D6, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(_pins.D7, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_gpio_write(_pins.RS, LOW);
    bcm2835_gpio_write(_pins.EN, LOW);
    bcm2835_gpio_write(_pins.D4, LOW);
    bcm2835_gpio_write(_pins.D5, LOW);
    bcm2835_gpio_write(_pins.D6, LOW);
    bcm2835_gpio_write(_pins.D7, LOW);

    bcm2835_delay(50);

    bcm2835_gpio_write(_pins.RS, LOW);

    sendNibble(0x03);
    bcm2835_delay(5);
    sendNibble(0x03);
    bcm2835_delay(1);
    sendNibble(0x03);
    sendNibble(0x02);

    sendCommand(0x28);
    sendCommand(0x08);
    sendCommand(0x01);
    bcm2835_delay(2);
    sendCommand(0x06);
    sendCommand(0x0C);

    _initialized = true;
    return true;
}

void HD44780::pulseEnable()
{
    bcm2835_gpio_write(_pins.EN, HIGH);
    bcm2835_delayMicroseconds(1);
    bcm2835_gpio_write(_pins.EN, LOW);
    bcm2835_delayMicroseconds(50);
}

void HD44780::sendNibble(uint8_t nibble)
{
    bcm2835_gpio_write(_pins.D4, (nibble >> 0) & 1);
    bcm2835_gpio_write(_pins.D5, (nibble >> 1) & 1);
    bcm2835_gpio_write(_pins.D6, (nibble >> 2) & 1);
    bcm2835_gpio_write(_pins.D7, (nibble >> 3) & 1);
    pulseEnable();
}

void HD44780::sendCommand(uint8_t cmd)
{
    bcm2835_gpio_write(_pins.RS, LOW);
    sendNibble(cmd >> 4);
    sendNibble(cmd & 0x0F);
    bcm2835_delayMicroseconds(100);
}

void HD44780::sendData(uint8_t data)
{
    bcm2835_gpio_write(_pins.RS, HIGH);
    sendNibble(data >> 4);
    sendNibble(data & 0x0F);
    bcm2835_delayMicroseconds(50);
}

void HD44780::write4Bits(uint8_t value)
{
    sendNibble(value);
}

void HD44780::write8Bits(uint8_t value, bool isData)
{
    if (isData)
        sendData(value);
    else
        sendCommand(value);
}

void HD44780::setCursor(uint8_t col, uint8_t row)
{
    if (row >= _rows) row = _rows - 1;
    if (col >= _cols) col = _cols - 1;
    uint8_t offset = ROW_OFFSETS[row];
    if (row >= 2 && _cols == 16) {
        offset = (row == 2) ? 0x10 : 0x50;
    }
    sendCommand(0x80 | (col + offset));
}

void HD44780::write(char c)
{
    sendData(static_cast<uint8_t>(c));
}

void HD44780::write(const std::string& str)
{
    for (char c : str)
        sendData(static_cast<uint8_t>(c));
}

void HD44780::print(const std::string& str)
{
    write(str);
}

void HD44780::printf(const char* format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    write(std::string(buffer));
}

void HD44780::clear()
{
    sendCommand(0x01);
    bcm2835_delay(2);
}

void HD44780::home()
{
    sendCommand(0x02);
    bcm2835_delay(2);
}

void HD44780::display(bool on)
{
    _displayOn = on;
    sendCommand(0x08 | (_displayOn << 2) | (_cursorOn << 1) | _blinkOn);
}

void HD44780::cursor(bool on)
{
    _cursorOn = on;
    sendCommand(0x08 | (_displayOn << 2) | (_cursorOn << 1) | _blinkOn);
}

void HD44780::blink(bool on)
{
    _blinkOn = on;
    sendCommand(0x08 | (_displayOn << 2) | (_cursorOn << 1) | _blinkOn);
}

void HD44780::backlight(bool on)
{
    _backlight = on;
}

void HD44780::createChar(uint8_t location, const uint8_t charmap[8])
{
    if (location > 7) return;
    sendCommand(0x40 + (location * 8));
    for (int i = 0; i < 8; i++)
        sendData(charmap[i]);
    sendCommand(0x80);
}

void HD44780::writeChar(uint8_t location)
{
    if (location > 7) return;
    sendData(location);
}

void HD44780::scrollLeft(uint8_t cols)
{
    for (uint8_t i = 0; i < cols; i++)
        sendCommand(0x18);
}

void HD44780::scrollRight(uint8_t cols)
{
    for (uint8_t i = 0; i < cols; i++)
        sendCommand(0x1E);
}

void HD44780::moveCursorLeft(uint8_t cols)
{
    for (uint8_t i = 0; i < cols; i++)
        sendCommand(0x10);
}

void HD44780::moveCursorRight(uint8_t cols)
{
    for (uint8_t i = 0; i < cols; i++)
        sendCommand(0x14);
}

void HD44780::setEntryMode(bool increment, bool shift)
{
    sendCommand(0x04 | (increment << 1) | shift);
}

void HD44780::clearLine(uint8_t row)
{
    setCursor(0, row);
    for (uint8_t i = 0; i < _cols; i++)
        sendData(' ');
}
