/*
 * sh1107.c
 *
 *  Created on: May 6, 2026
 *      Author: SwapnilShinde
 */

#include "sh1107.h"
#include "main.h"
#include <string.h>

/**
 *  @briefInternal Buffer: 1024 bytes (64 width * 128 height / 8 bits)
 *  What it does: Creates a "Virtual Screen" in the STM32's RAM.
 *  Why: You cannot read data back from the OLED easily. We draw everything in this buffer first,
    then send the whole thing at once.
 */
extern restart_counter;
static uint8_t SH1107_Buffer[SH1107_WIDTH * SH1107_HEIGHT / 8];

/**
 * @brief 11x5  Font subset for "(0-9)" - Used as a seed for scaling
 * What it does: This is a Lookup Table.
 * The Math: Each number (0–9) is 5 pixels wide. Each byte represents one vertical column of 8 pixels.
*/

const uint8_t BigDigits[11][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x60, 0x60, 0x00, 0x00}  // . (Index 10)
};

const uint8_t Alphabet[26][5] = {
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

const uint8_t Alphabet_Small[27][5] = {
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44},  // z
	{0x54, 0x00, 0x04, 0x00, 0x08}  // dimmed r
};

const uint8_t Symbol[10][5] = {
{0x10, 0x10, 0x10, 0x10, 0x10}, // Hyphen (-)
{0x00, 0x00, 0x5F, 0x00, 0x00} // !
//{0x00, 0x60, 0x60, 0x00, 0x00}  //This will be add on a after correct version . (Index 10)
};

/**
 * @brief Sends a command byte to the SH1107
 What it does: Sends a single "Instruction" to the display controller.
 Address 0x00: This tells the SH1107, "The next byte is a command (like 'Set Brightness'), not pixel data."
 */
void sh1107_WriteCommand(uint8_t command) {
    HAL_I2C_Mem_Write(&hi2c3, SH1107_I2C_ADDR, 0x00, 1, &command, 1, HAL_MAX_DELAY);
}

HAL_StatusTypeDef sh1107_Sleep() {
    HAL_I2C_Mem_Write(&hi2c3, SH1107_I2C_ADDR, 0x00, 1, 0xAE, 1, HAL_MAX_DELAY);
    return HAL_OK;
}

HAL_StatusTypeDef sh1107_Wakeup() {
    HAL_I2C_Mem_Write(&hi2c3, SH1107_I2C_ADDR, 0x00, 1, 0xAF, 1, HAL_MAX_DELAY);
    return HAL_OK;
}

/**
 * @brief Initializes the SH1107 and clears "garbage" RAM
 */
uint8_t sh1107_Init(void) {
    HAL_Delay(100); // Wait for screen stable

    sh1107_WriteCommand(0xAE); // Display OFF

    sh1107_WriteCommand(0x20);  // Set Page Addressing Mode. Tells the screen to organize its memory into 16 horizontal rows (Pages).

/** @brief
 * Crucial: This tells the chip that the physical glass starts at index 96 (0x60). Without this, your image is shifted or cut off.

    // OFFSET FIX: 0x60 is standard for GME64128 modules.
    // This moves the viewable area to the physical glass.
 */
    sh1107_WriteCommand(0xD3);
    sh1107_WriteCommand(0x60);

    sh1107_WriteCommand(0xDC); // Start Line
    sh1107_WriteCommand(0x00);
/**
 * @brief
 *  Orientation                   Segment        RemapCOM
 *  ScanCurrent (Mirrored)         0xA0             0xC0
 *  Flipped                        X0xA1            0xC0
 *  Flipped                        Y0xA0            0xC8
 *  Fully Rotated 180°             0xA1             0xC8

 Rotation: These two lines act like "Mirror" and "Flip." 0xA1 flips the X-axis so the text isn't backwards.
 */
    sh1107_WriteCommand(0xA1); // Segment Remap (X-Mirror)
    sh1107_WriteCommand(0xC0); // COM Scan Direction (Y-Mirror)  FOR CHANE THE DISPLAY ROATATION

    sh1107_WriteCommand(0xA8); // Multiplex Ratio
    sh1107_WriteCommand(0x7F); // 128 (Full Height)

    sh1107_WriteCommand(0x81); // Contrast
    sh1107_WriteCommand(0xFF);

    sh1107_WriteCommand(0xAD); // DC-DC ON
    sh1107_WriteCommand(0x8B);

    // Set Display Clock Divide Ratio/Oscillator Frequency
    sh1107_WriteCommand(0xD5);
    sh1107_WriteCommand(0x50); // Standard frequency

    // Set Pre-charge Period
    sh1107_WriteCommand(0xD9);
    sh1107_WriteCommand(0x22); // Default stable value

    // Set VCOMH Deselect Level
    sh1107_WriteCommand(0xDB);
    sh1107_WriteCommand(0x35); // Stable voltage level

    sh1107_WriteCommand(0xA4); // Normal Display
    sh1107_WriteCommand(0xA6); // Non-inverted

    // CRITICAL: Clear buffer and update RAM before turning display ON
    // This prevents the "garbage" static from appearing
    sh1107_Fill(Black);
    sh1107_UpdateScreen();

    sh1107_WriteCommand(0xAF); // Display ON

    return 0;
}

