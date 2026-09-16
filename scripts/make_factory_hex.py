#!/usr/bin/env python3
"""Build one Intel HEX file holding the application and the urboot bootloader, for
programming blank ATmega328Ps over ISP in manufacturing.

Usage:  python3 scripts/make_factory_hex.py [-e ENV]

Runs `pio run` for the environment, merges its firmware.hex with the same urboot image
the `fuses_bootloader` environment burns, checks the two do not overlap, and writes
release/managed-gig-ethos-<version>-factory.hex. Fuse and lock bytes are not part of a
hex file; docs/MANUFACTURING.md gives them alongside the avrdude command.
"""
import argparse
import os
import subprocess
import sys

from intelhex import IntelHex

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BOOTLOADER = os.path.expanduser(
	"~/.platformio/packages/framework-arduino-avr-minicore/bootloaders/urboot/atmega328p"
	"/watchdog_1_s/autobaud/uart0_rxd0_txd1/no-led/urboot_atmega328p_pr_ee_ce.hex"
)
FLASH_SIZE = 32 * 1024


def run(*cmd):
	return subprocess.check_output(cmd, cwd=ROOT, stderr=subprocess.DEVNULL).decode().strip()


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("-e", "--env", default="Upload_ISP")
	args = parser.parse_args()

	if run("git", "status", "--porcelain", "--untracked-files=no"):
		sys.exit("Working tree has uncommitted changes; a factory image must come from a commit.")
	version = run("git", "describe", "--tags", "--always", "--dirty")

	subprocess.check_call(["pio", "run", "-e", args.env], cwd=ROOT)
	app_path = os.path.join(ROOT, ".pio", "build", args.env, "firmware.hex")

	app = IntelHex(app_path)
	boot = IntelHex(BOOTLOADER)
	if app.maxaddr() >= boot.minaddr():
		sys.exit(f"Application ends at 0x{app.maxaddr():04X}, bootloader starts at 0x{boot.minaddr():04X}: overlap.")
	if boot.maxaddr() >= FLASH_SIZE:
		sys.exit("Bootloader extends past the end of flash.")

	merged = IntelHex()
	merged.merge(app, overlap="error")
	merged.merge(boot, overlap="error")

	out_dir = os.path.join(ROOT, "release")
	os.makedirs(out_dir, exist_ok=True)
	out_path = os.path.join(out_dir, f"managed-gig-ethos-{version}-factory.hex")
	merged.write_hex_file(out_path)

	print(f"application  0x{app.minaddr():04X}-0x{app.maxaddr():04X}  ({len(app)} bytes)")
	print(f"bootloader   0x{boot.minaddr():04X}-0x{boot.maxaddr():04X}  ({len(boot)} bytes, urboot no-led)")
	print(f"wrote        {os.path.relpath(out_path, ROOT)}")


if __name__ == "__main__":
	main()
