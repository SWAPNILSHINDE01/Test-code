/*
 * tsl2591.c
 *
 *  Created on: May 6, 2026
 *      Author: SwapnilShinde
 */

#include "tsl2591.h"
#include "main.h"
#include <math.h>
#include <string.h>
// Global or Static variable to store the reference light level
static float I0_Reference = 0.0f;

HAL_StatusTypeDef TSL2591_Init(I2C_HandleTypeDef *hi2c) {
/* 1. Sensor Setup
 * 1.1. Device Verification: It reads the ID_REG to ensure the connected device is a TSL2591.
 * (checking for the value 0x50).
 */
    uint8_t id;
    HAL_I2C_Mem_Read(hi2c, TSL2591_ADDR, TSL2591_COMMAND_BIT | TSL2591_ID_REG, 1, &id, 1, 100);

    if(id != 0x50) return HAL_ERROR;

/*  1.2. Power Control: It enables the sensor by setting the POWERON and AEN (ALS Enable)
 *  bits in the ENABLE_REG.
 */
    uint8_t enable = TSL2591_ENABLE_POWERON | TSL2591_ENABLE_AEN;
    HAL_I2C_Mem_Write(hi2c, TSL2591_ADDR, TSL2591_COMMAND_BIT | TSL2591_ENABLE_REG, 1, &enable, 1, 100);

/*
 *  1.3. Configuration: It sets the sensor to Medium Gain and a 100ms Integration Time by writing to the CONFIG_REG.
 */
    uint8_t config = TSL2591_GAIN_LOW | TSL2591_INTEGRATIONTIME_100MS;
    HAL_I2C_Mem_Write(hi2c, TSL2591_ADDR, TSL2591_COMMAND_BIT | TSL2591_CONFIG_REG, 1, &config, 1, 100);

    return HAL_OK;
}

/** Breif
2. Reading Raw Data (TSL2591_ReadData)
The TSL2591 has two photodiodes: Channel 0 (Visible + Infrared) and Channel 1 (Infrared only).
I2C Communication: The function uses HAL_I2C_Mem_Read to pull 16-bit values from the C0DATA and C1DATA registers.
Bit Shifting: Because the data is stored in low/high byte registers,
it combines them using bitwise operations:  (buf[1] << 8) | buf[0].
 */

void TSL2591_ReadData(I2C_HandleTypeDef *hi2c, uint16_t *ch0, uint16_t *ch1) {
    uint8_t buf[4];
    HAL_I2C_Mem_Read(hi2c, TSL2591_ADDR, TSL2591_COMMAND_BIT | TSL2591_C0DATA_L, 1, &buf[0], 2, 100);
    HAL_I2C_Mem_Read(hi2c, TSL2591_ADDR, TSL2591_COMMAND_BIT | TSL2591_C1DATA_L, 1, &buf[2], 2, 100);

    *ch0 = (buf[1] << 8) | buf[0];
    *ch1 = (buf[3] << 8) | buf[2];
}

/*
3. Lux Calculation (TSL2591_CalculateLux)This converts the raw digital "counts" into a human-readable Lux value:
Saturation Check: If either channel reads 0xFFFF (65535), the sensor is saturated (receiving too much light).
                    It returns -1.0f to signal an "out of range" error.
CPL (Counts Per Lux): This calculates a scaling factor based on the hardware settings:
    cpl = {atime * again}/{408.0}
    where atime is 100ms and again is 25x.
    The Formula: It subtracts the Infrared signal (CH1) from the Full Spectrum signal (CH0) to isolate visible light,
    then applies the scaling factor: lux = {(ch0 - ch1) * (1.0 - ( ch1} / ch0))}{cpl}.
    Negative Protection: If the result is negative (which can happen due to sensor noise in extremely low light),
    it returns 0.0f.
*/

float TSL2591_CalculateLux(uint16_t ch0, uint16_t ch1) {
    // 1. Check for overflow/saturation
	float ch0_f = (float)ch0;
	float ch1_f = (float)ch1;
    if (ch0 == 0xFFFF || ch1 == 0xFFFF) {
        return -1.0f; // Return -1 to indicate "Out of Range"
    }
    if (ch0 == 0) return 0.0f; // Prevent division by zero

    // 2. Integration time and Gain factors (Assumes 100ms and Medium Gain 25x)
    float atime = 100.0f;
    float again = 1.0f;

    // 3. Calculate CPL (Counts Per Lux)
    float cpl = (atime * again) / 408.0f;

    // 4. Improved Lux Formula
    // If CH0 is exactly CH1, we provide a tiny floor value to avoid 0.0
    float lux = (ch0_f - ch1_f) * (1.0f - (ch1_f / ch0_f)) / cpl;

    if (lux < 0) return 0.0f;
    return lux;
}


/**
 * @brief Calibrates the device by taking 10 readings of the light source (no film).
 * This sets the I0 value for OD calculations.
 */
float TSL2591_Calibrate(I2C_HandleTypeDef *hi2c) {
    uint16_t c0, c1;
    float sum = 0;
    int samples = 10;

    for (int i = 0; i < samples; i++) {
        TSL2591_ReadData(hi2c, &c0, &c1);
        sum += TSL2591_CalculateLux(c0, c1);
        HAL_Delay(210);
    }
    I0_Reference = sum / (float)samples;
//    // Save to permanent storage
//     TSL2591_Save_I0_To_Flash();
    char buf[64];
    int whole_number = (int)I0_Reference;
    int fractional_number = (int)((I0_Reference - (float)whole_number) * 100.0f);
    sprintf(buf, "Calibration Done. I0 Reference Value: %d.%02d\r\n",whole_number,fractional_number);
    UART_Log(buf);
    return I0_Reference;
}

/**
 * @brief Calculates Optical Density: OD = log10(I0 / I)
 */
float TSL2591_GetOpticalDensity(float current_lux) {
    if (I0_Reference <= 0.001f) return 0.0f; // Not calibrated
    if (current_lux <= 0.0001f) return 4.0f; // Absolute black (Max OD)
    if (current_lux >= I0_Reference) return 0.00f; // Light is brighter than reference

    // Using log10f for 32-bit float efficiency on STM32
    return log10f(I0_Reference / current_lux);
}

HAL_StatusTypeDef TSL2591_Sleep(I2C_HandleTypeDef *hi2c)
{
    uint8_t data = TSL2591_ENABLE_POWEROFF; // 0x00

    /* * In the TSL2591, you must OR the register address with the COMMAND_BIT (0xA0)
     * to tell the sensor's internal state machine that you are writing to a specific register.
     */
    uint8_t reg = TSL2591_COMMAND_BIT | TSL2591_ENABLE_REG;

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(hi2c,
    		TSL2591_ADDR,reg,
                                                 1,
                                                 &data,
                                                 1,
                                                 HAL_MAX_DELAY);
    return status;
}

HAL_StatusTypeDef TSL2591_Wakeup(I2C_HandleTypeDef *hi2c)
{
    uint8_t data = (TSL2591_ENABLE_POWERON | TSL2591_ENABLE_AEN); // 0x00

    /* * In the TSL2591, you must OR the register address with the COMMAND_BIT (0xA0)
     * to tell the sensor's internal state machine that you are writing to a specific register.
     */
    uint8_t reg = TSL2591_COMMAND_BIT | TSL2591_ENABLE_REG;

       HAL_StatusTypeDef status = HAL_I2C_Mem_Write(hi2c,
       		TSL2591_ADDR,reg,
                                                    1,
                                                    &data,
                                                    1,
                                                    HAL_MAX_DELAY);
    return status;
}
