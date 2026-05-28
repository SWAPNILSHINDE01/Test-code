/*
 * sh1107.h
 *
 *  Created on: May 6, 2026
 *      Author: SwapnilShinde
 */

#ifndef INC_SH1107_H_
#define INC_SH1107_H_

#include "main.h"

extern I2C_HandleTypeDef hi2c3;
#define SH1107_I2C_ADDR         (0x3C << 1)
#define SH1107_WIDTH            64
#define SH1107_HEIGHT           128

typedef enum {
	Black = 0x00,
	White = 0x01
} SH1107_COLOR;

void sh1107_DrawPixel(uint8_t x, uint8_t y, SH1107_COLOR color);
void sh1107_DisplayCustomData(uint8_t data[5]);
void sh1107_DisplayCustomData_1(uint8_t data[5]);
void sh1107_DisplayCustomData_2(uint8_t data[5]);
uint8_t sh1107_Init(void);
void sh1107_Fill(SH1107_COLOR color);
void sh1107_UpdateScreen(void);
void sh1107_WriteCommand(uint8_t command);
void sh1107_DrawPixel(uint8_t x, uint8_t y, SH1107_COLOR color);
void sh1107_DisplayUnit(void);
void sh1107_DrawGiantChar(uint8_t x, uint8_t y, const uint8_t font_ptr[5], uint8_t scaleX, uint8_t scaleY);
void SH1107_DrawVerticalLine(uint16_t x, uint16_t y_start, uint16_t y_end, uint16_t color);
void sh1107_DisplayFirstScreen();
void sh1107_DrawProgressBar(uint8_t percent);
void sh1107_DisplayCallibrationScreen();
void sh1107_SetBrightness(uint8_t level);
void sh1107_EndDisplay();
void sh1107_DisplayVoltage(uint32_t voltage_mv, uint8_t x, uint8_t y);
HAL_StatusTypeDef sh1107_Sleep();
HAL_StatusTypeDef sh1107_Wakeup();

#endif /* INC_SH1107_H_ */
