#include "watch/board.h"

#include <stdint.h>

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define GPIO_P0_BASE 0x50000000u
#define GPIO_P1_BASE 0x50000300u
#define GPIO_OUTSET  0x508u
#define GPIO_OUTCLR  0x50Cu
#define GPIO_IN      0x510u
#define GPIO_DIRSET  0x518u
#define GPIO_PIN_CNF 0x700u

#if WATCH_LCD_SPI_MHZ > 8
#define SPIM0_BASE          0x4002F000u /* SPIM3: up to 32 MHz on nRF52840. */
#else
#define SPIM0_BASE          0x40003000u
#endif
#define SPIM_TASKS_START    REG32(SPIM0_BASE + 0x010u)
#define SPIM_TASKS_STOP     REG32(SPIM0_BASE + 0x014u)
#define SPIM_EVENTS_STOPPED REG32(SPIM0_BASE + 0x104u)
#define SPIM_EVENTS_END     REG32(SPIM0_BASE + 0x118u)
#define SPIM_ENABLE         REG32(SPIM0_BASE + 0x500u)
#define SPIM_PSEL_SCK       REG32(SPIM0_BASE + 0x508u)
#define SPIM_PSEL_MOSI      REG32(SPIM0_BASE + 0x50Cu)
#define SPIM_PSEL_MISO      REG32(SPIM0_BASE + 0x510u)
#define SPIM_FREQUENCY      REG32(SPIM0_BASE + 0x524u)
#define SPIM_RXD_PTR        REG32(SPIM0_BASE + 0x534u)
#define SPIM_RXD_MAXCNT     REG32(SPIM0_BASE + 0x538u)
#define SPIM_TXD_PTR        REG32(SPIM0_BASE + 0x544u)
#define SPIM_TXD_MAXCNT     REG32(SPIM0_BASE + 0x548u)
#define SPIM_CONFIG         REG32(SPIM0_BASE + 0x554u)

#define TWIM1_BASE          0x40004000u
#define TWIM_TASKS_STARTRX  REG32(TWIM1_BASE + 0x000u)
#define TWIM_TASKS_STARTTX  REG32(TWIM1_BASE + 0x008u)
#define TWIM_TASKS_STOP     REG32(TWIM1_BASE + 0x014u)
#define TWIM_EVENTS_STOPPED REG32(TWIM1_BASE + 0x104u)
#define TWIM_EVENTS_ERROR   REG32(TWIM1_BASE + 0x124u)
#define TWIM_SHORTS         REG32(TWIM1_BASE + 0x200u)
#define TWIM_ERRORSRC       REG32(TWIM1_BASE + 0x4C4u)
#define TWIM_ENABLE         REG32(TWIM1_BASE + 0x500u)
#define TWIM_PSEL_SCL       REG32(TWIM1_BASE + 0x508u)
#define TWIM_PSEL_SDA       REG32(TWIM1_BASE + 0x50Cu)
#define TWIM_FREQUENCY      REG32(TWIM1_BASE + 0x524u)
#define TWIM_RXD_PTR        REG32(TWIM1_BASE + 0x534u)
#define TWIM_RXD_MAXCNT     REG32(TWIM1_BASE + 0x538u)
#define TWIM_TXD_PTR        REG32(TWIM1_BASE + 0x544u)
#define TWIM_TXD_MAXCNT     REG32(TWIM1_BASE + 0x548u)
#define TWIM_ADDRESS        REG32(TWIM1_BASE + 0x588u)

#define SYSTICK_CTRL REG32(0xE000E010u)
#define SYSTICK_LOAD REG32(0xE000E014u)
#define SYSTICK_VAL  REG32(0xE000E018u)

#define SPIM_ENABLE_ENABLED 7u
#define SPIM_DISCONNECTED   0xFFFFFFFFu
#define SPIM_TIMEOUT_LOOPS  2000000u
#define TWIM_ENABLE_ENABLED 6u
#define TWIM_FREQUENCY_100K 0x01980000u
#define TWIM_LASTTX_STARTRX (1u << 7)
#define TWIM_LASTTX_STOP    (1u << 9)
#define TWIM_LASTRX_STOP    (1u << 12)
#define TWIM_TIMEOUT_LOOPS  2000000u
#define TX_STAGING_SIZE     512u
#define TOUCH_TX_STAGING_SIZE 16u
#define PROMICRO_STATUS_LED_PIN 15u

#ifndef WATCH_LCD_SPI_MHZ
#define WATCH_LCD_SPI_MHZ 1
#endif

