#ifndef LCD_HPP
#define LCD_HPP

#include <cstdint>
#include <string>

class LCD {
public:
    LCD();
    ~LCD();
    void init();
    void clear();
    void setCursor(uint8_t row, uint8_t col);
    void writeString(const std::string& text);
    void print(const std::string& text);
private:
    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void sendNibble(uint8_t nibble);
    void enablePulse();
    static constexpr int PIN_RS = 13;
    static constexpr int PIN_EN = 6;
    static constexpr int PIN_D4 = 26;
    static constexpr int PIN_D5 = 19;
    static constexpr int PIN_D6 = 20;
    static constexpr int PIN_D7 = 21;
    bool initialized;
};

#endif
