# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Embedded firmware for an ATmega328P (Arduino framework) using PlatformIO with MiniCore. The device interfaces with Ethernet PHY chips via SPI and MDC/MDIO protocols to configure a managed gigabit switch/Ethernet device.

## Build & Upload Commands

```bash
# Build (default: Upload_UART environment)
pio run

# Build a specific environment
pio run -e Upload_UART
pio run -e Upload_ISP

# Upload via UART bootloader (ttyUSB0, requires urboot bootloader)
pio run -e Upload_UART -t upload

# Upload via Atmel ICE ISP programmer
pio run -e Upload_ISP -t upload

# Burn fuses only
pio run -e fuses_bootloader -t fuses

# Burn fuses + bootloader (urboot) via Atmel ICE
pio run -e fuses_bootloader -t bootloader

# Open serial monitor (57600 baud, port from Upload_UART)
pio device monitor

# Regenerate the clangd compilation database (needed for IDE IntelliSense/lint after
# changing includes or build flags). Output compile_commands.json is gitignored.
pio run -t compiledb
```

## Hardware Configuration

- **MCU**: ATmega328P at 8 MHz (external oscillator)
- **Bootloader**: urboot at 57600 baud
- **ISP Programmer**: Atmel ICE (USB)
- **Serial port**: `/dev/ttyUSB0`

## Code Architecture

The application in `src/main.cpp` wires together three controllers as file-scope globals and
delegates serial commands to them. All classes (`.h` + `.cpp`) live flat in `src/`:

- **`SpiController`** (`src/SpiController.{h,cpp}`): Manages SPI bus communication with PHY chips
- **`MdcMdioController`** (`src/MdcMdioController.{h,cpp}`): Bit-bangs the MDC (clock, A5/PC5) and MDIO (data, A4/PC4) bus for PHY register access; includes `initializeDualPhy()` for dual-PHY setup with patch registers
- **`Terminal`** (`src/Terminal.{h,cpp}`): Serial terminal (57600 baud) that accepts commands and delegates to the SPI/MDC controllers

The stock `include/` and `lib/` directories contain only PlatformIO's placeholder READMEs and are unused.

Style is enforced by `.clang-format` and `.editorconfig`: **tab indentation (width 4)**, K&R braces,
`PascalCase` classes, `camelCase` methods/variables, `UPPER_SNAKE` constants. Run
`clang-format -i src/*.cpp src/*.h` to reformat.

## Environment Notes

- LTO is explicitly disabled (`build_unflags = -flto`)
- `src/main.cpp` is the active application: it constructs the three controllers, drives the reset line and PHY bring-up in `setup()`, and pumps `Terminal::processInput()` in `loop()`
