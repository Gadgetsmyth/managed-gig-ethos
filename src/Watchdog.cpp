#include "Watchdog.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>

// Timeout must exceed the longest console command; scanmdc can take 3.2 s.
static constexpr uint8_t TIMEOUT = WDTO_8S;

// Markers written before a firmware-initiated reset and read back after it. SRAM keeps
// its contents through a reset (not a power cycle), and .noinit keeps the C runtime
// from zeroing it. urboot only touches the top of RAM for its stack, so these survive.
static constexpr uint16_t MARKER_WATCHDOG = 0xDEAD;
static constexpr uint16_t MARKER_REBOOT = 0xB007;
static uint16_t marker __attribute__((section(".noinit")));

// MCUSR reset flags as they were at reset.
static uint8_t resetFlags __attribute__((section(".noinit")));

// Runs from .init3, before .data/.bss are set up. urboot (like optiboot) clears MCUSR
// and hands the original flags to the application in r2; take that first, then OR in
// MCUSR itself for the no-bootloader case. Clearing MCUSR and disabling the watchdog
// here is required after a watchdog reset, since WDRF forces the watchdog back on.
static void captureResetFlags() __attribute__((naked, used, section(".init3")));
static void captureResetFlags() {
	uint8_t bootloaderFlags;
	__asm__ __volatile__("mov %0, r2" : "=r"(bootloaderFlags));
	resetFlags = bootloaderFlags | MCUSR;
	MCUSR = 0;
	wdt_disable();
}

// Stamp the marker and reset as fast as the hardware allows.
static void resetWithMarker(uint16_t value) __attribute__((noreturn));
static void resetWithMarker(uint16_t value) {
	marker = value;
	wdt_enable(WDTO_15MS);
	for (;;) {
	}
}

void Watchdog::begin() {
	uint8_t savedSreg = SREG;
	cli();
	wdt_reset();
	// Timed sequence: unlock, then write mode and prescaler within four cycles.
	WDTCSR = _BV(WDCE) | _BV(WDE);
	WDTCSR = _BV(WDIE) | _BV(WDE) | ((TIMEOUT & 0x08) ? _BV(WDP3) : 0) | (TIMEOUT & 0x07);
	SREG = savedSreg;
}

void Watchdog::kick() {
	wdt_reset();
}

void Watchdog::reboot() {
	resetWithMarker(MARKER_REBOOT);
}

// First timeout lands here: the firmware stopped kicking. Record it and reset now
// rather than waiting a second full timeout for the hardware reset.
ISR(WDT_vect) {
	resetWithMarker(MARKER_WATCHDOG);
}

void Watchdog::printResetCause() {
	Serial.print(F("Reset: "));
	if (resetFlags & _BV(PORF)) {
		Serial.print(F("power-on"));
	} else if (resetFlags & _BV(BORF)) {
		Serial.print(F("brown-out"));
	} else if ((resetFlags & _BV(WDRF)) && marker == MARKER_WATCHDOG) {
		Serial.print(F("watchdog (firmware hang)"));
	} else if ((resetFlags & _BV(WDRF)) && marker == MARKER_REBOOT) {
		Serial.print(F("software reboot"));
	} else if (resetFlags & (_BV(WDRF) | _BV(EXTRF))) {
		// WDRF without a marker is urboot's exit path after an external reset
		Serial.print(F("external"));
	} else {
		Serial.print(F("unknown"));
	}
	Serial.print(F(" (MCUSR 0x"));
	Serial.print(resetFlags, HEX);
	Serial.println(')');
	marker = 0;
}
