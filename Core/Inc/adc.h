/*
 * adc.h
 *
 *  Created on: May 8, 2026
 *      Author: SwapnilShinde
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_


/* Voltage Reading Functions Prototypes */
uint32_t get_mcu_vdda(void);
uint32_t read_battery_voltage(void);
int get_battery_percentage(void);
#endif /* INC_ADC_H_ */
