#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bcm2835.h>
#include "LCD.hpp"

// -------------------------------------------------------------
// Utilidades generales
// -------------------------------------------------------------
void setArgentinaTimeZone() {
    setenv("TZ", "America/Argentina/Buenos_Aires", 1);
    tzset();
}

std::string getHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) return std::string(hostname);
    return "desconocido";
}

std::string getIPAddress() {
    struct ifaddrs *ifaddr, *ifa;
    std::string ip = "No IP";
    if (getifaddrs(&ifaddr) == -1) return "error";
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        std::string name = ifa->ifa_name;
        if (name == "wlan0" || name == "eth0") {
            char ipstr[INET_ADDRSTRLEN];
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ipstr, sizeof(ipstr));
            ip = std::string(ipstr);
            break;
        }
    }
    freeifaddrs(ifaddr);
    return ip;
}

std::string getCurrentDateTime(const std::string& format) {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[64];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), format.c_str(), timeinfo);
    return std::string(buffer);
}

// -------------------------------------------------------------
// Módulo 1: Reloj + IP + Hostname
// -------------------------------------------------------------
void run_clock(LCD& lcd) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Host: " + getHostname().substr(0,14));
    lcd.setCursor(1,0); lcd.print("IP: " + getIPAddress());
    lcd.setCursor(2,0); lcd.print("Fecha: ");
    lcd.setCursor(3,0); lcd.print("Hora: ");
    while (true) {
        lcd.setCursor(2,7); lcd.print(getCurrentDateTime("%d/%m/%Y"));
        lcd.setCursor(3,6); lcd.print(getCurrentDateTime("%H:%M:%S"));
        sleep(1);
    }
}

// -------------------------------------------------------------
// Módulo 2: Monitor de sistema (CPU, RAM, Temp, Disco)
// -------------------------------------------------------------
float getCPUTemp() {
    std::ifstream tempFile("/sys/class/thermal/thermal_zone0/temp");
    if (!tempFile) return 0;
    int temp;
    tempFile >> temp;
    return temp / 1000.0;
}

float getCPUusage() {
    static unsigned long long lastTotal = 0, lastIdle = 0;
    std::ifstream stat("/proc/stat");
    std::string line;
    std::getline(stat, line);
    std::istringstream ss(line);
    std::string cpu;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long long idleAll = idle + iowait;
    if (lastTotal == 0) { lastTotal = total; lastIdle = idleAll; return 0; }
    float diffTotal = total - lastTotal;
    float diffIdle = idleAll - lastIdle;
    lastTotal = total; lastIdle = idleAll;
    if (diffTotal == 0) return 0;
    return (diffTotal - diffIdle) / diffTotal * 100.0;
}

unsigned long getRAMtotal() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            unsigned long kb;
            sscanf(line.c_str(), "MemTotal: %lu kB", &kb);
            return kb / 1024;
        }
    }
    return 0;
}

unsigned long getRAMfree() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.find("MemAvailable:") == 0) {
            unsigned long kb;
            sscanf(line.c_str(), "MemAvailable: %lu kB", &kb);
            return kb / 1024;
        }
    }
    return 0;
}

unsigned long getDiskFree(const std::string& path = "/") {
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) != 0) return 0;
    return (stat.f_bfree * stat.f_frsize) / (1024*1024);
}

void run_system(LCD& lcd) {
    lcd.clear();
    while (true) {
        float temp = getCPUTemp();
        float cpu = getCPUusage();
        unsigned long ramTotal = getRAMtotal();
        unsigned long ramFree = getRAMfree();
        unsigned long diskFree = getDiskFree();
        lcd.setCursor(0,0);
        char line0[21];
        snprintf(line0, sizeof(line0), "CPU:%3.0f%% Temp:%2.0fC", cpu, temp);
        lcd.print(line0);
        lcd.setCursor(1,0);
        char line1[21];
        snprintf(line1, sizeof(line1), "RAM:%3lu/%3lu MB", ramFree, ramTotal);
        lcd.print(line1);
        lcd.setCursor(2,0);
        char line2[21];
        snprintf(line2, sizeof(line2), "Disco libre: %3lu MB", diskFree);
        lcd.print(line2);
        lcd.setCursor(3,0);
        lcd.print("IP: " + getIPAddress());
        sleep(2);
    }
}

