#ifndef __BMS_GPIO_H__
#define __BMS_GPIO_H__

#define BMS_ADC_SP   ADC_MUXPOS_AIN1_gc  // PA1
#define BMS_ADC_BATT ADC_MUXPOS_AIN2_gc  // PA2
#define BMS_ADC_OUT  ADC_MUXPOS_AIN3_gc  // PA3
#define BMS_CUTOFF   PIN4_bm
#define BMS_CUTOFF_3V3 PIN5_bm           // PA5
#define BMS_ADC_BN   ADC_MUXNEG_AIN6_gc  // PA6
#define BMS_ADC_BP   ADC_MUXPOS_AIN7_gc  // PA7
#define BMS_IIC_SCL  PIN0_bm  // PB0
#define BMS_IIC_SDA  PIN1_bm  // PB1
#define BMS_UART_TX  PIN2_bm  // PB2
#define BMS_TP9      PIN3_bm  // PB3
#define BMS_ADC_TEMPSENSE ADC_MUXPOS_TEMPSENSE_gc

// Turn on or off the main BMS output on the screw terminal.
#define BMS_CUTOFF_ON() do { PORTA.DIRCLR |= BMS_CUTOFF; } while(0)
#define BMS_CUTOFF_OFF() do { PORTA.DIRSET |= BMS_CUTOFF; PORTA.OUTSET = BMS_CUTOFF; } while(0)

// Turn on or off the 3.3 V output on the QWIIC connector.
#define BMS_CUTOFF_3V3_ON() do { PORTA.DIRCLR |= BMS_CUTOFF_3V3; } while(0)
#define BMS_CUTOFF_3V3_OFF() do { PORTA.DIRSET |= BMS_CUTOFF_3V3; PORTA.OUTCLR = BMS_CUTOFF_3V3; } while(0)

#define HTONS(A) ((((uint16_t)(A) & 0xff00) >> 8) | (((uint16_t)(A) & 0x00ff) << 8))

#endif
