# Roadmap

Last updated 2026-09-15. The console command reference is in `README.md`; bench setup and
decisions are in `docs/HANDOFF.md`.

## Where the product stands

| Version | Contents | State |
|---|---|---|
| v0.1 | Reliability baseline: flash strings, watchdog with reset-cause reporting, chip ID checks at boot, `status`, `version`, KSZ9897R errata, MAC-follows-PHY speed tracking on ports 6-7 | Merged (PR #3), not yet tagged |
| v0.2 | Managed-switch commands: `port`, `speed`, `qos`, `ratelimit`, `isolate`, `mirror`, `counters`, `log`, `rgmii`, plus `show` / `save` / `defaults` with EEPROM persistence | Bench-tested 2026-09-15, all steps pass except `mirror`, which needs a capture host. `qos` and `ratelimit` added afterwards, see step 12 |
| v0.3 | Below | Planned |

Footprint after v0.2: 22.9 KB flash of 32 KB (urboot takes the top 0.5 KB), 328 bytes
static RAM of 2 KB. About 9 KB of flash remains for v0.3.

## v0.2 bench test plan

Run after flashing, in this order. Each step is independent of the ones after it.
Results from 2026-09-15 (build 4bfe7b4, PC on port 6, router on port 1): steps 1-4 and
6-10 pass. Step 5 not run (no capture host on the bench). Step 8 ran 2.7 GB through port 6
in both directions at 0x0044 with zero CRC, symbol, alignment and drop counts, matching
0x0042; 0x0044 is now the compiled default. Step 11 (`speed`) passed on both the external
and internal PHY paths: 94.0/94.4 Mbit/s at forced 100 with zero collisions or errors.

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
11. `speed 6 100` with the PC on port 6. Expect a `link: port 6 down` then
    `link: port 6 up 100 full`, `status` showing 100 full on both the PHY and MAC columns,
    and iperf around 94 Mbit/s. `speed 6 auto` returns it to 1000. Repeat `speed 1 100`
    on the router port to exercise the internal-PHY path.
12. `ratelimit 6 out 110` then iperf from the Ubuntu box toward the PC (reverse mode):
    expect about 105 Mbit/s. `ratelimit 6 in 110` and a run the other way: expect the same
    (the limiter throttles with pause frames; its discards do not show in RxDropped).
    `ratelimit 6 in off` / `out off` restores 941 Mbit/s. `qos on`, `qos 6 7`, `show`: expect the QoS line and Prio column, and
    iperf unchanged (a single flow cannot show queueing). `qos off`.

## v0.3

In suggested order. Each is a separate small change that can be flashed and verified on
its own, per the one-step-at-a-time working style.

1. **Tag `v0.1.0` on master and `v0.2.0` once v0.2 merges**, so `git describe` produces a
   real version string in the banner.
2. **`storm on|off`.** Broadcast storm protection: per-port enable is `0xN400` bit 1
   (Port MAC Control 0), the global rate is the 11-bit field split across `0x0332` bits 2:0
   and `0x0334` (default 1% of line rate). Small, but confirm on the bench that a
   broadcast flood is actually rate-limited.
3. **`cable <n>`.** LinkMD cable diagnostics on ports 1-5, datasheet section 4.1.9 and
   the PHY LinkMD register `0xN124`: disable autonegotiation, force master/slave via
   `0xN112`, start the test with bit 15 of `0xN124`, read open/short and distance per
   pair, then restore autonegotiation. Sales-friendly feature; the link drops during the
   test.
4. **`OK` / `ERR <reason>` response prefixes** on every command, for a factory test
   fixture that drives the console from a script. Also change `read` to refuse a count that
   would cross a register block boundary (handoff known issue 4).
5. **MDIO turnaround-ack check** in `MdcMdioController::readRegister`, so a missing PHY
   reads as absent rather than 0xFFFF, and a `scanmdc` that lists every responding address
   instead of stopping at the first (handoff known issue 2).
6. **Brown-out fuse (2.7 V)** in `platformio.ini`, burned via ISP, then verify boot and that
   saved settings survive a power dip (handoff known issue 5). Matters more now that the
   product relies on EEPROM.
7. **Host-side unit tests** (`pio test -e native`) for the argument parsers, port-list
   parser and `Settings` CRC/round-trip, plus a GitHub Actions workflow that builds both
   environments and runs `clang-format --dry-run`.
8. **Uptime and last reset cause in `status`.** Cheap, useful in the field.
9. **QoS extras.** DSCP classification (`0xN801` bit 1 plus the global DSCP map), strict
   priority scheduling as an option (`0xN914`, indexed per queue), and queue-based rate
   limits. The v0.2 `qos` command covers port priority and 802.1p only.
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
  bit 24). Verified: after a 1.3 GB transfer through port 1, a clear followed by
  `counters 1` showed only background traffic.
- **Disabled ports power the PHY down.** Verified on port 6: the partner sees the cable
  unplugged and `port N on` renegotiates without a reboot. Ports 1-5 use the same IEEE bit
  through SPI but have not been exercised with a partner attached.
