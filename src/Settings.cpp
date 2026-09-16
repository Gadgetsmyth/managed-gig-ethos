#include "Settings.h"
#include <EEPROM.h>
#include "MdcMdioController.h"

Settings::Settings() {
	setDefaults();
}

void Settings::setDefaults() {
	data.portEnabled = Board::ALL_PORTS_MASK;
	for (uint8_t i = 0; i < Board::PORT_COUNT; i++)
		data.membership[i] = Board::ALL_PORTS_MASK;
	data.mirrorSource = 0;
	data.mirrorDest = 0;
	data.mirrorMode = MIRROR_RX | MIRROR_TX;
	data.linkLog = true;
	data.rgmiiDelay = MdcMdioController::DEFAULT_RGMII_DELAY;
}

bool Settings::load() {
	Record record;
	EEPROM.get(EEPROM_ADDRESS, record);
	bool valid = record.magic == MAGIC && record.version == LAYOUT_VERSION &&
		record.crc == crc8(reinterpret_cast<const uint8_t*>(&record), sizeof(record) - 1);
	if (!valid) {
		setDefaults();
		return false;
	}
	data = record.data;
	return true;
}

// EEPROM.put only rewrites bytes that differ, so repeated saves cost no wear.
void Settings::save() {
	Record record = makeRecord();
	EEPROM.put(EEPROM_ADDRESS, record);
}

bool Settings::isSaved() {
	Record stored;
	EEPROM.get(EEPROM_ADDRESS, stored);
	Record current = makeRecord();
	return memcmp(&stored, &current, sizeof(Record)) == 0;
}

Settings::Record Settings::makeRecord() const {
	Record record;
	record.magic = MAGIC;
	record.version = LAYOUT_VERSION;
	record.data = data;
	record.crc = crc8(reinterpret_cast<const uint8_t*>(&record), sizeof(record) - 1);
	return record;
}

// CRC-8, polynomial 0x07, no reflection. Enough to catch a torn write or blank EEPROM.
uint8_t Settings::crc8(const uint8_t* bytes, uint8_t length) {
	uint8_t crc = 0;
	while (length--) {
		crc ^= *bytes++;
		for (uint8_t bit = 0; bit < 8; bit++)
			crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07)
							   : static_cast<uint8_t>(crc << 1);
	}
	return crc;
}
