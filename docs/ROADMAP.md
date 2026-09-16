# Roadmap

Last updated 2026-09-15. The console command reference is in `README.md`; bench setup and
decisions are in `docs/HANDOFF.md`.

## Where the product stands

| Version | Contents | State |
|---|---|---|
| v0.1 | Reliability baseline: flash strings, watchdog with reset-cause reporting, chip ID checks at boot, `status`, `version`, KSZ9897R errata, MAC-follows-PHY speed tracking on ports 6-7 | Merged (PR #3), not yet tagged |
| v0.2 | Managed-switch commands: `port`, `isolate`, `mirror`, `counters`, `log`, `rgmii`, plus `show` / `save` / `defaults` with EEPROM persistence | Implemented, awaiting bench test |
| v0.3 | Below | Planned |

Footprint after v0.2: 19.7 KB flash of 32 KB (urboot takes the top 0.5 KB), 286 bytes
static RAM of 2 KB. About 12 KB of flash remains for v0.3.

## v0.2 bench test plan

Run after flashing, in this order. Each step is independent of the ones after it.

1. Boot. Expect `Config: no saved settings, using defaults` the first time, then the chip
   checks, then one `link: port N up ...` line per connected port.
2. `show`, `save`, `reboot`. Expect no defaults message on the second boot and
   `Config: saved` from `show`.
3. `port 6 off` with a PC on port 6. Expect the PC to report the cable unplugged and
   `status` to show `off`. `port 6 on` brings it back; a `link:` line should follow.
4. `isolate 6 1` with the router on port 1 and the PC on port 6. The PC should still reach
   the internet but not a second device on any other port. `isolate 6 all` restores it.
5. `mirror 6 2 both` with a laptop running Wireshark on port 2. Traffic to and from the
   PC on port 6 should appear. `mirror off` stops it.
6. `counters 6` before and after a 10 s iperf run. RxBytes and TxBytes should be in the
   gigabyte range and RxCrcErr zero. `counters 6` again straight away should show near
   zero, since the chip clears on read.
7. `counters clear` then `counters 1`. Expect zeros apart from what arrived in between.
8. `rgmii 0x0044` with iperf running to port 6. The link should drop and recover; compare
   throughput and error counters against `0x0042`. This is the open tuning item from
   the handoff.
9. `defaults` after changing several settings. Ports that were off come back on; ports
   that were on should not flap. `show` should say unsaved.
10. `log off`, unplug and replug a port, expect silence. `log on` and repeat.

## v0.3

In suggested order. Each is a separate small change that can be flashed and verified on
its own, per the one-step-at-a-time working style.

1. **Tag `v0.1.0` on master and `v0.2.0` once v0.2 merges**, so `git describe` produces a
   real version string in the banner.
2. **`speed <n> auto|10|100|1000`.** Force a port's speed and duplex by narrowing the PHY's
   autonegotiation advertisement (IEEE registers 4 and 9) rather than disabling
   autonegotiation, so the partner still negotiates and duplex stays correct. Internal
   PHYs go through SPI at `0xN108`/`0xN112`; the VSC8531s go over MDIO. Persist per port
   in `Settings` (one byte per port). About 1 KB.
3. **`storm on|off`.** Broadcast storm protection: global rate in `0x0334`, enable via
   `0x0332` bit 0, per-port enable in the port MAC control register `0xN400` bit 0... verify
   the per-port bit in the datasheet before writing it. Small, but confirm on the bench that
   a broadcast flood is actually rate-limited.
4. **`cable <n>`.** LinkMD cable diagnostics on ports 1-5 through `0xN124` and
   `0xN112`, per datasheet section 4.2.x: reports open, short, or OK with an approximate
   distance. Sales-friendly feature. Needs the manual master/slave setup step and a link
   drop during the test.
5. **`OK` / `ERR <reason>` response prefixes** on every command, for a factory test
   fixture that drives the console from a script. Also change `read` to refuse a count that
   would cross a register block boundary (handoff known issue 4).
6. **MDIO turnaround-ack check** in `MdcMdioController::readRegister`, so a missing PHY
   reads as absent rather than 0xFFFF, and a `scanmdc` that lists every responding address
   instead of stopping at the first (handoff known issue 2).
7. **Brown-out fuse (2.7 V)** in `platformio.ini`, burned via ISP, then verify boot and that
   saved settings survive a power dip (handoff known issue 5). Matters more now that the
   product relies on EEPROM.
8. **Host-side unit tests** (`pio test -e native`) for the argument parsers, port-list
   parser and `Settings` CRC/round-trip, plus a GitHub Actions workflow that builds both
   environments and runs `clang-format --dry-run`.
9. **Uptime and last reset cause in `status`.** Cheap, useful in the field.
10. **Tagged 802.1Q VLANs.** Per-port PVID, tagged/untagged membership through the VLAN
    table (`0x0400` block indirect access) and ingress filtering. The largest v0.3 item and
    the one most likely to need the remaining flash budget; scope it last.

## Known gaps in v0.2 to keep in mind

- **Mirror destination still forwards normally.** A real managed switch usually stops
  ordinary traffic egressing the sniffer port. Not done in v0.2; combine `isolate` on the
  other ports if that matters.
- **Mirroring plus isolation.** Mirrored copies obey the source port's membership mask
  (global register `0x0390` bit 1 is left at its default). A sniffer port outside the
  source's membership list will not receive copies.
- **36-bit byte counters print only the low 32 bits**, with a `+` marker when the upper
  nibble or the overflow flag is set. Printing the full value needs 64-bit division, about
  1 KB of flash; deferred.
- **`counters clear` uses the chip's flush mechanism** (`0x0336` bit 7 gated by `0xN500`
  bit 24). Verify on the bench that it zeroes counters on all seven ports; the fallback is
  to read every counter once.
- **Disabled ports power the PHY down.** For the internal PHYs this is IEEE register 0 bit
  11 through SPI. Confirm on the bench that the link LED on the partner goes out and that
  `port N on` renegotiates without a reboot.