#if WATCH_LCD_SPI_MHZ == 1
#define WATCH_SPIM_FREQUENCY 0x10000000u
#elif WATCH_LCD_SPI_MHZ == 2
#define WATCH_SPIM_FREQUENCY 0x20000000u
#elif WATCH_LCD_SPI_MHZ == 4
#define WATCH_SPIM_FREQUENCY 0x40000000u
#elif WATCH_LCD_SPI_MHZ == 8
#define WATCH_SPIM_FREQUENCY 0x80000000u
#elif WATCH_LCD_SPI_MHZ == 16
#define WATCH_SPIM_FREQUENCY 0x0A000000u
#elif WATCH_LCD_SPI_MHZ == 32
#define WATCH_SPIM_FREQUENCY 0x14000000u
#else
#error "SPIM0 LCD frequency must be 1, 2, 4, or 8 MHz"
#endif

static uint8_t tx_staging[TX_STAGING_SIZE];
static uint8_t touch_tx_staging[TOUCH_TX_STAGING_SIZE];

static uintptr_t gpio_base(uint8_t encoded_pin)
{
    return (encoded_pin & 0x20u) ? GPIO_P1_BASE : GPIO_P0_BASE;
}

static uint32_t gpio_mask(uint8_t encoded_pin)
{
    return 1u << (encoded_pin & 0x1Fu);
}

static void gpio_write(uint8_t encoded_pin, bool high)
{
    uintptr_t base = gpio_base(encoded_pin);
    REG32(base + (high ? GPIO_OUTSET : GPIO_OUTCLR)) =
        gpio_mask(encoded_pin);
}

static void gpio_output_init(uint8_t encoded_pin, bool initial_high)
{
    uintptr_t base = gpio_base(encoded_pin);
    uint32_t pin = encoded_pin & 0x1Fu;

    gpio_write(encoded_pin, initial_high);
    REG32(base + GPIO_DIRSET) = 1u << pin;
    /*
     * DIR=Output, INPUT=Disconnect, PULL=Disabled, DRIVE=S0S1,
     * SENSE=Disabled.
     */
    REG32(base + GPIO_PIN_CNF + (pin * sizeof(uint32_t))) = 0x00000003u;
}

static void gpio_input_init(uint8_t encoded_pin)
{
    uintptr_t base = gpio_base(encoded_pin);
    uint32_t pin = encoded_pin & 0x1Fu;

    /* DIR=Input, INPUT=Connect, PULL=Disabled, SENSE=Disabled. */
    REG32(base + GPIO_PIN_CNF + (pin * sizeof(uint32_t))) = 0u;
}

static void gpio_i2c_init(uint8_t encoded_pin)
{
    uintptr_t base = gpio_base(encoded_pin);
    uint32_t pin = encoded_pin & 0x1Fu;

    /*
     * Input connected, internal pull-up, open-drain-compatible S0D1 drive.
     * The pull-up also makes generic display clones without populated I2C
     * pull resistors usable during bring-up.
     */
    REG32(base + GPIO_PIN_CNF + (pin * sizeof(uint32_t))) = 0x0000060Cu;
}

static bool gpio_read(uint8_t encoded_pin)
{
    return (REG32(gpio_base(encoded_pin) + GPIO_IN) &
            gpio_mask(encoded_pin)) != 0u;
}

void watch_board_init(void)
{
    /* SuperMini/ProMicro status LED is connected to P0.15. */
    gpio_output_init(PROMICRO_STATUS_LED_PIN, false);

    /* SPIM PSEL routes the peripheral, but GPIO direction remains explicit. */
    gpio_output_init(WATCH_LCD_SCK_PIN, false);
    gpio_output_init(WATCH_LCD_MOSI_PIN, false);
    gpio_output_init(WATCH_LCD_CS_PIN, true);
    gpio_output_init(WATCH_LCD_DC_PIN, true);
    gpio_output_init(WATCH_LCD_RESET_PIN, true);
    gpio_output_init(WATCH_LCD_BACKLIGHT_PIN, true);

    SPIM_ENABLE = 0u;
    SPIM_PSEL_SCK = WATCH_LCD_SCK_PIN;
    SPIM_PSEL_MOSI = WATCH_LCD_MOSI_PIN;
    SPIM_PSEL_MISO = SPIM_DISCONNECTED;
    SPIM_FREQUENCY = WATCH_SPIM_FREQUENCY;
    SPIM_CONFIG = 0u; /* MSB first, mode 0. */
    SPIM_RXD_PTR = 0u;
    SPIM_RXD_MAXCNT = 0u;
    SPIM_ENABLE = SPIM_ENABLE_ENABLED;
}

void watch_status_led_set(bool enabled)
{
    gpio_write(PROMICRO_STATUS_LED_PIN, enabled);
}

void watch_delay_ms(uint32_t milliseconds)
{
    SYSTICK_LOAD = 64000u - 1u;
    SYSTICK_VAL = 0u;
    SYSTICK_CTRL = 0x5u; /* Enable, processor clock, no interrupt. */

    while (milliseconds-- > 0u) {
        while ((SYSTICK_CTRL & (1u << 16)) == 0u) {
        }
    }

    SYSTICK_CTRL = 0u;
}

void watch_lcd_select(bool selected)
{
    gpio_write(WATCH_LCD_CS_PIN, !selected);
}

