# Makefile para compilar la aplicación del LCD 20x4
CXX = g++
CXXFLAGS = -Wall -O2
LDFLAGS = -lbcm2835
TARGET = app
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	sudo ./$(TARGET)

.PHONY: all clean run
