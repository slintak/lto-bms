#include <stdint.h>
#include <string.h>
#include <util/crc16.h>
#include <avr/eeprom.h>

#include "config.h"
#include "uart.h"

// bms_eeprom_config_t EEMEM *bms_config_eeprom;

#if defined(CELL_NAION)
/**
 *  Na-ion 1S
 */
static bms_eeprom_config_t bms_config_default = {
    .magic = CONFIG_MAGIC_CONSTANT,
    .serial_number = 0,
    .temp_offset = 0,
    .ovlo_cutoff  = 3900,
    .ovlo_release = 3800,
    .uvlo_release = 2200,
    .uvlo_cutoff  = 2000,
    .max_current = 1000,
    .oclo_timeout = 10
};
#elif defined(CELL_LTO)
/**
 *  LTO 1S
 */
static bms_eeprom_config_t bms_config_default = {
    .magic = CONFIG_MAGIC_CONSTANT,
    .serial_number = 0,
    .temp_offset = 0,
    .ovlo_cutoff = 2800,
    .ovlo_release = 2700,
    .uvlo_release = 1800,
    .uvlo_cutoff = 1700,
    .max_current = 1000,
    .oclo_timeout = 10
};
#else
#error "CELL_LTO or CELL_NAION must be defined"
#endif

void eeprom_config_init(bms_eeprom_config_t *cnf) {
    if(!eeprom_config_load(cnf)) {
        memcpy(
            (void *)cnf,
            (void *)&bms_config_default,
            sizeof(bms_eeprom_config_t)
        );
        eeprom_config_update(cnf);
    }
}

bool eeprom_config_load(bms_eeprom_config_t *cnf) {
    uint8_t *data = (uint8_t *)cnf;
    uint16_t crc = 0xFFFF;

    for(uint16_t i = 0; i < sizeof(bms_eeprom_config_t); i++) {
        data[i] = eeprom_read_byte((uint8_t *)i);
        crc = _crc16_update(crc, data[i]);

        DEBUG("%02X:", data[i]);
    }

    if(crc == 0x0000 && cnf->magic == CONFIG_MAGIC_CONSTANT) {
        DEBUG("EEPROM loaded.\n");
        return true;
    }

    DEBUG("EEPROM loading failed!\n");
    return false;
}

void eeprom_config_update(bms_eeprom_config_t *cnf) {
    uint8_t *data = (uint8_t *)cnf;
    cnf->crc = 0xFFFF;

    for(uint8_t i = 0; i < (sizeof(bms_eeprom_config_t) - 2); i++) {
        cnf->crc = _crc16_update(cnf->crc, data[i]);
    }

    eeprom_update_block((void *)cnf, 0x00, sizeof(bms_eeprom_config_t));
    DEBUG("EEPROM initialied.\n");
}
