#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>

// Hardware watchdog plus reset-cause reporting that stays accurate behind urboot.
//
// urboot exits to the application by letting its own watchdog reset the chip, so every
// external reset reaches the firmware as a plain watchdog reset flag. To tell the two
// apart, the firmware's watchdog runs in interrupt-then-reset mode: a genuine timeout
// first fires an interrupt that stamps a marker in reset-surviving RAM, then resets.
// reboot() stamps a different marker. A watchdog flag with no marker is therefore an
// external reset that came through the bootloader.
class Watchdog {
public:
	// Arm the watchdog. Call first thing in setup().
	static void begin();

	// Service the watchdog. Call every loop() iteration.
	static void kick();

	// Restart the controller. Does not return.
	static void reboot() __attribute__((noreturn));

	// Print one line describing what caused this boot, then clear the marker.
	static void printResetCause();
};

#endif // WATCHDOG_H