/*
 * used when we adjust a brightness of display SH1107
 */
void sh1107_SetBrightness(uint8_t level) {
    sh1107_WriteCommand(0x81); // Contrast Control Command
    sh1107_WriteCommand(level); // 0 to 255 (0 is dimmest, 255 is brightest)
}

/*
 * used when a we want a entire screen bloack or whilte
 * 	Black = 0x00,
 * 	White = 0x01
 */
void sh1107_Fill(SH1107_COLOR color) {
    memset(SH1107_Buffer, (color == White) ? 0xFF : 0x00, sizeof(SH1107_Buffer));
}

/**
 * @brief Low-level pixel drawing with boundary check
 * The Translation: Converts a 2D coordinate $(x, y)$ into a specific bit inside your 1024-byte array.y / 8: Finds which Page (row of 8 pixels) the dot belongs to.y % 8:
 * Finds which specific bit (0–7) inside that byte needs to turn on.
 */
void sh1107_DrawPixel(uint8_t x, uint8_t y, SH1107_COLOR color) {
    if(x >= SH1107_WIDTH || y >= SH1107_HEIGHT) return;

    if(color == White)
        SH1107_Buffer[x + (y / 8) * SH1107_WIDTH] |= (1 << (y % 8));
    else
        SH1107_Buffer[x + (y / 8) * SH1107_WIDTH] &= ~(1 << (y % 8));
}

/*
 * used when a we want a seperate a data current data and historical data.
 */
// change SH to sh
void SH1107_DrawVerticalLine(uint16_t x, uint16_t y_start, uint16_t y_end, uint16_t color) {    // Ensure coordinates are within bounds
    if (x >= 64) return;
    // Sort y_start and y_end if they are passed in reverse
        if (y_start > y_end) {
            uint16_t temp = y_start;
            y_start = y_end;
            y_end = temp;
        }

        // Loop through Y axis and set pixels
        for (uint16_t y = y_start; y <= y_end; y++) {
            if (y < 128) {
            	sh1107_DrawPixel(x, y, color);
            }
        }
}

/**
 * @brief Draws a single big digit
 *
 * Scaling: If a bit is found, it doesn't draw 1 pixel; it draws a rectangle of pixels of size scaleX by scaleY.
 * Rotation Mapping: Notice sh1107_DrawPixel(y + ..., x + ...).
 * X and Y are swapped here to turn the text 90 degrees so it fits the tall screen.
 */
//change Giant to big
void sh1107_DrawGiantDigit(uint8_t x, uint8_t y, uint8_t idx, uint8_t scaleX, uint8_t scaleY) {
    for (int col = 0; col < 5; col++) {
        uint8_t data = BigDigits[idx][col];
        for (int sx = 0; sx < scaleX; sx++) {
            for (int bit = 0; bit < 8; bit++) {
                if ((data >> bit) & 0x01) {
                    for (int sy = 0; sy < scaleY; sy++) {
                        // Rotation mapping: X is the 128 side, Y is the 64 side
                        sh1107_DrawPixel(y + (bit * scaleY) + sy, x + (col * scaleX) + sx, White);
                    }
                }
            }
        }
    }
}

/*
 * Draw a single character using Updated function to accept ANY 5-column font array.
 */
//giant to bit_alphabet
void sh1107_DrawGiantChar(uint8_t x, uint8_t y, const uint8_t font_ptr[5], uint8_t scaleX, uint8_t scaleY) {
    for (int col = 0; col < 5; col++) {
        uint8_t data = font_ptr[col]; // Access the passed pointer directly
        for (int sx = 0; sx < scaleX; sx++) {
            for (int bit = 0; bit < 8; bit++) {
                if ((data >> bit) & 0x01) {
                    for (int sy = 0; sy < scaleY; sy++) {
                        sh1107_DrawPixel(y + (bit * scaleY) + sy, x + (col * scaleX) + sx, White);
                    }
                }
            }
        }
    }
}

