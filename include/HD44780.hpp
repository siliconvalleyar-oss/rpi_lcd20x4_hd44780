#pragma once

#include <cstdint>
#include <string>

class HD44780 {
public:
    struct Pins {
        uint8_t RS;
        uint8_t EN;
        uint8_t D4;
        uint8_t D5;
        uint8_t D6;
        uint8_t D7;
    };

    HD44780(uint8_t cols, uint8_t rows, Pins pins);
    ~HD44780();

    bool begin();
    void clear();
    void home();
    void setCursor(uint8_t col, uint8_t row);
    void write(char c);
    void write(const std::string& str);
    void print(const std::string& str);
    void printf(const char* format, ...);

    void display(bool on);
    void cursor(bool on);
    void blink(bool on);
    void backlight(bool on);

    void createChar(uint8_t location, const uint8_t charmap[8]);
    void writeChar(uint8_t location);

    void scrollLeft(uint8_t cols = 1);
    void scrollRight(uint8_t cols = 1);
    void moveCursorLeft(uint8_t cols = 1);
    void moveCursorRight(uint8_t cols = 1);

    void setEntryMode(bool increment, bool shift);
    void clearLine(uint8_t row);

    uint8_t getCols() const { return _cols; }
    uint8_t getRows() const { return _rows; }

    static constexpr uint8_t ROW_OFFSETS[4] = {0x00, 0x40, 0x14, 0x54};

private:
    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void sendNibble(uint8_t nibble);
    void pulseEnable();

    void write4Bits(uint8_t value);
    void write8Bits(uint8_t value, bool isData);

    uint8_t _cols;
    uint8_t _rows;
    Pins _pins;
    bool _initialized;
    bool _backlight;
    bool _displayOn;
    bool _cursorOn;
    bool _blinkOn;
};
