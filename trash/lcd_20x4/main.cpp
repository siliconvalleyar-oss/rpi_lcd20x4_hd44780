//==============================================================================
// lcd_20x4_clock.cpp - LCD 20x4 con reloj en tiempo real
// Muestra la hora del sistema actualizada cada segundo.
//==============================================================================

#include <bcm2835.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

//==============================================================================
// Definición de pines GPIO (usando números BCM directos)
//==============================================================================
#define PIN_RS  13   // GPIO 13 (pin físico 33)
#define PIN_EN  6    // GPIO 6  (pin físico 31)

#define PIN_D4  26   // GPIO 26 (pin físico 37)
#define PIN_D5  19   // GPIO 19 (pin físico 35)
#define PIN_D6  20   // GPIO 20 (pin físico 38)
#define PIN_D7  21   // GPIO 21 (pin físico 40)

//==============================================================================
// Prototipos de funciones
//==============================================================================
void lcd_init(void);
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t data);
void lcd_write_string(const char *str);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_enable_pulse(void);
void lcd_send_nibble(uint8_t nibble);
void lcd_send_byte(uint8_t byte);

//==============================================================================
// Función principal
//==============================================================================
int main() {
    if (!bcm2835_init()) {
        printf("Error: No se pudo inicializar bcm2835. Ejecuta con 'sudo'.\n");
        return 1;
    }

    // Configurar pines de control y datos como salida
    bcm2835_gpio_fsel(PIN_RS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_EN, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_D4, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_D5, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_D6, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_D7, BCM2835_GPIO_FSEL_OUTP);

    // Estado inicial de pines
    bcm2835_gpio_write(PIN_RS, LOW);
    bcm2835_gpio_write(PIN_EN, LOW);
    bcm2835_gpio_write(PIN_D4, LOW);
    bcm2835_gpio_write(PIN_D5, LOW);
    bcm2835_gpio_write(PIN_D6, LOW);
    bcm2835_gpio_write(PIN_D7, LOW);

    lcd_init();  // Inicializar LCD

    // Mensajes fijos (solo se escriben una vez)
    lcd_set_cursor(0, 0);
  //  lcd_write_string("Hora del sistema  ");
    lcd_set_cursor(1, 0);
    lcd_write_string("Fecha:            ");
    lcd_set_cursor(2, 0);
    lcd_write_string("Hora:             ");
    lcd_set_cursor(3, 0);
    lcd_write_string("Actualizando...   ");

    printf("Mostrando hora en LCD. Presiona Ctrl+C para salir.\n");

    time_t rawtime;
    struct tm *timeinfo;
    char fecha_str[20];
    char hora_str[20];

    while (1) {
        // Obtener hora local del sistema
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        // Formatear fecha (DD/MM/AAAA)
        strftime(fecha_str, sizeof(fecha_str), "%d/%m/%Y", timeinfo);
        // Formatear hora (HH:MM:SS)
        strftime(hora_str, sizeof(hora_str), "%H:%M:%S", timeinfo);

        // Escribir fecha en línea 1 (columna 7 para centrar visualmente)
        lcd_set_cursor(1, 7);
        lcd_write_string(fecha_str);
        // Escribir hora en línea 2 (columna 7)
        lcd_set_cursor(2, 7);
        lcd_write_string(hora_str);

        // Opcional: Mostrar segundos adicionales en línea 3
//        lcd_set_cursor(3, 0);
//        lcd_write_string("Hora actualizada ");

        sleep(1); // Actualizar cada segundo
    }

    bcm2835_close();
    return 0;
}

//==============================================================================
// Funciones del LCD (igual que antes, sin cambios)
//==============================================================================

void lcd_enable_pulse() {
    bcm2835_gpio_write(PIN_EN, HIGH);
    bcm2835_delayMicroseconds(1);
    bcm2835_gpio_write(PIN_EN, LOW);
    bcm2835_delayMicroseconds(50);
}

void lcd_send_nibble(uint8_t nibble) {
    bcm2835_gpio_write(PIN_D4, (nibble >> 0) & 1);
    bcm2835_gpio_write(PIN_D5, (nibble >> 1) & 1);
    bcm2835_gpio_write(PIN_D6, (nibble >> 2) & 1);
    bcm2835_gpio_write(PIN_D7, (nibble >> 3) & 1);
    lcd_enable_pulse();
}

void lcd_send_byte(uint8_t byte) {
    lcd_send_nibble(byte >> 4);
    lcd_send_nibble(byte & 0x0F);
}

void lcd_command(uint8_t cmd) {
    bcm2835_gpio_write(PIN_RS, LOW);
    lcd_send_byte(cmd);
    bcm2835_delayMicroseconds(100);
}

void lcd_data(uint8_t data) {
    bcm2835_gpio_write(PIN_RS, HIGH);
    lcd_send_byte(data);
    bcm2835_delayMicroseconds(50);
}

void lcd_write_string(const char *str) {
    while (*str) {
        lcd_data((uint8_t)*str++);
    }
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    lcd_command(0x80 | (col + row_offsets[row]));
}

void lcd_init() {
    bcm2835_delay(50);
    bcm2835_gpio_write(PIN_RS, LOW);
    lcd_send_nibble(0x03);
    bcm2835_delay(5);
    lcd_send_nibble(0x03);
    bcm2835_delay(1);
    lcd_send_nibble(0x03);
    lcd_send_nibble(0x02);
    lcd_command(0x28);
    lcd_command(0x08);
    lcd_command(0x01);
    bcm2835_delay(2);
    lcd_command(0x06);
    lcd_command(0x0C);
}
