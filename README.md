# LCD Arabic Displayer

Simple Arduino project to display **Arabic** text on a 16x2 I2C LCD using the `LiquidCrystalArabic` library. [web:5]

## Overview

- Displays Arabic sentences on an HD44780 16x2 LCD via I2C. [web:52]
- Handles right-to-left text and Arabic character shaping using `LiquidCrystalArabic_I2C`. [attached_file:1]
- Suitable for small demos like Quran verses or general Arabic messages. [attached_file:1]

## Hardware

- Arduino-compatible board (e.g., Uno / Nano). [attached_file:1]
- 16x2 HD44780-based LCD with I2C backpack (e.g., address `0x27` or `0x3F`). [web:52]
- Jumper wires and USB cable for programming. [web:52]

## Software Setup

1. Install the Arduino IDE. [web:47]
2. From **Library Manager**, install:
   - `LiquidCrystal I2C` by Frank de Brabander (base I2C LCD library). [web:46][web:52]
   - `LiquidCrystalArabic` from this repository:  
     https://github.com/balawi28/LiquidCrystalArabic [web:5][attached_file:1]
3. Clone or download this repository.
4. Open the main `.ino` file, select your board and port, then upload. [web:47]

## How It Works

- Uses `#include <Wire.h>` and `#include "LiquidCrystalArabic_I2C.h"`. [attached_file:1]
- Creates the LCD object, for example:  
  `LiquidCrystalArabic lcd(0x27, 16, 2);` [attached_file:1]
- In `setup()`, initializes the display and backlight, then prints Arabic text with:  
  `lcd.printArabic("نص عربي", false, true);` [attached_file:1]
- Uses `setCursorArabic(x, y)` to position text correctly for right‑to‑left rendering. [attached_file:1]

## Acknowledgements

This project uses the **LiquidCrystalArabic** library by **balawi28**, which enables Arabic script (RTL, shaping, and ligatures) on HD44780 LCDs.  
Repository: https://github.com/balawi28/LiquidCrystalArabic [web:5][attached_file:1]
