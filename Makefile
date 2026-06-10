CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -Iinclude
LDFLAGS = -lbcm2835

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SRCS = $(SRC_DIR)/HD44780.cpp $(SRC_DIR)/main.cpp
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
TARGET = $(BIN_DIR)/lcd_app

PI_HOST ?= pi@raspberry.local
PI_DIR ?= /home/pi/src/lcd20x4

BITCOIN ?= 0
MENU ?= 0

ifeq ($(BITCOIN),1)
    CXXFLAGS += -DBITCOIN_ENABLED
    LDFLAGS += -lcurl
endif

ifeq ($(MENU),1)
    CXXFLAGS += -DMENU_ENABLED
endif

$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

.PHONY: all clean run help install-deps remote-build remote-run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

run: $(TARGET)
	sudo ./$(TARGET)

run-clock: $(TARGET)
	sudo ./$(TARGET) --clock

run-info: $(TARGET)
	sudo ./$(TARGET) --info

run-bitcoin: $(TARGET)
	sudo ./$(TARGET) --bitcoin

run-chars: $(TARGET)
	sudo ./$(TARGET) --chars

run-progress: $(TARGET)
	sudo ./$(TARGET) --progress

run-all: $(TARGET)
	sudo ./$(TARGET) --all

run-menu: $(TARGET)
	sudo ./$(TARGET) --menu

install-deps:
	bash script_tools/install_deps.sh

remote-build:
	ssh $(PI_HOST) "cd $(PI_DIR) && make -j4"

remote-run:
	ssh $(PI_HOST) "cd $(PI_DIR) && make run"

help:
	@echo "Targets:"
	@echo "  make              - Build $(TARGET)"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make run          - Run with default mode (clock)"
	@echo "  make run-clock    - Run clock mode"
	@echo "  make run-info     - Run system info mode"
	@echo "  make run-bitcoin  - Run bitcoin ticker"
	@echo "  make run-chars    - Run custom chars demo"
	@echo "  make run-progress - Run progress bar demo"
	@echo "  make run-all      - Run all demos"
	@echo "  make run-menu     - Run interactive menu"
	@echo "  make install-deps - Install libbcm2835"
	@echo "  make remote-build - Build on Pi via SSH"
	@echo "  make remote-run   - Build and run on Pi"
	@echo ""
	@echo "Options:"
	@echo "  make BITCOIN=1    - Include Bitcoin ticker (requires libcurl, nlohmann-json)"
	@echo "  make MENU=1       - Include interactive menu mode"
