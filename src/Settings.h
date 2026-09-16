#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include "Board.h"

// Switch configuration that survives a power cycle. One copy lives in RAM as the running
// config; commands change it and push it to the hardware straight away, save() writes it
// to EEPROM, and load() reads it back at boot. A stored copy that is missing, from a
// different layout version or fails its checksum is replaced by the defaults.
class Settings {
public:
	static constexpr uint8_t MIRROR_RX = 0x01;
	static constexpr uint8_t MIRROR_TX = 0x02;

	struct Data {
		uint8_t portEnabled;				   // bit N-1 set = port N forwards traffic
		uint8_t membership[Board::PORT_COUNT]; // per port: ports it may forward to
		uint8_t mirrorSource;				   // 0 = mirroring off
		uint8_t mirrorDest;
		uint8_t mirrorMode;	 // MIRROR_RX and/or MIRROR_TX
		bool linkLog;		 // print a line when any port changes link state
		uint16_t rgmiiDelay; // VSC8531 register 20E2 for both external PHYs
	};

	Data data;

	Settings();

	void setDefaults();

	// Read the stored copy into data. Returns false if it was unusable and defaults apply.
	bool load();

	void save();

	// True when the running config matches what is stored in EEPROM.
	bool isSaved();

private:
	static constexpr uint16_t MAGIC = 0x4745; // "GE"
	static constexpr uint8_t LAYOUT_VERSION = 1;
	static constexpr int EEPROM_ADDRESS = 0;

	struct Record {
		uint16_t magic;
		uint8_t version;
		Data data;
		uint8_t crc; // over every preceding byte
	};

	Record makeRecord() const;
	static uint8_t crc8(const uint8_t* bytes, uint8_t length);
};

#endif // SETTINGS_H
