#ifndef __BMS_CONFIG_H__
#define __BMS_CONFIG_H__

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_MAGIC_CONSTANT 0x7337

/**
 * This structure represents the data stored in the EEPROM.
 * EEPROM size for ATtiny824 is 128 B.
 */
typedef struct {
    // This constant serves as an indicator of valid daata in the EEPROM.
    uint16_t magic;

    uint16_t serial_number;

    // Difference between the real temperature and measured value from the
    // interval sensor. In Kelvines.
    // This offset is added to the measured value.
    int16_t temp_offset;

    // Limit settings.
    uint16_t ovlo_cutoff;
    uint16_t ovlo_release;
    uint16_t uvlo_release;
    uint16_t uvlo_cutoff;
    uint16_t max_current;
    uint16_t oclo_timeout;  // How long to remain in the over-current state.

    // CRC16 (polynomial: 0xa001, initial: 0xffff)
    uint16_t crc;
} bms_eeprom_config_t;

typedef enum {
    BMS_STATE_INVALID,
    BMS_STATE_UVLO,    // Under-voltage lockout
    BMS_STATE_OVLO,    // Over-voltage lockout
    BMS_STATE_OCLO,    // Over-current lockout
    BMS_STATE_CHARGING,
    BMS_STATE_DISCHARGING,
    BMS_STATE_IDLE,
    BMS_STATE_ERR
} bms_states_e;

static const char bms_state_str[BMS_STATE_ERR+1][22] = {
    "BMS_STATE_INVALID",
    "BMS_STATE_UVLO",    // Under-voltage lockout
    "BMS_STATE_OVLO",    // Over-voltage lockout
    "BMS_STATE_OCLO",    // Over-current lockout
    "BMS_STATE_CHARGING",
    "BMS_STATE_DISCHARGING",
    "BMS_STATE_IDLE",
    "BMS_STATE_ERR"
};

void eeprom_config_init(bms_eeprom_config_t *cnf);
bool eeprom_config_load(bms_eeprom_config_t *cnf);
void eeprom_config_update(bms_eeprom_config_t *cnf);

#endif