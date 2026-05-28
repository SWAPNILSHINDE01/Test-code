/*
 * ADC.c
 *
 *  Created on: May 8, 2026
 *      Author: SwapnilShinde
 */

#include <main.h>
#include <string.h>
#include "adc.h"

// Factory Calibration Value Address for STM32G0 (check datasheet for exact address)
// For STM32G030, VREFINT_CAL is usually at 0x1FFF75AA
//#define VREFINT_CAL_ADDR ((uint16_t*) ((uint32_t)0x1FFF75AA))
//#define VREFINT_CAL_VREF 3000 // The calibration was done at 3.0V (3000mV)

extern ADC_HandleTypeDef hadc3;
extern void UART_Log(char* message);

// Factory Calibration Value Address for STM32F303RE
// This value represents what the ADC reads for 1.2V when VDDA is exactly 3.3V
#define VREFINT_CAL_ADDR ((uint16_t*) ((uint32_t)0x1FFFF7BA))

uint32_t get_mcu_vdda(void)
{
    uint32_t vref_sum = 0;
    ADC_ChannelConfTypeDef sConfig = {0};

    // Configure ADC to read the Internal Reference Channel
    sConfig.Channel = ADC_CHANNEL_VREFINT;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_601CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;

    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK) return 3300;

    // Average 16 samples for stability
    for(int i = 0; i < 32; i++)
    {
        HAL_ADC_Start(&hadc3);
        if(HAL_ADC_PollForConversion(&hadc3, 10) == HAL_OK)
        {
            vref_sum += HAL_ADC_GetValue(&hadc3);
        }
        HAL_ADC_Stop(&hadc3);
    }

    uint32_t vref_avg = vref_sum / 32;
    if (vref_avg == 0) return 3300;

    // Calculate actual supply voltage based on the 3.3V factory calibration
    return (3300 * (*VREFINT_CAL_ADDR)) / vref_avg;
}


int percentage_return(void)
{
    int battery_percent = 0;
    char adc_buf[150];
    uint32_t ad_sum = 0;
    UART_Log("BATTERY");

    // 1. Get the true runtime VDD/VDDA reference voltage first
    uint32_t true_vdda = get_mcu_vdda();

    // 2. CRITICAL: Switch ADC back to your External Battery Pin (Channel 1)
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_601CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;

    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
    {
        UART_Log("ADC Channel Switch Failed!\r\n");
        return 0;
    }

    // 3. Sample Channel 1 safely 32 times
    for(int i = 0; i < 32; i++)
    {
        HAL_ADC_Start(&hadc3);
        if(HAL_ADC_PollForConversion(&hadc3, 10) == HAL_OK)
        {
            ad_sum += HAL_ADC_GetValue(&hadc3);
        }
    }
    HAL_ADC_Stop(&hadc3);

    uint32_t ad_avg = ad_sum / 32;

    // 4. Calculate real voltage (mV) present on ADC1 Channel 1 using dynamic VDD
    uint32_t battery_channel_mv = ((ad_avg * true_vdda) / 4095);

    // 5. External Battery Percentage Calculation (Scale: 2.7V - 3.3V)
    int v_min = 2700;
    int v_max = 3300;

    if (battery_channel_mv <= v_min)       battery_percent = 0;
    else if (battery_channel_mv >= v_max)  battery_percent = 100;
    else {
        battery_percent = (int)(((battery_channel_mv - v_min) * 100) / (v_max - v_min));
    }

    // 6. Split values for printing formatting
    uint32_t vdd_whole = true_vdda / 1000;
    uint32_t vdd_frac  = true_vdda % 1000;

    uint32_t bat_whole = battery_channel_mv / 1000;
    uint32_t bat_frac  = battery_channel_mv % 1000;

    // 7. Complete single log output containing all 4 requested metrics
    sprintf(adc_buf, "--- ADC1 Channel 1 Function ---\r\n"
                     "1. ADC1_Ch1 Raw Steps: %lu\r\n"
                     "2. Calculated VDD Ref:  %lu.%03lu V\r\n"
                     "3. Pin Ch1 True Volts: %lu.%03lu V\r\n"
                     "4. Battery Percentage: %d%%\r\n\r\n",
            ad_avg, vdd_whole, vdd_frac, bat_whole, bat_frac, battery_percent);

    UART_Log(adc_buf);

    return battery_percent;
}
