#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "adc.h"
#include "config.h"
#include "gpio.h"
#include "uart.h"
#include "twi.h"

// Minimum voltage difference (in mV) required between load and battery
// for the charging process to begin when UVLO is active. The cutoff protection
// will only be disabled if the load voltage is at least this much higher than
// the battery voltage.
#define UVLO_MIN_VOLTAGE_DIFFERENCE 50 // mV

// Minimum voltage difference (in mV) required between load and battery
// for the discharging process to begin when OVLO is active. The cutoff
// protection will only be disabled if the load voltage is at least this much
// higher than the battery voltage.
#define OVLO_MIN_VOLTAGE_DIFFERENCE 50 // mV

// How often to print measurements to stdout. In seconds.
#define MEASUREMENT_LOG_INTERVAL 60


#define BMS_CHANGE_STATE(ns) do { bms_state = ns; bms_state_transition_timestamp = uptime; } while(0)

static bms_states_e bms_state = BMS_STATE_INVALID;
static uint32_t bms_state_transition_timestamp = 0;

static bms_eeprom_config_t bms_config;
static uint32_t energy_discharge = 0;
static uint32_t energy_charge = 0;
static int32_t charge = 0;
static bool tick = false;
static uint32_t uptime = 0;
static adc_measurement_t *adc;

static uint32_t next_log_message = 0;

ISR(RTC_PIT_vect) {
    tick = true;
    uptime++;

    // Clear flag.
    RTC.PITINTFLAGS = RTC_PI_bm;
}

/**
 * Initialize RTC.
 * This will run 1.024 kHz clock with 1024 prescaler. We will
 * have RTC PIT interrput every 1 second. This will be our wake-up
 * source from the standby sleep.
 */
void RTC_init(void) {
    // 1.024 kHz internal crystal oscillator.
    RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;
    // Periodic Interrupt: enabled
    RTC.PITINTCTRL = RTC_PI_bm;
    while(RTC.PITSTATUS & RTC_CTRLABUSY_bm);

    RTC.PITCTRLA = RTC_PERIOD_CYC1024_gc | RTC_PITEN_bm;
}

void RTC_start(void) {
    RTC.PITCTRLA |= RTC_PITEN_bm;
}

void RTC_stop(void) {
    RTC.PITCTRLA &= ~RTC_PITEN_bm;
}

void PORT_init(void) {
    BMS_CUTOFF_ON();
    // BMS_CUTOFF_3V3_ON();

    // When debug (UART) is needed.
    PORTB.DIR |= BMS_UART_TX;

    // Disable digital input buffer for all analog inputs.
    // This should reduce consumption a little.
    PORTA.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTB.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTB.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
}

/**
 * Enter a standby sleep mode.
 * Only the RTC PIT can wake us.
 */
void standby_enable(void) {
    SLPCTRL.CTRLA = SLPCTRL_SMODE_STDBY_gc | SLPCTRL_SEN_bm;
    __asm__ __volatile__ ("SLEEP");
}

