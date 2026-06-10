#include <iostream>
#include <memory>
#include <unistd.h>
#include <string>
#include <bcm2835.h>
#include "LCD.hpp"

// ------------------------------------------------------------------
// Definición de 5 caracteres de barra de progreso (de 1 a 5 píxeles de ancho)
// Cada carácter tiene 8 filas, y los píxeles se encienden de izquierda a derecha.
// Para un carácter de ancho W (1..5), se llenan las columnas 0..W-1.
// Los píxeles se representan con bits (bit0 = columna más a la izquierda).
// ------------------------------------------------------------------

// Carácter con 1 píxel de ancho (columna 0 llena)
const uint8_t prog_1[8] = {
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b10000,
    0b10000
};

// Carácter con 2 píxeles de ancho (columnas 0 y 1 llenas)
const uint8_t prog_2[8] = {
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000
};

// Carácter con 3 píxeles de ancho (columnas 0,1,2 llenas)
const uint8_t prog_3[8] = {
    0b11100,
    0b11100,
    0b11100,
    0b11100,
    0b11100,
    0b11100,
    0b11100,
    0b11100
};

// Carácter con 4 píxeles de ancho (columnas 0..3 llenas)
const uint8_t prog_4[8] = {
    0b11110,
    0b11110,
    0b11110,
    0b11110,
    0b11110,
    0b11110,
    0b11110,
    0b11110
};

// Carácter completamente lleno (5 píxeles de ancho)
const uint8_t prog_5[8] = {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
};

// También necesitamos un carácter vacío (0 píxeles) - usaremos espacio normal (' ')
// pero podemos definir uno explícitamente por si acaso (aunque no hace falta).

// ------------------------------------------------------------------
// main: dibuja una barra de progreso de 20 caracteres en la fila 0
// Cada carácter tiene 5 niveles (1..5), más el nivel 0 que es espacio.
// La barra completa tiene 20*5 = 100 pasos.
// Avanza un paso cada 3000 ms, y vuelve a empezar al llegar a 100.
// ------------------------------------------------------------------
int main() {
    if (!bcm2835_init()) {
        std::cerr << "Error: bcm2835 no se pudo inicializar. Ejecutar con sudo." << std::endl;
        return 1;
    }

    auto lcd = std::make_unique<LCD>();
    lcd->init();
    lcd->clear();

    // Cargar los 5 caracteres de progreso en las posiciones 0..4
    lcd->createChar(0, prog_1);   // 1 pixel ancho
    lcd->createChar(1, prog_2);   // 2 pixeles
    lcd->createChar(2, prog_3);   // 3 pixeles
    lcd->createChar(3, prog_4);   // 4 pixeles
    lcd->createChar(4, prog_5);   // 5 pixeles (completo)

    // También podemos definir el carácter vacío como espacio (no hace falta createChar)

    const int TOTAL_STEPS = 20 * 5;   // 100 pasos
    int step = 0;                      // paso actual (0 a 99)

    std::cout << "Barra de progreso de 20 caracteres, actualización cada 3000 ms. Presiona Ctrl+C para salir." << std::endl;

    while (true) {
        // Construir la línea de 20 caracteres según el paso actual
        std::string bar;
        int remaining = step;
        for (int col = 0; col < 20; ++col) {
            if (remaining >= 5) {
                // Este carácter está completamente lleno (5 píxeles)
                bar += static_cast<char>(0x04);  // prog_5
                remaining -= 5;
            } else if (remaining > 0) {
                // Carácter parcialmente lleno: remaining = 1..4
                bar += static_cast<char>(remaining - 1);  // prog_1 (0), prog_2 (1), prog_3 (2), prog_4 (3)
                remaining = 0;
            } else {
                // Carácter vacío
                bar += ' ';
            }
        }

        // Escribir la barra en la primera fila (fila 0)
        lcd->setCursor(0, 0);
        lcd->print(bar);

        // Avanzar al siguiente paso, reiniciar si llega al final
        step++;
        if (step >= TOTAL_STEPS) {
            step = 0;
        }

        // Esperar 3000 ms
        usleep(30 * 1000);
    }

    bcm2835_close();
    return 0;
}
