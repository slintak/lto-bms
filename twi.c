#include <avr/io.h>
#include <avr/interrupt.h>

#include "gpio.h"
#include "twi.h"

static twi_registers_t twi_registers = {
    .active = 0,
    .state = TWI_STATE_IDLE,
    .ina260 = {
        .config = HTONS(0x6127),
        .manufacturer = HTONS(0x5449),
        .die_id = HTONS(0x2270)
    },
};

ISR(TWI0_TWIS_vect) {
    if(TWI0.SSTATUS & TWI_DIF_bm) {
        // Data Interrupt Flag

        if(((TWI0.SSTATUS & TWI_DIR_bm) >> TWI_DIR_bp) == TWI_WRITE) {
            // Data Write (Host -> Client)
            if(twi_registers.state == TWI_STATE_REGISTER) {
                twi_registers.active = (TWI0.SDATA & 0x0F) * 2;
                twi_registers.state = TWI_STATE_DATA;
            } else if(twi_registers.state == TWI_STATE_DATA) {
                twi_registers.raw[twi_registers.active] = TWI0.SDATA;
                twi_registers.active++;
            }
        } else {
            // Data Read (Host <- Client)
            TWI0.SDATA = twi_registers.raw[twi_registers.active];
            twi_registers.active++;
        }

        //ACK
        TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
    } else if (TWI0.SSTATUS & TWI_APIF_bm) {
        // Address Match or STOP

        if (TWI0.SSTATUS & TWI_AP_ADR_gc) {
            // Address Match
            twi_registers.state = TWI_STATE_REGISTER;
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        } else {
            // STOP Condition
            twi_registers.state = TWI_STATE_IDLE;
            TWI0.SCTRLB = TWI_ACKACT_NACK_gc | TWI_SCMD_COMPTRANS_gc;
        }
    } else {
        volatile uint8_t capture __attribute__((unused)) = TWI0.SSTATUS;
        asm("NOP");
    }
}

void TWI_init(void) {
    PORTB.DIRSET = BMS_IIC_SCL | BMS_IIC_SDA;
    TWI0.CTRLA = TWI_SDAHOLD_50NS_gc | TWI_SDASETUP_8CYC_gc | TWI_FMPEN_bm;
    TWI0.SADDR = BMS_TWI_ADDRESS_PRIMARY << 1;
    TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm | TWI_ENABLE_bm;
}

twi_registers_t *TWI_get_registers(void) {
    return &twi_registers;
}