#include <stdio.h>
#include <ESP.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#include "basicblock.h"

BasicBlock::BasicBlock()
{
	memset(&config, 0, sizeof(struct config));

}

void BasicBlock::setup()
{
	// Try to set up the UUID and hostname
	EEPROM.begin(512);

	EEPROM.get(CONFIG_START_ADDR, config);
	Serial.println();	// To get rid of junk that shows up in the serial monitor
	print_config(&config);

	// TODO: Add logic to start AP if ESP was continuously reset X times
	updateResetCounter(++config.resetCounter);
	delay(3000);

	if (config.resetCounter >= 5) {
		config.resetEEPROM = 1;
	} else {
		config.resetCounter = 0;
	}

	if(config.resetEEPROM) {
		// We need to reset the EEPROM
		resetEEPROM();
		Serial.printf("Finished reset of EEPROM. Restarting ESP\n");
		ESP.reset();
	}

	if(config.uuid[8] != '-') {
		// Invalid UUID
		Serial.printf("Invalid UUID..Overwriting config with defaults\n");
		strncpy(config.uuid, DEFAULT_UUID, 36);
		snprintf(config.hostname, 32, "ESP-%d", ESP.getChipId());

		EEPROM.put(CONFIG_START_ADDR, config);

		Serial.println("EEPROM contents after put+get:");
		EEPROM.get(CONFIG_START_ADDR, config);
		print_config(&config);
		EEPROM.commit();
	}

	WiFi.hostname(config.hostname);

	bool shouldStartAP = false;

	if(strlen(config.ssid) == 0) {
		shouldStartAP =  true;
	}

	if(config.resetCounter != 0) {
		shouldStartAP = true;
		memset(config.ssid, 0, sizeof(config.ssid));    
	}

	if(shouldStartAP) {
		Serial.printf("Starting soft AP with SSID: %s\n", config.hostname);
		IPAddress local_IP(192,168,1,1);
		IPAddress gateway(192,168,1,1);
		IPAddress subnet(255,255,255,0);
		WiFi.softAPConfig(local_IP, gateway, subnet);
		WiFi.softAP(config.hostname);
	}

	if(strlen(config.ssid) != 0) {
		Serial.printf("Read wifi config from EEPROM...\n");
		setupNetwork();
		setupOTA();
	}
}

// Expects EEPROM.begin() to have been called
void BasicBlock::resetConfig()
{
	for(int i = CONFIG_START_ADDR; i < CONFIG_START_ADDR+sizeof(struct config); i++) {
		EEPROM.write(i, 0);
	}
	EEPROM.commit();
}

void BasicBlock::resetEEPROM()
{
	for(int i = 0; i < EEPROM.length(); i++) {
		EEPROM.write(i, 0);
	}
	EEPROM.commit();
}

void BasicBlock::updateUUID(char *val)
{
	EEPROM.begin(512);
	int offset = offsetof(struct config, uuid);
	memset(config.uuid, 0, sizeof(config.uuid));
	strncpy(config.uuid, val, sizeof(config.uuid));
	EEPROM.put(CONFIG_START_ADDR+offset, config.uuid);
	EEPROM.commit();
}

void BasicBlock::updateHostname(char *val)
{
	EEPROM.begin(512);
	int offset = offsetof(struct config, hostname);
	memset(config.hostname, 0, sizeof(config.hostname));
	strncpy(config.hostname, val, sizeof(config.hostname));
	EEPROM.put(CONFIG_START_ADDR+offset, config.hostname);
	EEPROM.commit();
}

void BasicBlock::updateSSID(char *val)
{
	EEPROM.begin(512);
	int offset = offsetof(struct config, ssid);
	memset(config.ssid, 0, sizeof(config.ssid));
	strncpy(config.ssid, val, sizeof(config.ssid));
	EEPROM.put(CONFIG_START_ADDR+offset, config.ssid);
	EEPROM.commit();
}

void BasicBlock::updatePSK(char *val)
{
	EEPROM.begin(512);
	int offset = offsetof(struct config, psk);
	memset(config.psk, 0, sizeof(config.psk));
	strncpy(config.psk, val, sizeof(config.psk));
	EEPROM.put(CONFIG_START_ADDR+offset, config.psk);
	EEPROM.commit();
}

void BasicBlock::updateResetCounter(int val)
{
	EEPROM.begin(512);
	int offset = offsetof(struct config, resetCounter);
	EEPROM.put(CONFIG_START_ADDR+offset, val);
	EEPROM.commit();
}

/**
 * This function returns a char * representing the contents of the basic block
 * configuration page
 */
char *BasicBlock::wsServeIndex()
{
	return R"=(<!DOCTYPE html><html><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1"><link href="https://fonts.googleapis.com/icon?family=Material+Icons" rel=stylesheet><title>basicblock-vuejs</title><link href=/static/css/app.2c8b518f6458c4af5551f53d4881abff.css rel=stylesheet></head><body><div id=app></div><script type=text/javascript src=/static/js/manifest.2ae2e69a05c33dfc65f8.js></script><script type=text/javascript src=/static/js/vendor.a185610171b21f082148.js></script><script type=text/javascript src=/static/js/app.eedca2c9cbf4e8820b57.js></script></body></html>)=";
}

char *BasicBlock::getUUID()
{
	return config.uuid;
}

char *BasicBlock::getWifiSSID()
{
	return config.ssid;
}

char *BasicBlock::getWifiPSK()
{
	return config.psk;
}

char *BasicBlock::getHostname()
{
	return config.hostname;
}

void BasicBlock::updateConfig(char *configJsonStr)
{
	StaticJsonBuffer<512> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject(configJsonStr);
	struct config newConfig;
	memset(&newConfig, 0, sizeof(struct config));
  // TODO: Add checks here to make sure values are sane
	strncpy(newConfig.uuid, root["uuid"], 36);
	strncpy(newConfig.hostname, root["hostname"], 32);
	strncpy(newConfig.ssid, root["ssid"], 64);
	strncpy(newConfig.psk, root["psk"], 64);
	Serial.println("Received new config:");
	print_config(&newConfig);
	EEPROM.begin(512);
	EEPROM.put(CONFIG_START_ADDR, newConfig);
	EEPROM.commit();
	Serial.println("Resetting ESP ...");
	ESP.reset();
}

void BasicBlock::setupNetwork() {
  WiFi.begin(config.ssid, config.psk);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21) {
    Serial.println("Unable to connect to wifi ... Starting AP mode");
		updateResetCounter(3);
		ESP.reset();	// FIXME: This should ideally set a flag or reset config.ssid+psk so it doesn't keep resetting and doesn't lose existing wifi credentials
  }
  WiFi.hostname(config.hostname);
  Serial.printf("Wi-Fi connection established\n");
}

void BasicBlock::setupOTA() {
  ArduinoOTA.setHostname(config.hostname);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void BasicBlock::loop() {
	ArduinoOTA.handle();
}
