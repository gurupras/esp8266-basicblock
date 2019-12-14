/*
  basicblock.h - Basic block for ESP development
  Copyright (c) 2018 Guru Prasad Srinivasa.  All right reserved.
*/

// ensure this library description is only included once
#ifndef BASIC_BLOCK_H_
#define BASIC_BLOCK_H_

#include <HardwareSerial.h>
#include <stdint.h>
#include <EEPROM.h>

#define DEFAULT_UUID "00000000-0000-0000-0000-000000000000"

#define CONFIG_START_ADDR 1

struct config {
	char UUID[37];
	char hostname[32];
	char ssid[64];
	char psk[64];
	int resetCounter;
	bool forceStartAP;
};

static void print_config(struct config *config) {
	Serial.printf("UUID: %s\n", config->UUID);
	Serial.printf("hostname: %s\n", config->hostname);
	Serial.printf("ssid: %s\n", config->ssid);
	Serial.printf("psk: %s\n", config->psk);
	Serial.printf("resetCounter: %d\n", config->resetCounter);
}

// library interface description
class BasicBlock
{
  // user-accessible "public" interface
  public:
    BasicBlock();
	void earlySetup(void);
	void earlySetup(bool);
    void setup(void);
		char *wsServeIndex(void);
		char *getUUID(void);
		void updateUUID(char *);

		char *getHostname(void);
		void updateHostname(char *);

		char *getWifiSSID(void);
		void updateWifiSSID(char *);

		char *getWifiPSK(void);
		void updateWifiPSK (char *);

		void updateConfig(char *);
		void loop(void);

		void resetEEPROM(void);
		void resetConfig(void);

  // library-accessible "private" interface
  protected:
		struct config config;
  private:
		void updateResetCounter(int);
		void setupNetwork(void);
		void setupOTA(void);
};

#endif
