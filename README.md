# managed-gig-ethos

Firmware for the ATmega328P management controller on the managed gigabit switch board.
It brings up the switch and PHYs at power-on and exposes a serial console for register
access.

**Hardware**

| Part | Role | Bus from the ATmega |
|---|---|---|
| Microchip KSZ9897R | 7-port gigabit switch. Ports 1-5 are its internal PHYs to RJ45. | SPI, chip select D10 |
| 2x Microchip VSC8531XMW-05 | Single-port gigabit PHYs on switch ports 6 and 7 (RGMII), to RJ45 | Bit-banged MDIO. MDC on A5, MDIO on A4. Addresses 0x00 and 0x10 |
| PHY reset line | Drives both VSC8531 resets | A1 |
| ATmega328P | 8 MHz external crystal, 3.3 V rail, urboot bootloader | UART0 at 57600 baud |

Datasheets are in `docs/`.

## Prerequisites

- [PlatformIO Core](https://platformio.org/install/cli). Installs the AVR toolchain and
  MiniCore automatically on first build.
- `avrdude` 7 or newer on the PATH for the raw ISP commands below. PlatformIO also
  bundles its own copy for the `pio` targets.
- `clang-format` for the style check.
- On WSL2, USB devices reach Linux through `usbipd-win`. From a Windows terminal:

  ```
  usbipd list
  usbipd attach --wsl --busid <BUSID>
  ```

  Attach both the USBasp and the USB-serial adapter. If a device resets, usbipd drops
  it and you have to attach again. Check with `lsusb`. The USBasp shows as
  `16c0:05dc Van Ooijen Technische Informatica`.

## Build

```
pio run                      # default environment, Upload_UART
pio run -t compiledb         # regenerate compile_commands.json for clangd
clang-format -i src/*.cpp src/*.h
```

Current footprint is about 10.5 KB flash and 290 bytes of static RAM.

## Programming

There are two ways into the chip and each has one job:

- **ISP** (USBasp or Atmel ICE) sets fuses and installs the bootloader. Any ISP flash
  write erases the whole chip first, so it is not the everyday path.
- **UART** (urboot bootloader, `urclock` protocol) uploads the application. No
  programmer, no pins to hold.

### Step 1: USBasp hardware setup

Do all of this before touching the board.

1. **Voltage jumper to 3.3 V.** The board runs on a 3.3 V rail. On 5 V the programmer
   drives 5 V into the ATmega's pins and onto the rail shared with the switch and PHYs,
   whose IO is rated to about 3.6 V.
2. **Orientation.** Pin 1 of the 6-pin ISP footprint is MISO and is the marked corner.
   Rotating the connector 180 degrees puts the programmer's RESET driver onto the
   board's power rail and shorts it out. Layout looking at the board:

   ```
   1 MISO   2 VCC
   3 SCK    4 MOSI
   5 RESET  6 GND
   ```

3. **Pogo pins.** Press straight down with even pressure. A tilted fixture lifts one pin
   and gives "target does not answer". While the pins are seated the ATmega is held in
   reset, so the serial console goes quiet and restarts when you lift off. That is
   normal.
4. **Fresh chips only.** A brand new ATmega328P runs at 1 MHz and needs an ISP clock
   under 250 kHz. This USBasp's firmware ignores avrdude's `-B` option, so close the
   slow-SCK jumper (JP3) on the programmer for the first burn and open it afterward.
   A chip that already has these fuses runs at 8 MHz and does not need it.

### Step 2: Confirm the link

```
avrdude -c usbasp -p m328p -v
```

Expect `Device signature = 1E 95 0F (ATmega328P ...)`. The line
`Error: cannot set sck period; please check for usbasp firmware update` is the
programmer's old firmware and is harmless. To read the fuses:

```
avrdude -c usbasp -p m328p -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h -U lock:r:-:h
```

| Fuse | Expected | Meaning |
|---|---|---|
| lfuse | 0xF7 | Full-swing crystal oscillator, no clock divide |
| hfuse | 0xD7 | SPI enabled, EEPROM preserved on erase, 512-byte boot section, reset vector to application (urboot is a vector bootloader) |
| efuse | 0xFF | Brown-out detection disabled |
| lock | 0xFF | Unlocked |

### Step 3: Fuses and bootloader (ISP, pins held)

One command, about 15 seconds. It erases the chip, writes fuses, installs urboot, and
leaves the lock bits open.

```
avrdude -c usbasp -p m328p \
  -U lfuse:w:0xf7:m -U hfuse:w:0xd7:m -U efuse:w:0xff:m \
  -U flash:w:$HOME/.platformio/packages/framework-arduino-avr-minicore/bootloaders/urboot/atmega328p/watchdog_1_s/autobaud/uart0_rxd0_txd1/no-led/urboot_atmega328p_pr_ee_ce.hex:i \
  -U lock:w:0xff:m
```

The PlatformIO equivalent computes the same values from `platformio.ini`. The first run
downloads PlatformIO's avrdude package, so run it once with nothing connected to get
the download out of the way, then again with the pins held:

```
pio run -e fuses_bootloader_usbasp -t bootloader    # USBasp
pio run -e fuses_bootloader -t bootloader           # Atmel ICE
```

The urboot image is the autobaud variant, so the 57600 in `platformio.ini` is only the
speed avrdude uses to talk to it.

### Step 4: Application (UART, hands free)

```
pio run -e Upload_UART -t upload
```

PlatformIO auto-detects the serial port. If it picks the wrong one, add
`--upload-port /dev/ttyUSB0`. Then open the console:

```
pio device monitor
```

Type `help` for the command list. Quick check that both chips answer:

```
read 0x0000 4          -> 0x00 0x98 0x97 0xNN   (KSZ9897R chip ID, NN = revision)
readmdc 0x00 0x02      -> 0x0007                (VSC8531 OUI)
readmdc 0x00 0x03      -> 0x057N                (VSC8531 model, N = revision)
```

### Alternative: application over ISP

Works, but erases the bootloader, so serial uploads stop working until Step 3 is
repeated.

```
pio run -e Upload_USBasp -t upload
# or
avrdude -c usbasp -p m328p -U flash:w:.pio/build/Upload_UART/firmware.hex:i
```

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| `cannot find USB device with vid=0x16c0 pid=0x5dc` | Programmer not on the USB bus | `lsusb`. On WSL2, re-run `usbipd attach`. |
| `cannot set sck period; please check for usbasp firmware update` | Clone firmware can't change ISP clock | Ignore. Use the slow-SCK jumper if you need a slow clock. |
| `program enable: target does not answer (0x01)` | Board off, pins not seated, connector rotated, or fresh chip at 1 MHz with fast SCK | Work through Step 1. |
| Board shorts out and programmer disappears when connected | Connector rotated 180 degrees | Pin 1 is MISO at the marked corner. |
| Serial console silent while programmer is attached | ATmega held in reset by the programmer | Normal. It restarts when you lift the pins. |
| `urclock` upload times out | No bootloader on the chip, or wrong port | Do Step 3, or pass `--upload-port`. |

## Serial console commands

```
read <address> <count>       SPI read of <count> bytes from switch register <address>, e.g. read 0x01FF 3
write <address> <value>      SPI write one byte, e.g. write 0x01FF 0xC0
readmdc <phy> <reg>          MDIO read, e.g. readmdc 0x01 0x00
writemdc <phy> <reg> <val>   MDIO write, e.g. writemdc 0x01 0x00 0x1234
scanmdc                      Find the first responding PHY address
help                         List commands
```

Switch addresses are 16-bit: `0xPFRR` where P is the port (0 = global), F the function
block, RR the register. See the KSZ9897R datasheet section 5.