void watch_lcd_set_data_mode(bool data)
{
    gpio_write(WATCH_LCD_DC_PIN, data);
}

void watch_lcd_set_reset(bool released)
{
    gpio_write(WATCH_LCD_RESET_PIN, released);
}

void watch_lcd_set_backlight(bool enabled)
{
    gpio_write(WATCH_LCD_BACKLIGHT_PIN, enabled);
}

bool watch_lcd_write(const uint8_t *data, size_t length)
{
    if ((data == 0) && (length != 0u)) {
        return false;
    }

    while (length > 0u) {
        size_t chunk = length;
        if (chunk > sizeof(tx_staging)) {
            chunk = sizeof(tx_staging);
        }

        for (size_t i = 0; i < chunk; ++i) {
            tx_staging[i] = data[i];
        }

        SPIM_EVENTS_END = 0u;
        SPIM_TXD_PTR = (uint32_t)(uintptr_t)tx_staging;
        SPIM_TXD_MAXCNT = (uint32_t)chunk;
        SPIM_TASKS_START = 1u;

        uint32_t timeout = SPIM_TIMEOUT_LOOPS;
        while ((SPIM_EVENTS_END == 0u) && (timeout > 0u)) {
            --timeout;
        }
        if (SPIM_EVENTS_END == 0u) {
            SPIM_EVENTS_STOPPED = 0u;
            SPIM_TASKS_STOP = 1u;
            while ((SPIM_EVENTS_STOPPED == 0u) && (timeout > 0u)) {
                --timeout;
            }
            return false;
        }

        data += chunk;
        length -= chunk;
    }

    return true;
}

void watch_touch_bus_init(void)
{
    gpio_i2c_init(WATCH_TOUCH_SCL_PIN);
    gpio_i2c_init(WATCH_TOUCH_SDA_PIN);
    gpio_output_init(WATCH_TOUCH_RESET_PIN, true);
    gpio_input_init(WATCH_TOUCH_INT_PIN);

    TWIM_ENABLE = 0u;
    TWIM_PSEL_SCL = WATCH_TOUCH_SCL_PIN;
    TWIM_PSEL_SDA = WATCH_TOUCH_SDA_PIN;
    TWIM_FREQUENCY = TWIM_FREQUENCY_100K;
    TWIM_ENABLE = TWIM_ENABLE_ENABLED;
}

void watch_touch_set_reset(bool released)
{
    gpio_write(WATCH_TOUCH_RESET_PIN, released);
}

bool watch_touch_interrupt_active(void)
{
    return !gpio_read(WATCH_TOUCH_INT_PIN);
}

bool watch_touch_write_read(uint8_t address,
                            const uint8_t *tx_data,
                            size_t tx_length,
                            uint8_t *rx_data,
                            size_t rx_length)
{
    if ((tx_length > sizeof(touch_tx_staging)) ||
        ((tx_length != 0u) && (tx_data == 0)) ||
        ((rx_length != 0u) && (rx_data == 0))) {
        return false;
    }

    for (size_t i = 0; i < tx_length; ++i) {
        touch_tx_staging[i] = tx_data[i];
    }

    TWIM_EVENTS_STOPPED = 0u;
    TWIM_EVENTS_ERROR = 0u;
    TWIM_ERRORSRC = 0x7u;
    TWIM_ADDRESS = address;
    TWIM_TXD_PTR = (uint32_t)(uintptr_t)touch_tx_staging;
    TWIM_TXD_MAXCNT = (uint32_t)tx_length;
    TWIM_RXD_PTR = (uint32_t)(uintptr_t)rx_data;
    TWIM_RXD_MAXCNT = (uint32_t)rx_length;

    if ((tx_length != 0u) && (rx_length != 0u)) {
        TWIM_SHORTS = TWIM_LASTTX_STARTRX | TWIM_LASTRX_STOP;
        TWIM_TASKS_STARTTX = 1u;
    } else if (tx_length != 0u) {
        TWIM_SHORTS = TWIM_LASTTX_STOP;
        TWIM_TASKS_STARTTX = 1u;
    } else if (rx_length != 0u) {
        TWIM_SHORTS = TWIM_LASTRX_STOP;
        TWIM_TASKS_STARTRX = 1u;
    } else {
        return false;
    }

    uint32_t timeout = TWIM_TIMEOUT_LOOPS;
    while ((TWIM_EVENTS_STOPPED == 0u) &&
           (TWIM_EVENTS_ERROR == 0u) &&
           (timeout > 0u)) {
        --timeout;
    }

    if ((TWIM_EVENTS_ERROR != 0u) || (timeout == 0u)) {
        TWIM_TASKS_STOP = 1u;
        timeout = TWIM_TIMEOUT_LOOPS;
        while ((TWIM_EVENTS_STOPPED == 0u) && (timeout > 0u)) {
            --timeout;
        }
        TWIM_SHORTS = 0u;
        return false;
    }

    TWIM_SHORTS = 0u;
    return true;
}
