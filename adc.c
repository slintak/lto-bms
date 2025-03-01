#include <stdlib.h>
#include <math.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "adc.h"
#include "gpio.h"
#include "uart.h"

#define ABS(a) (((a) < 0) ? -(a) : (a))
#define ADC_TIMEBASE_VALUE ((uint8_t) ceil(F_CPU*0.000001))
#define ADC_SAMPLE_DURATION  50
#define ADC_SAMPLE_NUM       ADC_SAMPNUM_ACC16_gc
#define ADC_PGA_GAIN(g)      ADC0.PGACTRL = ((g << 5) | ADC_PGABIASSEL_1_4X_gc | ADC_PGAEN_bm)

static adc_gain_e adc_gain = ADC_GAIN_16X;

const static adc_channels_t adc_channels[ADC_CHANNELS_NUM] = {
    // Differential measurement of current on the shunt resistor.
    // This measurement should use gain amplifier (PGA).
    {.muxpos = BMS_ADC_BP | ADC_VIA_PGA_gc, .muxneg = BMS_ADC_BN | ADC_VIA_PGA_gc, .diff = ADC_DIFF_bm},
    // Single ended measurements of the voltages on battery and output
    // and temperature of the internal MCU sensor. No PGA.
    {.muxpos = BMS_ADC_BATT,      .muxneg = ADC_MUXNEG_GND_gc, .diff = 0},
    {.muxpos = BMS_ADC_OUT,       .muxneg = ADC_MUXNEG_GND_gc, .diff = 0},
    {.muxpos = BMS_ADC_TEMPSENSE, .muxneg = ADC_MUXNEG_GND_gc, .diff = 0},
};

static adc_result_t adc_results = {
    .new_measurement = 0,
    .active_channel =  0,
    .adc = {0, 0, 0, 0},
    .meas.raw = {0, 0, 0, 0},
};

ISR(ADC0_RESRDY_vect) {
    uint8_t start = 0;

    ADC0.INTFLAGS = ADC_RESRDY_bm;

    adc_results.adc[adc_results.active_channel] = (int32_t)ADC0.RESULT;

    if(adc_results.active_channel == ADC_CHANNELS_NUM - 1) {
        adc_results.new_measurement = 1;
        adc_results.active_channel = 0;
    } else {
        start = ADC_START_IMMEDIATE_gc;
        adc_results.active_channel++;
    }

    ADC0.MUXPOS = adc_channels[adc_results.active_channel].muxpos;
    ADC0.MUXNEG = adc_channels[adc_results.active_channel].muxneg;
    ADC0.COMMAND = start | ADC_MODE_BURST_gc | adc_channels[adc_results.active_channel].diff;
}

void adc_init(void) {
    ADC0.CTRLA = 1 << ADC_ENABLE_bp    // ADC Enable: enabled
               | 1 << ADC_RUNSTDBY_bp;  // Run standby mode: enabled
    ADC0.CTRLB = ADC_PRESC_DIV2_gc;    // CLK_ADC = CLK_PER / PRESCALER
    ADC0.CTRLC = ADC_REFSEL_1024MV_gc | (ADC_TIMEBASE_VALUE << ADC_TIMEBASE_gp);
    ADC0.CTRLE = ADC_SAMPLE_DURATION;
    ADC0.CTRLF = ADC_SAMPLE_NUM;
    ADC_PGA_GAIN(adc_gain); // Enable the gain amplifier. Will be used only for some measurements.
    ADC0.INTCTRL =  ADC_RESRDY_bm;

    ADC0.MUXPOS = adc_channels[adc_results.active_channel].muxpos;
    ADC0.MUXNEG = adc_channels[adc_results.active_channel].muxneg;
    ADC0.COMMAND = ADC_MODE_BURST_gc | adc_channels[adc_results.active_channel].diff;
}

void adc_start(void) {
    ADC0.COMMAND |= ADC_START_IMMEDIATE_gc;
    adc_results.new_measurement = false;
}

void adc_stop(void) {
    ADC0.COMMAND &= ADC_START_STOP_gc;
}

bool adc_get_measurements(adc_measurement_t **meas) {
    if(!adc_results.new_measurement) {
        return false;
    }

    for(uint8_t i = 0; i < ADC_CHANNELS_NUM; i++) {
        adc_results.meas.raw[i] = adc_results.adc[i] >> 2;
        if(adc_channels[i].muxneg & ADC_VIA_PGA_gc) {
            uint32_t abs_value = ABS(adc_results.adc[i]);
            adc_results.meas.raw[i] /= (1 << adc_gain);

            if(abs_value > 0x7000) {
                adc_gain = (adc_gain > 0) ? (adc_gain - 1) : adc_gain;
            } else if(abs_value < 0x1000) {
                adc_gain = (adc_gain < ADC_GAIN_MAX - 1) ? (adc_gain + 1) : adc_gain;
            }

            ADC_PGA_GAIN(adc_gain);
        }
    }

    *meas = &(adc_results.meas.ch);
    return true;
}

adc_gain_e adc_get_gain(void) {
    return adc_gain;
}

uint16_t adc_to_temperature(uint16_t value) {
    int8_t sigrow_offset = SIGROW.TEMPSENSE1; // Read signed offset from signature row
    uint8_t sigrow_gain = SIGROW.TEMPSENSE0; // Read unsigned gain/slope from signature row
    value >>= 2; // 10-bit MSb of ADC result with 1.024V internal reference
    uint32_t temp = value - sigrow_offset;
    temp *= sigrow_gain; // Result might overflow 16-bit variable (10-bit + 8- bit)
    temp += 0x80; // Add 256/2 to get correct integer rounding on division below
    temp >>= 8; // Divide result by 256 to get processed temperature in
    return temp >> 2;
}
