# Manufacturing: programming the ATmega328P

One Intel HEX file programs a blank ATmega328P completely over ISP: the application and
the urboot serial bootloader together. Fuses and lock bits are not part of a hex file and
are written by the same avrdude command.

## Files

| Item | Value |
|---|---|
| Image | `managed-gig-ethos-<version>-factory.hex`, attached to the GitHub release for `<version>` |
| Application | flash 0x0000 up to about 0x5A00, reports `<version>` on the serial console |
| Bootloader | urboot, autobaud, no LED, flash 0x7E00-0x7FFF |
| Low fuse | `0xF7` full-swing 8 MHz crystal |
| High fuse | `0xD7` 512-byte boot section, EEPROM preserved on chip erase |
| Extended fuse | `0xFE` brown-out detection at 1.8 V. The ATmega runs from a 2.5 V rail, so the 2.7 V setting (`0xFD`) holds it in reset permanently |
| Lock | `0xFF` no locks |

Fuse values must match the table exactly. A wrong low fuse selects a different clock
source and the board will not start.

## Command

Chip erase, fuses, image and lock in one operation, with a USBasp:

```
avrdude -c usbasp -P usb -p m328p -e \
  -U lfuse:w:0xF7:m -U hfuse:w:0xD7:m -U efuse:w:0xFE:m \
  -U flash:w:managed-gig-ethos-<version>-factory.hex:i \
  -U lock:w:0xFF:m
```

Set the USBasp's voltage jumper to 3.3 V to match the board. Expect the signature
`0x1e950f`. Two lines reading "cannot set sck period; please check for usbasp firmware
update" are harmless on USBasp clones with older firmware. An Atmel ICE works with
`-c atmelice_isp` in place of `-c usbasp`.

Verification is automatic: avrdude reads flash back after writing and reports an error on
any mismatch. Exit status 0 means the part is programmed.

## Functional check

Connect a serial adapter (57600 8N1) and power-cycle. The console prints:

```
managed-gig-ethos <version> built ...
Reset: power-on (MCUSR 0x1)
Config: no saved settings, using defaults
Switch KSZ9897 rev 0: OK
PHY 0x00 VSC8531 rev 2: OK
PHY 0x10 VSC8531 rev 2: OK
```

Three `OK` lines mean the microcontroller, the switch and both PHYs are alive and wired
correctly. The "no saved settings" line is expected on a new part.

## Validation record

2026-09-15, `managed-gig-ethos-v0.2.0-factory.hex` (sha256 `14016b73207c2e40...`): programmed
the bench board over a USBasp with the command above. Flash verified at 23,284 bytes, all
fuses and lock verified (extended fuse 0xFF at that point; 0xFD was then tried and held the
2.5 V part in reset, 0xFE burned and verified the same day). First boot printed the `v0.2.0` banner and three `OK` lines; the
reset cause read as a watchdog flag, which only happens when urboot ran. A UART upload
through the bootloader then succeeded and the new banner appeared. Saved EEPROM settings
survived the chip erase.

## Rebuilding the image

From a clean checkout of the release tag:

```
git checkout v0.2.0
python3 scripts/make_factory_hex.py
```

The script builds the application, merges it with the bootloader image from the MiniCore
package, refuses to run on an uncommitted tree, and writes `release/`. Needs the
`intelhex` Python package.
