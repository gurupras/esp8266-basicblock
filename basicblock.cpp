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

	/*
	for(int i = CONFIG_START_ADDR; i < CONFIG_START_ADDR+sizeof(struct config); i++) {
		EEPROM.write(i, 0);
	}
	*/

	EEPROM.get(CONFIG_START_ADDR, config);
	Serial.println();	// To get rid of junk that shows up in the serial monitor
	print_config(&config);
	if(config.UUID[8] != '-') {
		// Invalid UUID
		Serial.printf("Invalid UUID..Overwriting config with defaults\n");
		strncpy(config.UUID, DEFAULT_UUID, 36);
		snprintf(config.hostname, 32, "ESP-%d", ESP.getChipId());

		EEPROM.put(CONFIG_START_ADDR, config);

		Serial.println("EEPROM contents after put+get:");
		EEPROM.get(CONFIG_START_ADDR, config);
		print_config(&config);
		EEPROM.commit();
	}

	bool shouldStartAP = false;

	// TODO: Add logic to start AP if ESP was continuously reset X times
	if(strlen(config.ssid) == 0) {
		shouldStartAP =  true;
	}

	if(config.resetCounter != 0) {
    shouldStartAP = true;
    // Reset the resetCounter
		updateResetCounter(0);
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

void BasicBlock::updateResetCounter(int val)
{
	EEPROM.begin(512);
	int offset = offsetof(struct config, resetCounter);
	EEPROM.put(CONFIG_START_ADDR+offset, 0);
	EEPROM.commit();
}

/**
 * This function returns a char * representing the contents of the basic block
 * configuration page
 */
char *BasicBlock::wsServeIndex()
{
	return "<html><head> <style>.button, button, input[type='button'], input[type='submit']{background-color: #9b4dca; border: 0.1rem solid #9b4dca; border-radius: .4rem; color: #fff; cursor: pointer; display: inline-block; font-size: 1.1rem; font-weight: 700; height: 3.8rem; letter-spacing: .1rem; line-height: 3.8rem; padding: 0 3.0rem; text-align: center; text-decoration: none; text-transform: uppercase; white-space: nowrap;}.button:hover, button:hover, input[type='button']:hover, input[type='submit']:hover{color: #fff; background-color: #606c76;}label, legend{display: block; font-size: 1.6rem; font-weight: 700; margin-bottom: .5rem;}input[type='email'], input[type='number'], input[type='password'], input[type='search'], input[type='tel'], input[type='text'], input[type='url'], textarea, select{-webkit-appearance: none; -moz-appearance: none; appearance: none; background-color: transparent; border: 0.1rem solid #d1d1d1; border-radius: .4rem; box-shadow: none; box-sizing: inherit; height: 3.8rem; padding: .6rem 1.0rem; width: 100%;}input[type='email']:focus, input[type='number']:focus, input[type='password']:focus, input[type='search']:focus, input[type='tel']:focus, input[type='text']:focus, input[type='url']:focus, textarea:focus, select:focus{border-color: #9b4dca; outline: 0;}fieldset, input, select, textarea{margin-bottom: 1.5rem;}button, input{overflow: visible;}button, input, optgroup, select, textarea{font-family: sans-serif; font-size: 100%; line-height: 1.15; margin: 0;}*, *:after, *:before{box-sizing: inherit;}body{color: #606c76; font-family: 'Roboto', 'Helvetica Neue', 'Helvetica', 'Arial', sans-serif; font-size: 1.6em; font-weight: 300; letter-spacing: .01em; line-height: 1.6;}body{margin: 0;}html{box-sizing: border-box; font-size: 62.5%;}html{font-family: sans-serif; line-height: 1.15; -ms-text-size-adjust: 100%; -webkit-text-size-adjust: 100%;}@media (min-width: 40rem) .row{flex-direction: row; margin-left: -1.0rem; width: calc(100% + 2.0rem);}.row{display: flex; flex-direction: column; padding: 0; width: 100%;}@media (min-width: 40rem) .row .column{margin-bottom: inherit; padding: 0 1.0rem;}.row .column{display: block; flex: 1 1 auto; margin-left: 0; max-width: 100%; width: 100%;}</style></head><body> <h1>Device Configuration</h1> <div class='row'> <div class='column' style='flex: 0 0 30%; max-width: 60%'> <div> <label for='uuid'>Device UUID</label> <input id='uuid' type='text'> <button id='generate-uuid' class='button'>Generate UUID</button> </div><div> <label for='hostname'>Device Hostname</label> <input id='hostname' type='text'> </div></div></div><h2>Wi-Fi configuration</h2> <div class='row'> <div class='column' style='flex: 0 0 30%; max-width: 60%'> <div> <label for='wifi-ssid'>Wi-Fi SSID</label> <input id='wifi-ssid' type='text'> <label for='wifi-psk'>Wi-Fi Password</label> <input id='wifi-psk' type='password'> </div></div></div><button id='update-btn' class='button'>Update</button> <script>function xhr(method, endpoint, data, type){return new Promise((resolve, reject)=>{function response(){resolve(this.responseText);}var req=new XMLHttpRequest(); req.overrideMimeType('text/plain; charset=utf'); req.addEventListener('loadend', response); req.open(method, endpoint); if (type){req.setRequestHeader('Content-Type', type);}req.send(data);});}function generateUUID(){var dt=new Date().getTime(); var uuid='xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c){var r=(dt + Math.random() * 16) % 16 | 0; dt=Math.floor(dt / 16); return (c=='x' ? r : (r & 0x3 | 0x8)).toString(16);}); return uuid;}window.onload=function(){var uuid=document.getElementById('uuid'); var hostname=document.getElementById('hostname'); var ssid=document.getElementById('wifi-ssid'); var psk=document.getElementById('wifi-psk'); xhr('GET', '/bblock-get-uuid').then((uuidStr)=>{uuid.value=uuidStr;}); xhr('GET', '/bblock-get-hostname').then((hostnameStr)=>{hostname.value=hostnameStr;}); xhr('GET', '/bblock-get-wifi-ssid').then((ssidStr)=>{ssid.value=ssidStr;}); xhr('GET', '/bblock-get-wifi-psk').then((pskStr)=>{psk.value=pskStr;}); var updateButton=document.getElementById('update-btn'); updateButton.addEventListener('click', ()=>{var payload={uuid: uuid.value, hostname: hostname.value, ssid: ssid.value, psk: psk.value}; xhr('POST', '/bblock-update-config', JSON.stringify(payload), 'application/json');}); var generateUUIDButton=document.getElementById('generate-uuid'); generateUUIDButton.addEventListener('click', ()=>{uuid.value=generateUUID();});}; </script></body></html>";
}

char *BasicBlock::getUUID()
{
	return config.UUID;
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
	strncpy(newConfig.UUID, root["uuid"], 36);
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
