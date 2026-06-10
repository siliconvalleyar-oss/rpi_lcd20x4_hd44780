#include <iostream>
#include <memory>
#include <ctime>
#include <unistd.h>
#include <bcm2835.h>
#include "LCD.hpp"

// Configurar zona horaria Argentina (GMT-3)
void setArgentinaTimeZone() {
    setenv("TZ", "America/Argentina/Buenos_Aires", 1);
    tzset();
}

int main() {
    if (!bcm2835_init()) {
        std::cerr << "Error: bcm2835 no se pudo inicializar. Ejecutar con sudo." << std::endl;
        return 1;
    }

    setArgentinaTimeZone();

    auto lcd = std::make_unique<LCD>();
    lcd->init();
    lcd->clear();

    // Mostrar solo fecha y hora, sin textos adicionales
    lcd->setCursor(0, 0);
    lcd->print("Fecha:            ");
    lcd->setCursor(1, 0);
    lcd->print("Hora:             ");

    time_t rawtime;
    struct tm *timeinfo;
    char fecha_str[20];
    char hora_str[20];

    std::cout << "Mostrando fecha y hora Argentina. Ctrl+C para salir." << std::endl;

    while (true) {
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(fecha_str, sizeof(fecha_str), "%d/%m/%Y", timeinfo);
        strftime(hora_str, sizeof(hora_str), "%H:%M:%S", timeinfo);

        lcd->setCursor(0, 7);
        lcd->print(fecha_str);
        lcd->setCursor(1, 7);
        lcd->print(hora_str);

        sleep(1);
    }

    bcm2835_close();
    return 0;
}
