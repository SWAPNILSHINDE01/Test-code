Here is a descriptive summary for your **DestnoMeter** project on GitHub, based on the provided implementation details and sensor logic.

**DestnoMeter: High-Precision Digital Densitometer**

**DestnoMeter** is an STM32-based digital densitometer and light meter designed to provide high-dynamic-range illuminance measurements. By utilizing the **TSL2591** light-to-digital converter and an **SH1107** OLED display, the system delivers real-time Lux readings with a historical data tracking feature.

---

**Core Functionality**
*   **Dual-Channel Sensing**: Utilizes two photodiodes to measure both Visible + Infrared (Channel 0) and pure Infrared (Channel 1) spectrums.
*   **Real-Time Lux Calculation**: Implements an improved physics-based formula to isolate visible light from infrared noise, providing accurate illuminance readings.
*   **Smart History Tracking**: Features a rolling history buffer that stores and displays previous readings, allowing the user to compare light levels over time.
*   **Dynamic OLED Interface**: Uses a custom-scaled font to display current readings in a large, readable format while keeping historical data visible in secondary slots.

---

**Technical Implementation**

**1. Sensor Logic (TSL2591)**
The firmware initializes the sensor with specific hardware parameters to balance sensitivity and range:
*   **Gain**: Set to **Medium (25x)** for general-purpose lighting conditions.
*   **Integration Time**: Configured for **100ms** per sample.
*   **Saturation Protection**: Includes logic to detect and report "Out of Range" errors if the light exceeds the sensor's physical limits (0xFFFF).

 **2. Data Processing**
The calculation engine converts raw digital counts into Lux using a calculated **Counts Per Lux (CPL)** factor:
$$lux = \frac{(CH0 - CH1) \times (1.0 - \frac{CH1}{CH0})}{CPL}$$

**3. Firmware Architecture**
*   **MCU**: STM32 (F3 Series recommended based on configuration).
*   **Communication**: I2C protocol at 100kHz for sensor and display interfacing.
*   **Logging**: Full debug output via UART (38400 baud) for monitoring raw spectrum data and calculated Lux values.

---

**Hardware Requirements**
*   **Microcontroller**: STM32 Nucleo or equivalent.
*   **Sensor**: Adafruit TSL2591 High Dynamic Range Digital Light Sensor.
*   **Display**: SH1107 128x128 I2C OLED.
