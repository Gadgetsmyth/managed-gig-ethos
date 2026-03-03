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

# Open serial monitor (9600 baud, port from Upload_UART)
pio device monitor
```

## Hardware Configuration

- **MCU**: ATmega328P at 8 MHz (external oscillator)
- **Bootloader**: urboot at 57600 baud
- **ISP Programmer**: Atmel ICE (USB)
- **Serial port**: `/dev/ttyUSB0`

## Code Architecture

The intended application (currently commented out in `src/main.cpp`) consists of three controllers:

- **`SpiController`** (`include/` / `lib/`): Manages SPI bus communication with PHY chips
- **`MdcMdioController`** (`include/` / `lib/`): Manages MDC (clock, A4/PC4) and MDIO (data, A5/PC5) bus for PHY register access; includes `initialize_dual_phy()` for dual-PHY setup with patch registers
- **`Terminal`** (`include/` / `lib/`): Serial terminal (57600 baud) that accepts commands and delegates to SPI/MDC controllers

All application headers belong in `include/`, private libraries in `lib/<LibraryName>/`. PlatformIO auto-discovers library dependencies.

## Environment Notes

- LTO is explicitly disabled (`build_unflags = -flto`)
- The active `main.cpp` is a placeholder blink sketch on pin 11; the real application is commented out
