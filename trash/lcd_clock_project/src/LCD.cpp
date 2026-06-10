#include "LCD.hpp"
#include <bcm2835.h>
#include <unistd.h>

LCD::LCD() : initialized(false) {}

LCD::~LCD() {
    // No cerramos bcm2835 aquí porque el main lo hace
}

void LCD::init() {
    // Configurar pines como salida
    bcm2835_gpio_fsel(PIN_RS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_EN, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_D4, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_D5, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_D6, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_D7, BCM2835_GPIO_FSEL_OUTP);

    // Estado inicial bajo
    bcm2835_gpio_write(PIN_RS, LOW);
    bcm2835_gpio_write(PIN_EN, LOW);
    bcm2835_gpio_write(PIN_D4, LOW);
    bcm2835_gpio_write(PIN_D5, LOW);
    bcm2835_gpio_write(PIN_D6, LOW);
    bcm2835_gpio_write(PIN_D7, LOW);

    bcm2835_delay(50);

    // Secuencia de inicialización en modo 4 bits
    bcm2835_gpio_write(PIN_RS, LOW);
    sendNibble(0x03);
    bcm2835_delay(5);
    sendNibble(0x03);
    bcm2835_delay(1);
    sendNibble(0x03);
    sendNibble(0x02);   // Cambiar a modo 4 bits

    sendCommand(0x28);   // 4 bits, 2 líneas, 5x8 puntos
    sendCommand(0x08);   // Display off
    sendCommand(0x01);   // Clear
    bcm2835_delay(2);
    sendCommand(0x06);   // Incremento cursor, sin desplazar
    sendCommand(0x0C);   // Display on, cursor off, blink off

    initialized = true;
}

void LCD::enablePulse() {
    bcm2835_gpio_write(PIN_EN, HIGH);
    bcm2835_delayMicroseconds(1);
    bcm2835_gpio_write(PIN_EN, LOW);
    bcm2835_delayMicroseconds(50);
}

void LCD::sendNibble(uint8_t nibble) {
    bcm2835_gpio_write(PIN_D4, (nibble >> 0) & 1);
    bcm2835_gpio_write(PIN_D5, (nibble >> 1) & 1);
    bcm2835_gpio_write(PIN_D6, (nibble >> 2) & 1);
    bcm2835_gpio_write(PIN_D7, (nibble >> 3) & 1);
    enablePulse();
}

void LCD::sendCommand(uint8_t cmd) {
    bcm2835_gpio_write(PIN_RS, LOW);
    sendNibble(cmd >> 4);
    sendNibble(cmd & 0x0F);
    bcm2835_delayMicroseconds(100);
}

void LCD::sendData(uint8_t data) {
    bcm2835_gpio_write(PIN_RS, HIGH);
    sendNibble(data >> 4);
    sendNibble(data & 0x0F);
    bcm2835_delayMicroseconds(50);
}

void LCD::setCursor(uint8_t row, uint8_t col) {
    static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    sendCommand(0x80 | (col + row_offsets[row]));
}

void LCD::writeString(const std::string& text) {
    for (char c : text) {
        sendData(static_cast<uint8_t>(c));
    }
}

void LCD::print(const std::string& text) {
    writeString(text);
}

void LCD::clear() {
    sendCommand(0x01);
    bcm2835_delay(2);
}
