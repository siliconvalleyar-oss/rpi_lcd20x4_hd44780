#include "HD44780.hpp"
#include <bcm2835.h>
#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <memory>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>

static volatile bool running = true;
static std::unique_ptr<HD44780> lcd;

void signalHandler(int)
{
    running = false;
}

static constexpr HD44780::Pins DEFAULT_PINS = {13, 6, 26, 19, 20, 21};

static constexpr uint8_t BAT_EMPTY[8] = {
    0b01110, 0b11011, 0b10001, 0b10001,
    0b10001, 0b10001, 0b10001, 0b11111
};
static constexpr uint8_t BAT_HALF[8] = {
    0b01110, 0b11011, 0b10001, 0b10001,
    0b11111, 0b11111, 0b10001, 0b11111
};
static constexpr uint8_t BAT_FULL[8] = {
    0b01110, 0b11011, 0b11111, 0b11111,
    0b11111, 0b11111, 0b11111, 0b11111
};
static constexpr uint8_t LOCKED[8] = {
    0b01110, 0b10001, 0b10001, 0b11111,
    0b11011, 0b11011, 0b11111, 0b11111
};
static constexpr uint8_t FACE_LT[8] = {
    0b00000, 0b00000, 0b01010, 0b01010,
    0b01010, 0b00000, 0b00000, 0b00000
};
static constexpr uint8_t FACE_RT[8] = {
    0b00000, 0b10000, 0b01010, 0b01010,
    0b01010, 0b00000, 0b00000, 0b00000
};
static constexpr uint8_t FACE_LB[8] = {
    0b00000, 0b01110, 0b10001, 0b10001,
    0b01110, 0b00000, 0b00000, 0b00000
};
static constexpr uint8_t FACE_RB[8] = {
    0b00000, 0b01110, 0b10001, 0b10001,
    0b01110, 0b00000, 0b00000, 0b00000
};
static constexpr uint8_t PROG_1[8] = {
    0b10000, 0b10000, 0b10000, 0b10000,
    0b10000, 0b10000, 0b10000, 0b10000
};
static constexpr uint8_t PROG_2[8] = {
    0b11000, 0b11000, 0b11000, 0b11000,
    0b11000, 0b11000, 0b11000, 0b11000
};
static constexpr uint8_t PROG_3[8] = {
    0b11100, 0b11100, 0b11100, 0b11100,
    0b11100, 0b11100, 0b11100, 0b11100
};
static constexpr uint8_t PROG_4[8] = {
    0b11110, 0b11110, 0b11110, 0b11110,
    0b11110, 0b11110, 0b11110, 0b11110
};
static constexpr uint8_t PROG_5[8] = {
    0b11111, 0b11111, 0b11111, 0b11111,
    0b11111, 0b11111, 0b11111, 0b11111
};

static void setArgentinaTZ()
{
    setenv("TZ", "America/Argentina/Buenos_Aires", 1);
    tzset();
}

static std::string getTimeStr()
{
    time_t t; struct tm* ti; char buf[16];
    time(&t); ti = localtime(&t);
    strftime(buf, sizeof(buf), "%H:%M:%S", ti);
    return buf;
}

static std::string getDateStr()
{
    time_t t; struct tm* ti; char buf[16];
    time(&t); ti = localtime(&t);
    strftime(buf, sizeof(buf), "%d/%m/%Y", ti);
    return buf;
}

static std::string getHostname()
{
    char buf[256];
    if (gethostname(buf, sizeof(buf)) == 0) return buf;
    return "unknown";
}

static std::string getIP()
{
    FILE* f = popen("hostname -I 2>/dev/null | awk '{print $1}'", "r");
    if (!f) return "N/A";
    char buf[64];
    if (fgets(buf, sizeof(buf), f)) {
        pclose(f);
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        return buf;
    }
    pclose(f);
    return "N/A";
}

static float getCPUTemp()
{
    FILE* f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) return -1.0f;
    int v;
    if (fscanf(f, "%d", &v) == 1) { fclose(f); return v / 1000.0f; }
    fclose(f);
    return -1.0f;
}

static float getCPUUsage()
{
    static unsigned long prevIdle = 0, prevTotal = 0;
    FILE* f = fopen("/proc/stat", "r");
    if (!f) return -1.0f;
    char buf[256];
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return -1.0f; }
    fclose(f);
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
    sscanf(buf, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    unsigned long total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long totalD = total - prevTotal;
    unsigned long idleD = idle - prevIdle;
    float usage = (totalD > 0) ? (1.0f - (float)idleD / totalD) * 100.0f : 0.0f;
    prevTotal = total; prevIdle = idle;
    return usage;
}

