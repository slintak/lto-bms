#ifndef __BMS_ADC_H__
#define __BMS_ADC_H__

#include <stdlib.h>
#include <stdbool.h>

#define SHUNT_RESISTOR   (0.1) // Ohm
#define VOLTAGE_DIVIDER  (0.5/1.5)

#if defined(CELL_NAION)
#define ADC_REF_MV 2048.0
#elif defined(CELL_LTO)
#define ADC_REF_MV 1024.0
#else
#error "CELL_LTO or CELL_NAION must be defined"
#endif

#define ADC_CURRENT_CONSTANT ((ADC_REF_MV / SHUNT_RESISTOR) / 8192.0)
#define ADC_VOLTAGE_CONSTANT (ADC_REF_MV / (VOLTAGE_DIVIDER * 16384.0))
#define ADC_TO_VOLTAGE(a) (uint32_t)((float)(a) * (ADC_VOLTAGE_CONSTANT))
#define ADC_TO_CURRENT(a) (int32_t)((float)(a) * (ADC_CURRENT_CONSTANT))

#define ADC_CHANNELS_NUM     4

typedef enum {
    ADC_GAIN_1X = 0,
    ADC_GAIN_2X = 1,
    ADC_GAIN_4X = 2,
    ADC_GAIN_8X = 3,
    ADC_GAIN_16X = 4,
    ADC_GAIN_MAX = 5,
} adc_gain_e;

typedef struct {
    int32_t current;
    uint32_t battery;
    uint32_t load;
    uint32_t temperature;
} adc_measurement_t;

typedef struct {
    bool new_measurement;
    uint8_t active_channel;
    int32_t adc[ADC_CHANNELS_NUM];
    union {
        int32_t raw[ADC_CHANNELS_NUM];
        adc_measurement_t ch;
    } meas;
} adc_result_t;

typedef struct {
    uint8_t muxpos;
    uint8_t muxneg;
    uint8_t diff;
} adc_channels_t;

void adc_init(void);
void adc_start(void);
void adc_stop(void);
bool adc_get_measurements(adc_measurement_t **meas);
uint16_t adc_to_temperature(uint16_t value);

adc_gain_e adc_get_gain(void);

#endif
