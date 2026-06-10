# HD44780 LCD 20x4 for Raspberry Pi

Unified HD44780 LCD library and multi-mode application for Raspberry Pi (20x4 and 16x2 character displays). Controls a Hitachi HD44780-compatible LCD in 4-bit mode via GPIO using `libbcm2835`.

## Features

- **HD44780 library** (`include/HD44780.hpp` + `src/HD44780.cpp`) — reusable C++ class with full HD44780 support
- **Multi-mode application** (`src/main.cpp`) — selectable modes via command-line arguments:
  - `--clock` — Clock with hostname, IP, date/time (Argentina TZ)
  - `--info` — System info: CPU%, temperature, memory, IP
  - `--bitcoin` — Bitcoin price ticker via CoinGecko API (optional, requires libcurl)
  - `--chars` — Custom characters demo (battery animation, big face, lock icon)
  - `--progress` — Smooth progress bar (100 steps, 5 custom chars)
  - `--all` — Run all modes sequentially
  - `--menu` — Interactive menu with hardware buttons (optional)
- **Custom characters** — CGRAM support for up to 8 user-defined 5x8 glyphs
- **20x4 and 16x2 support** — auto-detected via constructor parameters

## Pinout

| Function | BCM GPIO | Physical Pin |
|----------|----------|--------------|
| RS       | 13       | 33           |
| EN       | 6        | 31           |
| D4       | 26       | 37           |
| D5       | 19       | 35           |
| D6       | 20       | 38           |
| D7       | 21       | 40           |

### Menu mode buttons (optional, GPIO 17/22/27)

| Button | BCM GPIO | Physical Pin |
|--------|----------|--------------|
| UP     | 17       | 11           |
| DOWN   | 22       | 15           |
| OK     | 27       | 13           |

Buttons connect each GPIO to GND (internal pull-up enabled).

## Dependencies

- `libbcm2835` — GPIO access library (not in distro repos, install from source)
- Optional: `libcurl` + `nlohmann-json` for Bitcoin ticker mode

### Install

```bash
bash script_tools/install_deps.sh
```

Or manually:

```bash
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.75.tar.gz
tar xzf bcm2835-1.75.tar.gz
cd bcm2835-1.75
./configure && make && sudo make install
```

## Build & Run

### Basic build

```bash
make                  # builds bin/lcd_app
sudo ./bin/lcd_app    # run default mode (clock)
```

### Run specific modes

```bash
make run              # clock (default)
make run-info         # system info
make run-chars        # custom characters demo
make run-progress     # progress bar demo
sudo ./bin/lcd_app --all   # all demos sequentially
```

### Bitcoin mode (requires extra deps)

```bash
make BITCOIN=1
sudo ./bin/lcd_app --bitcoin
```

### Menu mode (requires hardware buttons on GPIO 17/22/27)

```bash
make MENU=1
sudo ./bin/lcd_app --menu
```

### Remote build via SSH

```bash
make remote-build              # ssh pi@raspberry.local "cd <dir> && make -j4"
make remote-build PI_HOST=pi@192.168.1.100
make remote-run                # build + run on Pi
```

## Project Structure

```
lcd20x4/
├── include/
│   └── HD44780.hpp          # HD44780 LCD driver header
├── src/
│   ├── HD44780.cpp          # HD44780 LCD driver implementation
│   └── main.cpp             # Multi-mode application entry point
├── trash/                   # Original project files (preserved for reference)
│   ├── bitcoin_ticker/
│   ├── HD44780_LCD_RPI-1.3.3/
│   ├── lcd_20x4/
│   ├── lcd_all_in_one/
│   ├── lcd_clock_project/
│   ├── lcd_custom_chars/
│   ├── lcd_full_info/
│   └── lcd_progressbar/
├── script_tools/
│   └── install_deps.sh      # Dependency installer
├── .opencode/
│   └── skills/
│       └── lcd20x4.jsonc    # opencode skill
├── Makefile
├── .gitignore
└── README.md
```

## HD44780 Library API

```cpp
#include "HD44780.hpp"

// Create LCD instance (20 columns, 4 rows, default pins)
HD44780::Pins pins = {13, 6, 26, 19, 20, 21};
HD44780 lcd(20, 4, pins);

// Initialize
lcd.begin();

// Basic output
lcd.setCursor(col, row);
lcd.print("Hello World");
lcd.printf("Value: %d", 42);

// Display control
lcd.clear();
lcd.display(true);    // on/off
lcd.cursor(true);     // show cursor
lcd.blink(true);      // blinking cursor

// Custom characters
const uint8_t heart[8] = {0b00000, 0b01010, 0b11111, ...};
lcd.createChar(0, heart);
lcd.writeChar(0);

// Scrolling
lcd.scrollLeft(3);
lcd.scrollRight(1);

// Line operations
lcd.clearLine(0);
```

## Origin

This project consolidates 8 separate LCD projects into a single well-organized codebase:

| Original project | Mode in unified app |
|---|---|
| `lcd_20x4` | Clock mode |
| `lcd_clock_project` | Clock mode |
| `lcd_full_info` | Clock mode (with system info) |
| `lcd_all_in_one` | All modes + menu |
| `bitcoin_ticker` | Bitcoin mode |
| `lcd_custom_chars` | Custom chars mode |
| `lcd_progressbar` | Progress bar mode |
| `HD44780_LCD_RPI-1.3.3` | Reference I2C library (Gavin Lyons) |

All original sources are preserved in `trash/` for reference.

## Troubleshooting

- **"bcm2835.h not found"** — Run `bash script_tools/install_deps.sh`
- **No display output** — Check wiring (RS=13, EN=6, D4=26, D5=19, D6=20, D7=21)
- **Garbage on screen** — Verify 5V logic (Pi GPIOs are 3.3V, HD44780 typically needs 5V; use level shifter or check your LCD module's tolerance)
- **Permission denied** — Run with `sudo`
- **I2C version** — See `trash/HD44780_LCD_RPI-1.3.3/` for Gavin Lyons' I2C-based library