static std::string getMemInfo()
{
    FILE* f = fopen("/proc/meminfo", "r");
    if (!f) return "N/A";
    char buf[128];
    unsigned long total = 0, avail = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (sscanf(buf, "MemTotal: %lu kB", &total) == 1) continue;
        if (sscanf(buf, "MemAvailable: %lu kB", &avail) == 1) continue;
    }
    fclose(f);
    if (total == 0) return "N/A";
    char out[32];
    snprintf(out, sizeof(out), "%luM/%luM", avail / 1024, total / 1024);
    return out;
}

#ifdef BITCOIN_ENABLED
#include <curl/curl.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    size_t total = size * nmemb;
    output->append((char*)contents, total);
    return total;
}

static std::string fetchBitcoinPrice()
{
    CURL* curl = curl_easy_init();
    if (!curl) return "Error";
    std::string url = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true";
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "RPi-LCD/1.0");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return "Error";
    return response;
}

static void bitcoinMode()
{
    lcd->clear();
    const uint8_t btcSym[8] = {
        0b00000, 0b01110, 0b10001, 0b10001,
        0b11111, 0b11111, 0b11011, 0b11111
    };
    lcd->createChar(0, btcSym);
    lcd->setCursor(0, 0); lcd->print("BTC: ");
    lcd->setCursor(0, 4); lcd->writeChar(0);
    lcd->setCursor(0, 6); lcd->print("USD");
    lcd->setCursor(1, 0); lcd->print("24h: ");
    lcd->setCursor(2, 0); lcd->print("Date:");
    lcd->setCursor(3, 0); lcd->print("Time:");
    std::cout << "Bitcoin ticker running. Ctrl+C to exit." << std::endl;
    while (running) {
        std::string resp = fetchBitcoinPrice();
        if (resp != "Error") {
            try {
                json data = json::parse(resp);
                double price = data["bitcoin"]["usd"].get<double>();
                double change = data["bitcoin"]["usd_24h_change"].get<double>();
                char buf[16];
                snprintf(buf, sizeof(buf), "$%.0f", price);
                lcd->setCursor(0, 10); lcd->print("            ");
                lcd->setCursor(0, 10); lcd->print(buf);
                snprintf(buf, sizeof(buf), "%+.2f%%", change);
                lcd->setCursor(1, 6); lcd->print("            ");
                lcd->setCursor(1, 6); lcd->print(buf);
            } catch (...) {
                lcd->setCursor(0, 10); lcd->print("  Error   ");
            }
        } else {
            lcd->setCursor(0, 10); lcd->print("  Error   ");
        }
        lcd->setCursor(2, 6); lcd->print(getDateStr());
        lcd->setCursor(3, 6); lcd->print("        ");
        lcd->setCursor(3, 6); lcd->print(getTimeStr());
        for (int i = 0; i < 60 && running; i++) sleep(1);
    }
}
#else
static void bitcoinMode()
{
    lcd->clear();
    lcd->setCursor(0, 0); lcd->print("Bitcoin mode");
    lcd->setCursor(1, 0); lcd->print("not available");
    lcd->setCursor(2, 0); lcd->print("Recompile with:");
    lcd->setCursor(3, 0); lcd->print("make BITCOIN=1");
    sleep(4);
}
#endif

static void clockMode()
{
    lcd->clear();
    if (_rows >= 4) {
        lcd->setCursor(0, 0); lcd->print("Host: "); lcd->print(getHostname());
        lcd->setCursor(1, 0); lcd->print("IP: ");
        lcd->setCursor(2, 0); lcd->print("Date: ");
        lcd->setCursor(3, 0); lcd->print("Time: ");
    } else {
        lcd->setCursor(0, 0); lcd->print("Date: ");
        lcd->setCursor(1, 0); lcd->print("Time: ");
    }
    uint32_t lastIPRefresh = 0;
    std::string ip = getIP();
    std::cout << "Clock mode running. Ctrl+C to exit." << std::endl;
    while (running) {
        uint32_t now = bcm2835_read();
        if (now - lastIPRefresh > 30 || lastIPRefresh == 0) {
            ip = getIP();
            lastIPRefresh = now;
        }
        if (_rows >= 4) {
            lcd->setCursor(0, 6); lcd->print("                ");
            lcd->setCursor(0, 6); lcd->print(getHostname());
            lcd->setCursor(1, 4); lcd->print("                ");
            lcd->setCursor(1, 4); lcd->print(ip);
            lcd->setCursor(2, 6); lcd->print("          ");
            lcd->setCursor(2, 6); lcd->print(getDateStr());
            lcd->setCursor(3, 6); lcd->print("          ");
            lcd->setCursor(3, 6); lcd->print(getTimeStr());
        } else {
            lcd->setCursor(0, 6); lcd->print("          ");
            lcd->setCursor(0, 6); lcd->print(getDateStr());
            lcd->setCursor(1, 6); lcd->print("          ");
            lcd->setCursor(1, 6); lcd->print(getTimeStr());
        }
        sleep(1);
    }
}