// -------------------------------------------------------------
// Módulo 3: Clima (requiere API key, simula si no hay conexión)
// -------------------------------------------------------------
#ifdef WEATHER_ENABLED
#include <curl/curl.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total = size * nmemb;
    output->append((char*)contents, total);
    return total;
}

std::string getWeather(const std::string& city, const std::string& apiKey) {
    CURL* curl = curl_easy_init();
    if (!curl) return "Error curl";
    std::string url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&units=metric&appid=" + apiKey;
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return "Sin conexion";
    try {
        json data = json::parse(response);
        float temp = data["main"]["temp"];
        int hum = data["main"]["humidity"];
        std::string desc = data["weather"][0]["description"];
        char buf[64];
        snprintf(buf, sizeof(buf), "%s %.1fC %d%%", desc.c_str(), temp, hum);
        return std::string(buf);
    } catch (...) {
        return "Error parseo";
    }
}
#endif

void run_weather(LCD& lcd) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Clima: Cargando...");
#ifdef WEATHER_ENABLED
    std::string apiKey = "TU_API_KEY_OPENWEATHER";  // Reemplazar
    std::string city = "Buenos Aires";
    while (true) {
        std::string weather = getWeather(city, apiKey);
        lcd.setCursor(0,0); lcd.print("                ");
        lcd.setCursor(0,0); lcd.print(city.substr(0,16));
        lcd.setCursor(1,0); lcd.print(weather);
        lcd.setCursor(2,0); lcd.print("Actualizado");
        lcd.setCursor(3,0); lcd.print(getCurrentDateTime("%H:%M:%S"));
        sleep(600); // cada 10 minutos
    }
#else
    lcd.setCursor(0,0); lcd.print("Clima no disponible");
    lcd.setCursor(1,0); lcd.print("Compila con: make WEATHER=1");
    sleep(5);
#endif
}

// -------------------------------------------------------------
// Módulo 4: MPD (Music Player Daemon)
// -------------------------------------------------------------
std::string getMPDStatus() {
    FILE* fp = popen("mpc current", "r");
    if (!fp) return "MPD no instalado";
    char buf[256];
    if (fgets(buf, sizeof(buf), fp) != nullptr) {
        std::string song(buf);
        song.erase(song.find_last_not_of("\n")+1);
        pclose(fp);
        if (!song.empty()) return song;
        return "Detenido";
    }
    pclose(fp);
    return "Error";
}

void run_mpd(LCD& lcd) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("MPD Player");
    while (true) {
        std::string song = getMPDStatus();
        lcd.setCursor(1,0); lcd.print("                    ");
        lcd.setCursor(1,0); lcd.print(song.substr(0,20));
        lcd.setCursor(2,0); lcd.print("Presiona Ctrl+C");
        lcd.setCursor(3,0); lcd.print("para salir");
        sleep(1);
    }
}

// -------------------------------------------------------------
// Módulo 5: Cola FIFO (mensajes desde otros procesos)
// -------------------------------------------------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

void run_queue(LCD& lcd) {
    const char* fifo_path = "/tmp/lcd_queue";
    unlink(fifo_path);
    mkfifo(fifo_path, 0666);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Esperando msgs...");
    int fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) { lcd.setCursor(1,0); lcd.print("Error FIFO"); return; }
    struct pollfd pfd = {fd, POLLIN, 0};
    char buffer[128];
    while (true) {
        int ret = poll(&pfd, 1, 1000);
        if (ret > 0 && (pfd.revents & POLLIN)) {
            ssize_t n = read(fd, buffer, sizeof(buffer)-1);
            if (n > 0) {
                buffer[n] = '\0';
                lcd.clear();
                lcd.setCursor(0,0); lcd.print("Mensaje:");
                for (int i=0; i<3; i++) {
                    int line = i+1;
                    lcd.setCursor(line,0);
                    std::string msg(buffer);
                    size_t start = i*20;
                    if (start < msg.length())
                        lcd.print(msg.substr(start,20));
                    else
                        lcd.print("                    ");
                }
            }
        }
        // Mostrar reloj pequeño en esquina
        lcd.setCursor(3,15); lcd.print(getCurrentDateTime("%H:%M"));
        usleep(500000);
    }
    close(fd);
}

