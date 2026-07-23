#ifndef WATCH_BOARD_H
#define WATCH_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Touch-display wiring, USB connector at the top:
 *   GC9A01 SCL    -> P0.20 (silk "020")
 *   GC9A01 SDA    -> P0.22 (silk "022")
 *   GC9A01 CS     -> P0.24 (silk "024")
 *   GC9A01 DC     -> P1.00 (silk "100")
 *   GC9A01 RES    -> P0.11 (silk "011")
 *   LCD BLK       -> P1.04 (silk "104")
 *   CST816S SCL   -> P0.29 (silk "029")
 *   CST816S SDA   -> P0.31 (silk "031")
 *   CST816S RESET -> P0.02 (silk "002")
 *   CST816S INT   -> P1.15 (silk "115")
 */
enum {
    WATCH_LCD_SCK_PIN = 20,
    WATCH_LCD_MOSI_PIN = 22,
    WATCH_LCD_CS_PIN = 24,
    WATCH_LCD_DC_PIN = 32,  /* P1.00: port is encoded in bit 5. */
    WATCH_LCD_RESET_PIN = 11,
    WATCH_LCD_BACKLIGHT_PIN = 36, /* P1.04 */
    WATCH_TOUCH_SCL_PIN = 29,
    WATCH_TOUCH_SDA_PIN = 31,
    WATCH_TOUCH_RESET_PIN = 2,
    WATCH_TOUCH_INT_PIN = 47 /* P1.15 */
};

void watch_board_init(void);
void watch_delay_ms(uint32_t milliseconds);
void watch_lcd_select(bool selected);
void watch_lcd_set_data_mode(bool data);
void watch_lcd_set_reset(bool released);
void watch_lcd_set_backlight(bool enabled);
bool watch_lcd_write(const uint8_t *data, size_t length);
void watch_touch_bus_init(void);
void watch_touch_set_reset(bool released);
bool watch_touch_interrupt_active(void);
bool watch_touch_write_read(uint8_t address,
                            const uint8_t *tx_data,
                            size_t tx_length,
                            uint8_t *rx_data,
                            size_t rx_length);

#endif
