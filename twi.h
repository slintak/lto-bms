#ifndef __BMS_TWI_H__
#define __BMS_TWI_H__

#include <stdbool.h>
#include <stdint.h>

#define BMS_TWI_ADDRESS_PRIMARY 0x41

#define TWI_READ true
#define TWI_WRITE false

typedef enum {
    TWI_STATE_IDLE,
    TWI_STATE_REGISTER,
    TWI_STATE_DATA,
} twi_state_e;

typedef struct {
    uint8_t active;
    uint8_t state;
    union {
        uint8_t raw[32];
        struct {
            uint16_t config;
            int16_t current;
            uint16_t voltage;
            uint16_t power;
            uint16_t reserved_04h;
            uint16_t reserved_05h;
            uint16_t mask_enable;
            uint16_t alert_limit;
            uint16_t reserved_08h;
            uint16_t reserved_09h;
            uint16_t reserved_0Ah;
            uint16_t reserved_0Bh;
            uint16_t reserved_0Ch;
            uint16_t reserved_0Dh;
            uint16_t manufacturer;
            uint16_t die_id;
        } ina260;
    };
} twi_registers_t;

void TWI_init(void);
twi_registers_t *TWI_get_registers(void);

#endif