static void infoMode()
{
    lcd->clear();
    std::cout << "System info mode. Ctrl+C to exit." << std::endl;
    while (running) {
        float cpu = getCPUUsage();
        float temp = getCPUTemp();
        std::string mem = getMemInfo();
        std::string ip = getIP();
        lcd->setCursor(0, 0);
        lcd->printf("CPU:%3.0f%% %s", cpu > 0 ? cpu : 0.0f, mem.c_str());
        char line2[32];
        if (temp > 0)
            snprintf(line2, sizeof(line2), "Temp:%.1fC IP:%s", temp, ip.c_str());
        else
            snprintf(line2, sizeof(line2), "IP:%s", ip.c_str());
        lcd->setCursor(1, 0); lcd->print(line2);
        lcd->setCursor(2, 0); lcd->printf("Date:%s", getDateStr().c_str());
        lcd->setCursor(3, 0); lcd->printf("Time:%s", getTimeStr().c_str());
        sleep(2);
    }
}

static void customCharsMode()
{
    lcd->clear();
    lcd->createChar(0, BAT_EMPTY);
    lcd->createChar(1, BAT_HALF);
    lcd->createChar(2, BAT_FULL);
    lcd->createChar(3, LOCKED);
    lcd->createChar(4, FACE_LT);
    lcd->createChar(5, FACE_RT);
    lcd->createChar(6, FACE_LB);
    lcd->createChar(7, FACE_RB);
    std::cout << "Custom chars demo running. Ctrl+C to exit." << std::endl;
    int batState = 0;
    const uint8_t batChars[3] = {0, 1, 2};
    while (running) {
        lcd->setCursor(0, 0);
        lcd->print("Battery: [");
        lcd->writeChar(batChars[batState]);
        lcd->print("]      ");
        lcd->setCursor(0, 15);
        lcd->print("OK");
        lcd->setCursor(1, 4); lcd->writeChar(4);
        lcd->setCursor(1, 5); lcd->writeChar(5);
        lcd->setCursor(2, 4); lcd->writeChar(6);
        lcd->setCursor(2, 5); lcd->writeChar(7);
        lcd->setCursor(3, 15); lcd->writeChar(3);
        lcd->setCursor(3, 0);
        lcd->printf("Locked %s", getTimeStr().c_str());
        batState = (batState + 1) % 3;
        sleep(1);
    }
}

static void progressBarMode()
{
    lcd->createChar(0, PROG_1);
    lcd->createChar(1, PROG_2);
    lcd->createChar(2, PROG_3);
    lcd->createChar(3, PROG_4);
    lcd->createChar(4, PROG_5);
    lcd->clear();
    std::cout << "Progress bar demo running. Ctrl+C to exit." << std::endl;
    while (running) {
        for (int step = 0; step <= 100 && running; step++) {
            int fullChars = (step * _cols) / 100;
            int remainder = ((step * _cols * 5) / 100) % 5;
            lcd->setCursor(1, 0);
            lcd->printf("Progress:%3d%%", step);
            lcd->setCursor(2, 0);
            for (int c = 0; c < (int)_cols && running; c++) {
                if (c < fullChars)
                    lcd->writeChar(4);
                else if (c == fullChars && remainder > 0)
                    lcd->writeChar(remainder - 1);
                else
                    lcd->write(' ');
            }
            usleep(50000);
        }
    }
}

static void allDemosMode()
{
    const char* modes[] = {"Clock", "System Info", "Custom Chars", "Progress Bar", "Bitcoin"};
    const int numModes = 5;
    for (int i = 0; i < numModes && running; i++) {
        lcd->clear();
        lcd->setCursor(0, 0); lcd->print("Mode: ");
        lcd->setCursor(0, 6); lcd->print(modes[i]);
        lcd->setCursor(2, 0); lcd->print("Starting in 3s...");
        sleep(3);
        if (!running) break;
        switch (i) {
            case 0: clockMode(); break;
            case 1: infoMode(); break;
            case 2: customCharsMode(); break;
            case 3: progressBarMode(); break;
            case 4: bitcoinMode(); break;
        }
    }
}

