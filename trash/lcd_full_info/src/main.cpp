#include <iostream>
#include <memory>
#include <ctime>
#include <unistd.h>
#include <bcm2835.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <iomanip>
#include <sstream>

#include "LCD.hpp"

// Configurar zona horaria Argentina (GMT-3)
void setArgentinaTimeZone() {
    setenv("TZ", "America/Argentina/Buenos_Aires", 1);
    tzset();
}

// Obtener hostname del sistema
std::string getHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "desconocido";
}

// Obtener IP de la interfaz activa (prioriza wlan0, luego eth0)
std::string getIPAddress() {
    struct ifaddrs *ifaddr, *ifa;
    std::string ip = "No IP";

    if (getifaddrs(&ifaddr) == -1) {
        return "error";
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue; // solo IPv4

        std::string name = ifa->ifa_name;
        if (name == "wlan0" || name == "eth0") {
            char ipstr[INET_ADDRSTRLEN];
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ipstr, sizeof(ipstr));
            ip = std::string(ipstr);
            break; // tomar la primera encontrada
        }
    }
    freeifaddrs(ifaddr);
    return ip;
}

// Formatear la hora actual
std::string getCurrentTime() {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[9]; // HH:MM:SS + null
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return std::string(buffer);
}

// Formatear la fecha actual
std::string getCurrentDate() {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[11]; // DD/MM/AAAA + null
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%d/%m/%Y", timeinfo);
    return std::string(buffer);
}

// Truncar texto a un máximo de caracteres
std::string truncate(const std::string& text, size_t max_len) {
    if (text.length() <= max_len) return text;
    return text.substr(0, max_len - 3) + "...";
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

    // Obtener datos estáticos o semi-estáticos
    std::string hostname = getHostname();
    std::string ip = getIPAddress();

    // Mostrar información en pantalla (20 columnas)
    // Línea 0: Hostname
    lcd->setCursor(0, 0);
    lcd->print("Host: " + truncate(hostname, 14));

    // Línea 1: IP
    lcd->setCursor(1, 0);
    lcd->print("IP: " + truncate(ip, 16));

    // Línea 2: Fecha
    lcd->setCursor(2, 0);
    lcd->print("Fecha: ");

    // Línea 3: Hora
    lcd->setCursor(3, 0);
    lcd->print("Hora: ");

    std::cout << "Mostrando hostname, IP, fecha y hora Argentina. Ctrl+C para salir." << std::endl;

    time_t last_ip_check = time(nullptr);

    while (true) {
        // Actualizar fecha y hora cada segundo
        std::string fecha = getCurrentDate();
        std::string hora = getCurrentTime();

        lcd->setCursor(2, 7);
        lcd->print(fecha);
        lcd->setCursor(3, 6);
        lcd->print(hora);

        // Actualizar IP cada 30 segundos (por si cambia)
        time_t now = time(nullptr);
        if (now - last_ip_check >= 30) {
            ip = getIPAddress();
            lcd->setCursor(1, 4);
            lcd->print(truncate(ip, 16) + "                "); // limpiar resto
            last_ip_check = now;
        }

        sleep(1);
    }

    bcm2835_close();
    return 0;
}
