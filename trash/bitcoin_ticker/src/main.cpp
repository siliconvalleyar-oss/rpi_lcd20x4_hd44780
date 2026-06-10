#include <iostream>
#include <memory>
#include <unistd.h>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <bcm2835.h>
#include "LCD.hpp"
#include "BitcoinClient.hpp"

// Configurar zona horaria Argentina (GMT-3)
void setArgentinaTimeZone() {
    setenv("TZ", "America/Argentina/Buenos_Aires", 1);
    tzset();
}

// Formatear hora actual
std::string getCurrentTime() {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[9];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return std::string(buffer);
}

// Formatear fecha actual
std::string getCurrentDate() {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[11];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%d/%m/%Y", timeinfo);
    return std::string(buffer);
}

// Obtener hostname del sistema
std::string getHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "desconocido";
}

// Caracteres personalizados para el ticker
const uint8_t bitcoin_symbol[8] = {
    0b00000,
    0b01110,
    0b10001,
    0b10001,
    0b11111,
    0b11111,
    0b11011,
    0b11111
};

int main() {
    if (!bcm2835_init()) {
        std::cerr << "Error: bcm2835 no se pudo inicializar. Ejecutar con sudo." << std::endl;
        return 1;
    }

    setArgentinaTimeZone();

    auto lcd = std::make_unique<LCD>();
    auto btcClient = std::make_unique<BitcoinClient>();

    lcd->init();
    lcd->clear();

    // Crear carácter personalizado del Bitcoin
    lcd->createChar(0, bitcoin_symbol);

    // Mostrar encabezados fijos
    lcd->setCursor(0, 0);
    lcd->print("BTC: ");
    lcd->setCursor(0, 4);
    lcd->writeString("\x00");  // Símbolo de Bitcoin

    lcd->setCursor(0, 6);
    lcd->print("USD");

    lcd->setCursor(1, 0);
    lcd->print("24h: ");

    lcd->setCursor(2, 0);
    lcd->print("Fecha: ");

    lcd->setCursor(3, 0);
    lcd->print("Hora: ");

    lcd->setCursor(3, 15);
    lcd->print("BTC");

    std::cout << "Bitcoin Ticker en LCD. Actualización cada 60 segundos. Ctrl+C para salir." << std::endl;

    while (true) {
        // Obtener precio de Bitcoin
        double price = btcClient->getCurrentPrice();
        double change = btcClient->getPriceChangePercentage();

        // Formatear precio
        std::string priceStr;
        std::string changeStr;
        if (price > 0.0) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2);
            ss << "$" << price;
            priceStr = ss.str();
            
            std::stringstream ss2;
            ss2 << std::fixed << std::setprecision(2);
            ss2 << (change >= 0.0 ? "+" : "") << change << "%";
            changeStr = ss2.str();
        } else {
            priceStr = "  Error   ";
            changeStr = "  Error  ";
        }

        // Mostrar precio en LCD
        lcd->setCursor(0, 10);
        lcd->print("            ");  // Limpiar
        lcd->setCursor(0, 10);
        lcd->print(priceStr);

        // Mostrar cambio 24h (con color verde/rojo según corresponda)
        lcd->setCursor(1, 6);
        lcd->print("            ");
        lcd->setCursor(1, 6);
        lcd->print(changeStr);

        // Mostrar fecha y hora
        lcd->setCursor(2, 7);
        lcd->print(getCurrentDate());

        lcd->setCursor(3, 6);
        lcd->print(getCurrentTime());

        // Esperar 60 segundos antes de la próxima actualización
        sleep(60);
    }

    bcm2835_close();
    return 0;
}
