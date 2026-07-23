#include "watch/cst816.h"

#include <stddef.h>

#include "watch/board.h"

#define CST816S_ADDRESS          0x15u
#define CST816S_REG_GESTURE      0x01u
#define CST816S_REG_CHIP_ID      0xA7u
#define CST816S_REG_PROJECT_ID   0xA8u
#define CST816S_REG_FW_VERSION   0xA9u
#define CST816S_REG_DIS_AUTOSLEEP 0xFEu

static bool register_read(uint8_t reg, uint8_t *data, size_t length)
{
    return watch_touch_write_read(CST816S_ADDRESS,
                                  &reg,
                                  sizeof(reg),
                                  data,
                                  length);
}

static bool register_write(uint8_t reg, uint8_t value)
{
    uint8_t message[2] = {reg, value};
    return watch_touch_write_read(CST816S_ADDRESS,
                                  message,
                                  sizeof(message),
                                  0,
                                  0u);
}

bool cst816_init(cst816_identity_t *identity)
{
    if (identity == 0) {
        return false;
    }

    watch_touch_bus_init();
    watch_touch_set_reset(false);
    watch_delay_ms(20u);
    watch_touch_set_reset(true);
    watch_delay_ms(100u);

    uint8_t ids[3] = {0u, 0u, 0u};
    if (!register_read(CST816S_REG_CHIP_ID, ids, sizeof(ids))) {
        return false;
    }

    identity->chip_id = ids[0];
    identity->project_id = ids[1];
    identity->firmware_version = ids[2];

    /* Keep the controller responsive while polling during bring-up. */
    return register_write(CST816S_REG_DIS_AUTOSLEEP, 0x01u);
}

bool cst816_read_touch(cst816_touch_t *touch)
{
    if (touch == 0) {
        return false;
    }

    uint8_t data[6];
    if (!register_read(CST816S_REG_GESTURE, data, sizeof(data))) {
        return false;
    }

    touch->gesture = data[0];
    touch->fingers = data[1];
    touch->event = data[2] >> 6;
    touch->x = (uint16_t)(((uint16_t)(data[2] & 0x0Fu) << 8) | data[3]);
    touch->y = (uint16_t)(((uint16_t)(data[4] & 0x0Fu) << 8) | data[5]);
    return true;
}