/*
 * for a display to a current reading data.
 */
void sh1107_DisplayCustomData(uint8_t data[5])
		{
    uint8_t sX = 4;  // Horizontal stretch per column (5 * 5 columns = 25px per digit)
    uint8_t sY = 4;  // Vertical stretch per bit (8 * 7 bits = 56px height)
    uint8_t vertical_margin = 8  ;    //Vertical Margin: Moves the text "down" the screen by 25 pixels from the top edge
    uint8_t Horizontal_margin = 0 ;

//	Positions the first digit at X=2. The next digits are placed at 28, 54, 74, 100 to create even spacing.

	    sh1107_DrawGiantDigit(Horizontal_margin + 10,   vertical_margin, data[0], sX, sY); // Tens
	    sh1107_DrawGiantDigit(Horizontal_margin + 30,  vertical_margin, 10, 4, sY); // Ones
	    sh1107_DrawGiantDigit(Horizontal_margin + 50,  vertical_margin, data[1],      sX,  sY); // Dot/Colon (Thinner)
	    sh1107_DrawGiantDigit(Horizontal_margin + 75,  vertical_margin, data[2], sX, sY); // Tenths
}

/*
 * for a display to a first historical data.
*/
void sh1107_DisplayCustomData_1(uint8_t data[5])
{
    uint8_t sX = 2;  // Horizontal stretch per column (5 * 5 columns = 25px per digit)
    uint8_t sY = 2;  // Vertical stretch per bit (8 * 7 bits = 56px height)
    uint8_t vertical_margin = 50;    //Vertical Margin: Moves the text "down" the screen by 25 pixels from the top edge
    uint8_t Horizontal_margin = 0 ;

//	Positions the first digit at X=2. The next digits are placed at 28, 54, 74, 100 to create even spacing.

	    sh1107_DrawGiantDigit(Horizontal_margin + 2,   vertical_margin, data[0], sX, sY); // Tens
	    sh1107_DrawGiantDigit(Horizontal_margin + 14 ,  vertical_margin, 10, 2, sY); // Ones
	    sh1107_DrawGiantDigit(Horizontal_margin + 25,  vertical_margin, data[1], sX,  sY); // Dot/Colon (Thinner)
	    sh1107_DrawGiantDigit(Horizontal_margin + 40,  vertical_margin, data[2], sX, sY); // Tenths
}

/*
 * for a display to second historical data.
*/
void sh1107_DisplayCustomData_2(uint8_t data[5])
{
    uint8_t sX = 2;  // Horizontal stretch per column (5 * 5 columns = 25px per digit)
    uint8_t sY = 2;  // Vertical stretch per bit (8 * 7 bits = 56px height)
    uint8_t vertical_margin = 50 ;    //Vertical Margin: Moves the text "down" the screen by 25 pixels from the top edge
    uint8_t Horizontal_margin = 0 ;

//	Positions the first digit at X=2. The next digits are placed at 28, 54, 74, 100 to create even spacing.

	    sh1107_DrawGiantDigit(Horizontal_margin + 70,   vertical_margin, data[0], sX, sY); // Tens
	    sh1107_DrawGiantDigit(Horizontal_margin + 84,  vertical_margin, 10, 2 , sY); // Ones
	    sh1107_DrawGiantDigit(Horizontal_margin + 95,  vertical_margin, data[1], sX,  sY); // Dot/Colon (Thinner)
	    sh1107_DrawGiantDigit(Horizontal_margin +110,  vertical_margin, data[2], sX, sY); // Tenths

}

/*
 * This function are used for display a incertech.
 */
