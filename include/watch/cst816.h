#ifndef WATCH_CST816_H
#define WATCH_CST816_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t chip_id;
    uint8_t project_id;
    uint8_t firmware_version;
} cst816_identity_t;

typedef struct {
    uint8_t gesture;
    uint8_t fingers;
    uint8_t event;
    uint16_t x;
    uint16_t y;
} cst816_touch_t;

bool cst816_init(cst816_identity_t *identity);
bool cst816_read_touch(cst816_touch_t *touch);

#endif
