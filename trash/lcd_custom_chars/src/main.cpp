#include <iostream>
#include <memory>
#include <unistd.h>
#include <string>
#include <cstring>
#include <ctime>
#include <bcm2835.h>
#include "LCD.hpp"

// ------------------------------------------------------------------
// Definición de los mapas de bits para caracteres personalizados
// ------------------------------------------------------------------
// Batería vacía
const uint8_t battery_EMP[8] = {
    0b00000,
    0b01110,
    0b11111,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111
};

// Batería media
const uint8_t battery_HLF[8] = {
    0b00000,
    0b01110,
    0b11111,
    0b10001,
    0b10001,
    0b11111,
    0b11111,
    0b11111
};

// Batería llena
const uint8_t battery_FULL[8] = {
    0b00000,
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
};

// Candado cerrado
const uint8_t Locked[8] = {
    0b00000,
    0b01110,
    0b10001,
    0b10001,
    0b11111,
    0b11111,
    0b11011,
    0b11111
};

// Carita - partes (4 caracteres que forman una cara grande)
const uint8_t face_LT[8] = { // superior izquierda
    0b10111,
    0b10111,
    0b01000,
    0b01000,
    0b10000,
    0b10001,
    0b10001,
    0b10000
};

const uint8_t face_RT[8] = { // superior derecha
    0b10101,
    0b11101,
    0b00010,
    0b00010,
    0b00001,
    0b10001,
    0b10001,
    0b00001
};

const uint8_t face_LB[8] = { // inferior izquierda
    0b10000,
    0b10100,
    0b10011,
    0b10000,
    0b11100,
    0b11111,
    0b01111,
    0b00111
};

const uint8_t face_RB[8] = { // inferior derecha
    0b00001,
    0b00101,
    0b11001,
    0b00001,
    0b01111,
    0b11111,
    0b11110,
    0b11100
};

// ------------------------------------------------------------------
// Función para texto desplazable (scrolling) en una línea
// ------------------------------------------------------------------
void scrollText(LCD& lcd, uint8_t row, const std::string& text, int delay_ms = 200) {
    int len = text.length();
    if (len <= 20) {
        lcd.setCursor(row, 0);
        lcd.print(text);
        return;
    }
    for (int i = 0; i <= len - 20; ++i) {
        lcd.setCursor(row, 0);
        lcd.print(text.substr(i, 20));
        usleep(delay_ms * 1000);
    }
}

// ------------------------------------------------------------------
// main
// ------------------------------------------------------------------
int main() {
    if (!bcm2835_init()) {
        std::cerr << "Error: bcm2835 no se pudo inicializar. Ejecutar con sudo." << std::endl;
        return 1;
    }

    auto lcd = std::make_unique<LCD>();
    lcd->init();
    lcd->clear();

    // Cargar caracteres personalizados en CGRAM (ubicaciones 0..7)
    lcd->createChar(0, battery_EMP);
    lcd->createChar(1, battery_HLF);
    lcd->createChar(2, battery_FULL);
    lcd->createChar(3, Locked);
    lcd->createChar(4, face_LT);
    lcd->createChar(5, face_RT);
    lcd->createChar(6, face_LB);
    lcd->createChar(7, face_RB);

    // Texto fijo en línea 0 (título)
    lcd->setCursor(0, 0);
    lcd->print("Battery Charging");

    // Mostrar la carita de 4 caracteres en las líneas 2 y 3, columnas 4-5
    // (ajustar posición según preferencia)
    lcd->setCursor(2, 4);
    lcd->writeString("\x04"); // face_LT
    lcd->setCursor(2, 5);
    lcd->writeString("\x05"); // face_RT
    lcd->setCursor(3, 4);
    lcd->writeString("\x06"); // face_LB
    lcd->setCursor(3, 5);
    lcd->writeString("\x07"); // face_RB

    // Texto desplazable en línea 1
    std::string scrolling_msg = " Hola! Este es un texto que se desplaza para demostrar el scrolling en LCD 20x4. ";
    // Texto fijo en línea 3, columna 19 para el candado (esquina inferior derecha)
    // pero cuidado: la carita ocupa columnas 4-5, dejamos el candado en columna 19
    lcd->setCursor(3, 19);
    lcd->writeString("\x03"); // candado cerrado

    int battery_level = 0; // 0=vacío, 1=medio, 2=lleno
    int step = 1;

    std::cout << "Mostrando demo con caracteres personalizados. Presiona Ctrl+C para salir." << std::endl;

    while (true) {
        // Actualizar animación de batería en esquina superior derecha (fila 0, columna 19)
        lcd->setCursor(0, 19);
        switch (battery_level) {
            case 0: lcd->writeString("\x00"); break;
            case 1: lcd->writeString("\x01"); break;
            case 2: lcd->writeString("\x02"); break;
        }
        battery_level += step;
        if (battery_level >= 2) step = -1;
        if (battery_level <= 0) step = 1;

        // Actualizar texto desplazable
        scrollText(*lcd, 1, scrolling_msg, 250);

        // Pequeña pausa entre ciclos de scroll
        usleep(500000);
    }

    bcm2835_close();
    return 0;
}
