#!/usr/bin/env python3
"""Build one Intel HEX file holding the application and the urboot bootloader, for
programming blank ATmega328Ps over ISP in manufacturing.

Usage:  python3 scripts/make_factory_hex.py [-e ENV]

Runs `pio run` for the environment, merges its firmware.hex with the same urboot image
the `fuses_bootloader` environment burns, checks the two do not overlap, and writes
release/managed-gig-ethos-<version>-factory.hex. Fuse and lock bytes are not part of a
hex file; docs/MANUFACTURING.md gives them alongside the avrdude command.

urboot here is a vector bootloader (high fuse 0xD7 leaves BOOTRST off), so reset goes
to address 0 and something must send it on to the bootloader. `avrdude -c urclock`
patches that when it uploads over serial; an ISP write of a plain merged image would
boot straight into the application and the bootloader would never run. This script
applies the same patch urclock does (avrdude src/urclock.c, urclock_flash_readhook):
address 0 becomes a jmp to the bootloader, and the application's original reset jmp is
saved in the vector slot the bootloader names in its metadata, from which it starts the
application after a timeout or a completed upload.
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
PAGE_SIZE = 128
VECTOR_SIZE = 4  # ATmega328P interrupt vectors are jmp instructions
JMP_OPCODE = 0x940C


def jmp_bytes(byte_address):
	"""Encode `jmp byte_address` as the 4 little-endian bytes of the instruction."""
	k = byte_address >> 1
	word1 = JMP_OPCODE | (((k >> 17) & 0x1F) << 4) | ((k >> 16) & 1)
	word2 = k & 0xFFFF
	return bytes([word1 & 0xFF, word1 >> 8, word2 & 0xFF, word2 >> 8])


def is_jmp(four_bytes):
	return (four_bytes[0] | (four_bytes[1] << 8)) & 0xFE0E == JMP_OPCODE


def patch_reset_vector(app, boot):
	"""Redirect reset to the bootloader and park the application's reset jmp in the
	bootloader's vector slot. Returns (bootloader start, vector number)."""
	# urboot metadata is the last 6 bytes of flash: pages, vector number, rjmpwp, cap, version.
	meta = bytes(boot.tobinarray(start=FLASH_SIZE - 6, end=FLASH_SIZE - 1))
	pages = meta[0] & 0x7F
	vector = meta[1] & 0x7F
	blstart = FLASH_SIZE - pages * PAGE_SIZE
	if blstart != boot.minaddr() or vector == 0:
		sys.exit(f"Bootloader metadata does not describe a vector bootloader at 0x{boot.minaddr():04X}.")

	original = bytes(app.tobinarray(start=0, end=VECTOR_SIZE - 1))
	if not is_jmp(original):
		sys.exit(f"Application reset vector is not a jmp: {original.hex()}")
	slot = vector * VECTOR_SIZE
	if not is_jmp(bytes(app.tobinarray(start=slot, end=slot + VECTOR_SIZE - 1))):
		sys.exit(f"Vector slot {vector} at 0x{slot:04X} does not hold a jmp; refusing to overwrite it.")

	app.puts(slot, original)
	app.puts(0, jmp_bytes(blstart))
	return blstart, vector


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

	blstart, vector = patch_reset_vector(app, boot)

	merged = IntelHex()
	merged.merge(app, overlap="error")
	merged.merge(boot, overlap="error")

	out_dir = os.path.join(ROOT, "release")
	os.makedirs(out_dir, exist_ok=True)
	out_path = os.path.join(out_dir, f"managed-gig-ethos-{version}-factory.hex")
	merged.write_hex_file(out_path)

	print(f"application  0x{app.minaddr():04X}-0x{app.maxaddr():04X}  ({len(app)} bytes)")
	print(f"bootloader   0x{boot.minaddr():04X}-0x{boot.maxaddr():04X}  ({len(boot)} bytes, urboot no-led)")
	print(f"reset        jmp 0x{blstart:04X} (bootloader); application entry saved in vector {vector}")
	print(f"wrote        {os.path.relpath(out_path, ROOT)}")


if __name__ == "__main__":
	main()