// -------------------------------------------------------------
// Módulo 6: Sensor de puerta (reed switch) con contador
// -------------------------------------------------------------
void run_door(LCD& lcd) {
    const int PIN_SENSOR = 22; // GPIO 22 (Pin 15) - configurar como entrada con pull-up
    bcm2835_gpio_fsel(PIN_SENSOR, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(PIN_SENSOR, BCM2835_GPIO_PUD_UP);
    int count = 0;
    int last_state = HIGH;
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Puerta: Cerrada");
    lcd.setCursor(1,0); lcd.print("Aperturas hoy: 0");
    lcd.setCursor(2,0); lcd.print("Ultima: --:--:--");
    while (true) {
        int state = bcm2835_gpio_lev(PIN_SENSOR);
        if (state == LOW && last_state == HIGH) { // abrió
            count++;
            std::string hora = getCurrentDateTime("%H:%M:%S");
            lcd.setCursor(1,14); lcd.print(std::to_string(count));
            lcd.setCursor(2,8);  lcd.print(hora);
            lcd.setCursor(0,8);  lcd.print("Abierta ");
            // opcional: esperar a que cierre
        } else if (state == HIGH && last_state == LOW) {
            lcd.setCursor(0,8); lcd.print("Cerrada");
        }
        last_state = state;
        sleep(0.1);
    }
}

// -------------------------------------------------------------
// Módulo 7: Menú interactivo con botones
// -------------------------------------------------------------
// Botones en GPIO 5, 6, 13 (por ejemplo) - adaptar
void run_menu(LCD& lcd) {
    const int BTN_UP = 5;    // GPIO 5
    const int BTN_DOWN = 6;  // GPIO 6
    const int BTN_OK = 13;   // GPIO 13
    bcm2835_gpio_fsel(BTN_UP, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(BTN_DOWN, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(BTN_OK, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(BTN_UP, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_set_pud(BTN_DOWN, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_set_pud(BTN_OK, BCM2835_GPIO_PUD_UP);
    const char* items[] = {"Reloj", "Sistema", "Clima", "MPD", "Cola", "Puerta"};
    int selected = 0;
    int n_items = 6;
    while (true) {
        lcd.clear();
        for (int i=0; i<4 && i+selected < n_items; i++) {
            lcd.setCursor(i,0);
            if (i == 0) lcd.print(">");
            else lcd.print(" ");
            lcd.print(items[selected + i]);
        }
        if (bcm2835_gpio_lev(BTN_DOWN) == LOW) { selected = (selected+1) % n_items; delay(200); }
        if (bcm2835_gpio_lev(BTN_UP) == LOW) { selected = (selected-1+n_items) % n_items; delay(200); }
        if (bcm2835_gpio_lev(BTN_OK) == LOW) {
            // Ejecutar submódulo
            lcd.clear(); lcd.setCursor(0,0); lcd.print("Ejecutando...");
            delay(500);
            if (selected == 0) run_clock(lcd);
            else if (selected == 1) run_system(lcd);
            else if (selected == 2) run_weather(lcd);
            else if (selected == 3) run_mpd(lcd);
            else if (selected == 4) run_queue(lcd);
            else if (selected == 5) run_door(lcd);
        }
        delay(50);
    }
}

// -------------------------------------------------------------
// Main: selección de módulo por argumento
// -------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (!bcm2835_init()) {
        std::cerr << "Error: bcm2835 init. Ejecutar con sudo." << std::endl;
        return 1;
    }
    setArgentinaTimeZone();
    auto lcd = std::make_unique<LCD>();
    lcd->init();
    lcd->clear();

    if (argc < 2) {
        std::cout << "Uso: " << argv[0] << " [modulo]\n"
                  << "Modulos: --clock, --system, --weather, --mpd, --queue, --door, --menu\n";
        return 0;
    }
    std::string mod = argv[1];
    if (mod == "--clock") run_clock(*lcd);
    else if (mod == "--system") run_system(*lcd);
    else if (mod == "--weather") run_weather(*lcd);
    else if (mod == "--mpd") run_mpd(*lcd);
    else if (mod == "--queue") run_queue(*lcd);
    else if (mod == "--door") run_door(*lcd);
    else if (mod == "--menu") run_menu(*lcd);
    else std::cout << "Modulo desconocido\n";

    bcm2835_close();
    return 0;
}