void sh1107_DisplayFirstScreen()
{
	uint8_t sX = 2;  // Horizontal stretch per column (5 * 5 columns = 25px per digit)
	uint8_t sY = 4;  // Vertical stretch per bit (8 * 7 bits = 56px height)
	uint8_t vertical_margin = 20 ;    //Vertical Margin: Moves the text "down" the screen by 25 pixels from the top edge
	uint8_t Horizontal_margin = 0 ;
	sh1107_DrawGiantChar(Horizontal_margin + 5,vertical_margin, Alphabet_Small[8], sX, sY); // i
	sh1107_DrawGiantChar(Horizontal_margin + 18,vertical_margin, Alphabet_Small[13], sX, sY); // n
	sh1107_DrawGiantChar(Horizontal_margin + 31,vertical_margin, Alphabet_Small[2], sX, sY); // c
	sh1107_DrawGiantChar(Horizontal_margin + 44,vertical_margin, Alphabet_Small[4], sX, sY); // e
	sh1107_DrawGiantChar(Horizontal_margin + 57,vertical_margin, Alphabet_Small[26], sX, sY); // r
	sh1107_DrawGiantChar(Horizontal_margin + 70,vertical_margin, Alphabet_Small[19], sX, sY); // t
	sh1107_DrawGiantChar(Horizontal_margin + 82,vertical_margin, Alphabet_Small[4], sX, sY); // e
	sh1107_DrawGiantChar(Horizontal_margin + 96,vertical_margin, Alphabet_Small[2], sX, sY); // c
	sh1107_DrawGiantChar(Horizontal_margin + 107,vertical_margin, Alphabet_Small[7], sX, sY); // h


}

/*
 *  This function are used to a shown a cal in display screen.
 */
void sh1107_DisplayCallibrationScreen()
{
	uint8_t sX = 5;  // Horizontal stretch per column (5 * 5 columns = 25px per digit)
	uint8_t sY = 5;  // Vertical stretch per bit (8 * 7 bits = 56px height)
	uint8_t vertical_margin = 15 ;    //Vertical Margin: Moves the text "down" the screen by 25 pixels from the top edge
	uint8_t Horizontal_margin = 15 ;
	sh1107_DrawGiantChar(Horizontal_margin ,vertical_margin, Alphabet_Small[2], sX, sY); // c
	sh1107_DrawGiantChar(Horizontal_margin + 30,vertical_margin, Alphabet_Small[0], sX, sY); // a
	sh1107_DrawGiantChar(Horizontal_margin + 60,vertical_margin, Alphabet_Small[11], sX, sY); //  l
//	sh1107_DrawGiantChar(Horizontal_margin + 80,vertical_margin, Symbol[1], sX, sY); //!
}

/**
 * @brief Draws a framed progress bar
 * @param percent: 0 to 100
 * @param y_page: The vertical page (0-15) where you want the bar (Suggested: 14)
 */

void sh1107_DrawProgressBar(uint8_t percent) {
	/**
	 * @brief Draws a horizontally aligned battery in the center of the screen
	 * @param percent: 0 to 100
	 **/

	    // 1. Position and Size Constants
	        const uint8_t centerY = 120;   // Middle of 64px height
	        const uint8_t width   = 20;   // Length of battery body
	        const uint8_t height  = 8;   // Thickness of battery body
	        const uint8_t leftX   = 10;   // Start position on X axis

	        uint8_t rightX  = leftX + width;
	        uint8_t topY    = centerY - (height / 2);
	        uint8_t bottomY = centerY + (height / 2);

	        // 1. Position and Size Constants
	            static uint32_t lastToggleTick = 0;  // Stores the last time we "blinked"
	            static uint8_t isVisible = 1;        // Stores current visibility state
	            uint32_t currentTick = HAL_GetTick();
	            int blink_value ;

	        // --- BLINK LOGIC ---
	            if (percent < 25 ) {
	            	if (percent <15 || (restart_counter < 5))
	            	{
	            		blink_value = 300;
	            	}
	            	else
	            	{
	            		blink_value= 1000;
	            	}
	                // Toggle state every 300ms
	                if (currentTick - lastToggleTick >= blink_value) {
	                    isVisible = !isVisible;    // Switch between 0 and 1
	                    lastToggleTick = currentTick; // Reset the timer
	                }
	            } else {
	                isVisible = 1; // Always show if battery is healthy
	            }

	            // If we are in the "hide" phase of the blink, erase and exit
	            if (!isVisible) {
	                for (uint8_t x = leftX - 2; x <= rightX; x++) {
	                    for (uint8_t y = topY; y <= bottomY; y++) {
	                        sh1107_DrawPixel(x, y, Black);
	                    }
	                }
	                return; // Skip the drawing part
	            }


	        // 2. Draw the Main Body Outline
	        // Horizontal Walls (Top and Bottom)
	        for (uint8_t x = leftX; x <= rightX; x++) {
	            sh1107_DrawPixel(x, topY, White);
	            sh1107_DrawPixel(x, bottomY, White);
	        }
	        // Vertical Caps (Left and Right)
	        for (uint8_t y = topY; y <= bottomY; y++) {
	            sh1107_DrawPixel(leftX, y, White);  // Left wall
	            sh1107_DrawPixel(rightX, y, White); // Right wall
	        }

	        // 3. Draw the "Positive Terminal" (Small bump on the left side)
	        // Assuming the main battery body starts at leftX
	        for (uint8_t i = 119; i <= 122; i++)
	        {
	            // Draw 2 pixels wide to the left of the battery body
	            sh1107_DrawPixel(leftX - 1, i, White);
	            sh1107_DrawPixel(leftX - 2, i, White);
	        }


	        // 4. Draw the Horizontal Fill (Filling from left to right)
	        if (percent > 0) {
	            if (percent > 100) percent = 100;

	            // Calculate fill width (leaving a 2px internal margin)
	            uint8_t maxFillWidth = width - 10;
	            uint8_t fillPx = (percent * maxFillWidth) / 100;

	            // The right-most internal boundary
	                uint8_t rightInnerX = leftX + width - 3;
	                // The point where the fill stops (moving left)
	                uint8_t fillStartX = rightInnerX - fillPx;

	                for (uint8_t x = rightInnerX; x > maxFillWidth; x--) {
	                        for (uint8_t y = topY + 2; y <= bottomY - 2; y++) {
	                        	if(x > fillStartX)
	                        	{
	                            sh1107_DrawPixel(x, y, White);
	                        	}
	                        	else
	                        	{
	                        	sh1107_DrawPixel(x, y, Black);
	                        	}
	                        }
	                    }
	        }
}