#ifdef MENU_ENABLED
static constexpr uint8_t BTN_UP = 17;
static constexpr uint8_t BTN_DOWN = 22;
static constexpr uint8_t BTN_OK = 27;

static void menuMode()
{
    bcm2835_gpio_fsel(BTN_UP, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(BTN_DOWN, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(BTN_OK, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(BTN_UP, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_set_pud(BTN_DOWN, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_set_pud(BTN_OK, BCM2835_GPIO_PUD_UP);
    const char* items[] = {"Clock", "System Info", "Custom Chars", "Progress", "Bitcoin", "All Demos", "Exit"};
    const int numItems = 7;
    int sel = 0;
    bool active = true;
    lcd->clear();
    std::cout << "Menu mode. Use buttons (UP/DOWN/OK). Ctrl+C to exit." << std::endl;
    while (active && running) {
        lcd->setCursor(0, 0); lcd->print(" Select mode:");
        for (int i = 0; i < 3 && (i + 1) < numItems; i++) {
            int idx = sel + i;
            if (idx >= numItems) break;
            lcd->setCursor(i + 1, 0);
            if (i == 0) lcd->print(">"); else lcd->print(" ");
            lcd->print(items[idx]);
            int remaining = _cols - 2 - strlen(items[idx]);
            for (int s = 0; s < remaining; s++) lcd->write(' ');
        }
        bool pressed = false;
        while (!pressed && running) {
            usleep(50000);
            if (bcm2835_gpio_lev(BTN_UP) == LOW) {
                sel = (sel > 0) ? sel - 1 : numItems - 1;
                pressed = true;
                while (bcm2835_gpio_lev(BTN_UP) == LOW) usleep(50000);
            }
            if (bcm2835_gpio_lev(BTN_DOWN) == LOW) {
                sel = (sel + 1) % numItems;
                pressed = true;
                while (bcm2835_gpio_lev(BTN_DOWN) == LOW) usleep(50000);
            }
            if (bcm2835_gpio_lev(BTN_OK) == LOW) {
                pressed = true;
                while (bcm2835_gpio_lev(BTN_OK) == LOW) usleep(50000);
                lcd->clear();
                switch (sel) {
                    case 0: clockMode(); break;
                    case 1: infoMode(); break;
                    case 2: customCharsMode(); break;
                    case 3: progressBarMode(); break;
                    case 4: bitcoinMode(); break;
                    case 5: allDemosMode(); break;
                    case 6: active = false; break;
                }
                lcd->clear();
            }
        }
    }
}
#endif

static void printUsage(const char* prog)
{
    std::cout << "Usage: " << prog << " [mode]" << std::endl;
    std::cout << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  --clock       Clock with date/time and system info (default)" << std::endl;
    std::cout << "  --info        System information (CPU, temp, memory, IP)" << std::endl;
    std::cout << "  --bitcoin     Bitcoin price ticker" << std::endl;
    std::cout << "  --chars       Custom characters demo" << std::endl;
    std::cout << "  --progress    Progress bar demo" << std::endl;
    std::cout << "  --all         Run all demos sequentially" << std::endl;
#ifdef MENU_ENABLED
    std::cout << "  --menu        Interactive menu with buttons" << std::endl;
#endif
    std::cout << "  --help        Show this help" << std::endl;
}

int main(int argc, char* argv[])
{
    std::string mode = "clock";
    if (argc > 1) {
        mode = argv[1];
        if (mode == "--help" || mode == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (mode.rfind("--", 0) != 0) {
            std::cerr << "Unknown option: " << mode << std::endl;
            printUsage(argv[0]);
            return 1;
        }
        mode = mode.substr(2);
    }

    if (!bcm2835_init()) {
        std::cerr << "Error: bcm2835 init failed. Run with sudo." << std::endl;
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    setArgentinaTZ();

    lcd = std::make_unique<HD44780>(20, 4, DEFAULT_PINS);
    lcd->begin();
    lcd->clear();

    if (mode == "clock") clockMode();
    else if (mode == "info") infoMode();
    else if (mode == "bitcoin") bitcoinMode();
    else if (mode == "chars") customCharsMode();
    else if (mode == "progress") progressBarMode();
    else if (mode == "all") allDemosMode();
#ifdef MENU_ENABLED
    else if (mode == "menu") menuMode();
#endif
    else {
        std::cerr << "Unknown mode: " << mode << std::endl;
        printUsage(argv[0]);
    }

    lcd->clear();
    lcd->setCursor(0, 0); lcd->print("Goodbye!");
    bcm2835_close();
    std::cout << "Exited cleanly." << std::endl;
    return 0;
}
