/*
 * tsl2591.h
 *
 *  Created on: May 6, 2026
 *      Author: SwapnilShinde
 */

#ifndef INC_TSL2591_H_
#define INC_TSL2591_H_

#include "main.h"

#define TSL2591_ADDR          (0x29 << 1)
#define TSL2591_COMMAND_BIT   0xA0
#define TSL2591_ENABLE_REG    0x00
#define TSL2591_CONFIG_REG    0x01
#define TSL2591_ID_REG        0x12
#define TSL2591_C0DATA_L      0x14
#define TSL2591_C1DATA_L      0x16
#define FLASH_STORAGE_ADDRESS  0x0801FC00

// Enable Register Bits
#define TSL2591_ENABLE_POWEROFF     0x00        // Turns off completely (0.5 uA)
#define TSL2591_ENABLE_POWERON 0x01
#define TSL2591_ENABLE_AEN     0x02
/*
 * ALS gain sets the gain of the internal integration amplifiers for both
photodiode channels.
*/
#define TSL2591_GAIN_LOW       0x00
#define TSL2591_GAIN_MED 	   0x01
#define TSL2591_GAIN_HIGH      0x10
#define TSL2591_GAIN_MAXHIGH   0x11

/*
 * ALS time sets the internal ADC integration time for both
photodiode channels.
 */
#define TSL2591_INTEGRATIONTIME_100MS 0x000
#define TSL2591_INTEGRATIONTIME_200MS 0x001
#define TSL2591_INTEGRATIONTIME_300MS 0x010
#define TSL2591_INTEGRATIONTIME_400MS 0x011
#define TSL2591_INTEGRATIONTIME_500MS 0x100
#define TSL2591_INTEGRATIONTIME_600MS 0x101

HAL_StatusTypeDef TSL2591_Init(I2C_HandleTypeDef *hi2c);
void TSL2591_ReadData(I2C_HandleTypeDef *hi2c, uint16_t *ch0, uint16_t *ch1);
float TSL2591_CalculateLux(uint16_t ch0, uint16_t ch1); // Add this prototype!
float TSL2591_GetOpticalDensity(float current_lux);
float TSL2591_Calibrate(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef TSL2591_Sleep(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef TSL2591_Wakeup(I2C_HandleTypeDef *hi2c);

#endif /* INC_TSL2591_H_ */