/*
 * this function are used as screen refresh a data.
 */

/**
 * @brief Displays the numerical voltage and "V" symbol on the SH1107
 * @param voltage_mv: The voltage in millivolts (e.g., 3270)
 * @param x: Horizontal position (0-127)
 * @param y: Vertical position (0-63)
 */
void sh1107_DisplayVoltage(uint32_t voltage_mv, uint8_t x, uint8_t y) {
    uint8_t sX = 4; // Small scale for the voltage text
    uint8_t sY = 4;

    // Calculate digits: e.g., 3270mv -> 3, 2, 7
    uint8_t d1 = voltage_mv / 1000;          // Whole volts (3)
    uint8_t d2 = (voltage_mv % 1000) / 100;  // Tenths (2)
    uint8_t d3 = (voltage_mv % 100) / 10;    // Hundredths (7)

    // Draw Digit 1 (Whole Volts)
    sh1107_DrawGiantDigit(x, y, d1, sX, sY);

    // Draw Dot (Index 10 in BigDigits)
    sh1107_DrawGiantDigit(x + 7, y, 10, sX, sY);

    // Draw Digit 2 (Tenths)
    sh1107_DrawGiantDigit(x + 29, y, d2, sX, sY);

    // Draw Digit 3 (Hundredths)
    sh1107_DrawGiantDigit(x + 55, y, d3, sX, sY);

    // Draw "V" (Alphabet index 21 is 'V')
    sh1107_DrawGiantChar(x + 90, y, Alphabet[21], sX, sY);
}

void sh1107_UpdateScreen(void) {
    for (uint8_t i = 0; i < 16; i++) {
        sh1107_WriteCommand(0xB0 + i); // Set Page Address
        sh1107_WriteCommand(0x00);      // Set Lower Column Address
        sh1107_WriteCommand(0x10);      // Set Higher Column Address (0)
//		  The Data: Sends 64 bytes (one full horizontal line) to the screen.
//        Address 0x40: This tells the SH1107, "The following bytes are actual pixels to be displayed."
        HAL_I2C_Mem_Write(&hi2c3, SH1107_I2C_ADDR, 0x40, 1,
                         &SH1107_Buffer[i * SH1107_WIDTH], SH1107_WIDTH, HAL_MAX_DELAY);
    }
}


/*
 * for a display to a current reading data.
 */
void sh1107_EndDisplay()
		{
uint8_t sX = 5;  // Horizontal stretch per column (5 * 5 columns = 25px per digit)
uint8_t sY = 5;  // Vertical stretch per bit (8 * 7 bits = 56px height)
uint8_t vertical_margin = 20 ;    //Vertical Margin: Moves the text "down" the screen by 25 pixels from the top edge
uint8_t Horizontal_margin = 25 ;
sh1107_DrawGiantChar(Horizontal_margin + 0,vertical_margin, Alphabet[4], sX, sY); // e
sh1107_DrawGiantChar(Horizontal_margin + 30,vertical_margin, Alphabet[13], sX, sY); // n
sh1107_DrawGiantChar(Horizontal_margin + 60,vertical_margin, Alphabet[3], sX, sY); // d

}