int main(void) {
    // Watchdog should be enabled automatically by fuses.
    // They are in the `fuses.hex` file, use `make fuses` to write them.

    // Configure clock with 16x prescaler and enable prescaler division.
    // This should result in 20 / 16 = 1.25 MHz CPU clock. This should result
    // in lower consumption and more stable operation under lower voltages.
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_16X_gc | CLKCTRL_PEN_bm;

    PORT_init();

    RTC_init();
    RTC_start();

    adc_init();
    adc_start();

    TWI_init();
    twi_registers_t *twi_registers = TWI_get_registers();

    sei();

    LOG_INIT(9600);
    DEBUG("Up and running.\n");

    // Turn on cutoff (turn off voltage on the ouput)
    BMS_CUTOFF_ON();
    // BMS_CUTOFF_3V3_ON();

    eeprom_config_init(&bms_config);
    BMS_CHANGE_STATE(BMS_STATE_IDLE);

    while(1) {
        if(!tick) {
            goto main_continue;
        }

        if(!adc_get_measurements(&adc)) {
            DEBUG("Failed to get new measurement!\n");
            goto main_error;
        }

        int32_t current = ADC_TO_CURRENT(adc->current);
        uint32_t battery = ADC_TO_VOLTAGE(adc->battery);
        uint32_t load = ADC_TO_VOLTAGE(adc->load);
        uint32_t temperature = adc_to_temperature(adc->temperature);

        // Current values from -2 to +2 mA are most probably noise on ADC.
        // These will be ignored. This will introduce a small error in our
        // measurement.
        current = (abs(current) > 2) ? current : 0;

        // Current in mA added every second to a total charge in mAs.
        // To calculate mAh later, we will divide the sum by `3600`.
        charge += current;

        // Calculate total discharged/charged energy since last reset.
        if(current > 0) {
            energy_discharge += current * battery / 1000;
        } else {
            energy_charge += abs(current) * battery / 1000;
        }

        switch(bms_state) {
        case BMS_STATE_IDLE:
        case BMS_STATE_CHARGING:
        case BMS_STATE_DISCHARGING:
            if(battery <= bms_config.uvlo_cutoff) {
                BMS_CHANGE_STATE(BMS_STATE_UVLO);
            } else if(battery >= bms_config.ovlo_cutoff) {
                BMS_CHANGE_STATE(BMS_STATE_OVLO);
            } else if(current > bms_config.max_current) {
                BMS_CHANGE_STATE(BMS_STATE_OCLO);
            } else {
                BMS_CUTOFF_OFF();
                // BMS_CUTOFF_3V3_OFF();
                if(current > 0) {
                    BMS_CHANGE_STATE(BMS_STATE_DISCHARGING);
                } else if(current < 0) {
                    BMS_CHANGE_STATE(BMS_STATE_CHARGING);
                } else {
                    BMS_CHANGE_STATE(BMS_STATE_IDLE);
                }
            }
            break;

        case BMS_STATE_UVLO:
            if(battery >= bms_config.uvlo_release) {
                BMS_CHANGE_STATE(BMS_STATE_IDLE);
            } else {
                // Check if the load-side voltage is higher than the battery
                // voltage. If so, the charging process can begin, and we need
                // to disable the cutoff protection.
                // Only disable the cutoff if the load voltage is at least
                // 50 mV higher than the battery voltage.
                int32_t vdiff = (int32_t)load - battery;
                if(vdiff > UVLO_MIN_VOLTAGE_DIFFERENCE) {
                    BMS_CUTOFF_OFF();
                    // BMS_CUTOFF_3V3_OFF();
                } else {
                    BMS_CUTOFF_ON();
                    // BMS_CUTOFF_3V3_ON();
                }
            }
            break;

        case BMS_STATE_OVLO:
            if(battery <= bms_config.ovlo_release) {
                BMS_CHANGE_STATE(BMS_STATE_IDLE);
            } else {
                // Check if the load-side voltage is lower than the battery
                // voltage. If so, the discharging process can begin, and we need
                // to disable the cutoff protection.
                // Only disable the cutoff if the load voltage is at least
                // 50 mV lower than the battery voltage.
                int32_t vdiff = (int32_t)battery - load;
                if(vdiff > OVLO_MIN_VOLTAGE_DIFFERENCE) {
                    BMS_CUTOFF_OFF();
                } else if(vdiff < -5) {
                    BMS_CUTOFF_ON();
                }
            }
            break;

        case BMS_STATE_OCLO:
            // Release the over-current event only after a specified time period.
            if((uptime - bms_state_transition_timestamp) > bms_config.oclo_timeout) {
                BMS_CUTOFF_OFF();
                BMS_CHANGE_STATE(BMS_STATE_IDLE);
            } else {
                BMS_CUTOFF_ON();
            }
        break;

        case BMS_STATE_INVALID:
        case BMS_STATE_ERR:
        default:
            DEBUG("INVALID STATE: %u!\n", bms_state);
            goto main_error;
        }

        // Update new measured values into the registers of the "INA260" interface.
        twi_registers->ina260.current = HTONS((int16_t)current / 1.25);
        twi_registers->ina260.voltage = HTONS((uint16_t)battery / 1.25);
        twi_registers->ina260.power = HTONS((uint16_t)((abs(current) * battery) / 10000));

        if(uptime >= next_log_message) {
            LOG("lto-bms,id=%04X uptime=%lui,v_batt=%lui,v_load=%lui,current=%ldi,",
                bms_config.serial_number,
                uptime,
                battery,
                load,
                current
            );

            LOG("E_dis=%lui,E_chg=%lui,charge=%ldi,temp=%lui,gain=%ui,state=\"%s\"\n",
                energy_discharge / 3600,
                energy_charge / 3600,
                charge / 3600,
                temperature + bms_config.temp_offset,
                1 << adc_get_gain(),
                bms_state_str[bms_state]
            );

            next_log_message = next_log_message + MEASUREMENT_LOG_INTERVAL;
            _delay_ms(5); // Small delay for UART to transmit all data.
        }

        tick = false;
        wdt_reset();
        adc_start();

main_continue:
        standby_enable();
    }

main_error:
    BMS_CUTOFF_ON();
    DEBUG("Main loop finished!\n");
    while(1) {
        _delay_ms(1000);
    }
    return 0;
}
