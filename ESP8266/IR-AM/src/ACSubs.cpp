#include <Setup.h>

/** Power consumption for decision to send a broadcast */
double consPowerOld = 0;
/** Solar panel production from spMonitor */
double solarPower = 0;
/** Collected solar production of last 10 minutes of the house from spMonitor */
double avgSolarPower[10] = {0,0,0,0,0,0,0,0,0,0};
/** Pointer to element in avgConsPower[] */
byte avgConsIndex = 0;

/**
	handleCmd
	Handles commands received over the webserver
*/
void handleCmd() {
	actLedFlashStart(0.1);
	byte lastTemp;
String debugRPiMsg;
	// Handle the different AC commands
	switch (irCmd) {
		case CMD_REMOTE_0: // Should only be received in slave AC
			// Set remote AC into powerStatus 0 and switch it off
			if ((acMode & AUTO_ON) == AUTO_ON) { // Handle only if Auto mode is on!
				if ((acMode & AC_ON) == AC_ON) { // AC is on
					acMode = acMode & MODE_CLR; // set mode bits to 0
					acMode = acMode | MODE_FAN; // set mode bits to FAN mode
					My_Sender.sendNEC(Fan,32);
					delay(1000);
					acMode = acMode & AC_CLR; // set power bit to 0
					acMode = acMode | AC_OFF; // set power bit to OFF
					My_Sender.sendNEC(On_Off,32);
				}
				powerStatus = 0;
				writeStatus();
			}
			break;
		case CMD_REMOTE_1: // Should only be received in slave AC
			// Set remote AC into powerStatus 1 and switch it on into FAN mode
			if ((acMode & AUTO_ON) == AUTO_ON) { // Handle only if Auto mode is on!
				if ((acMode & AC_ON) != AC_ON) { // AC is off
					acMode = acMode & AC_CLR; // set power bit to 0
					acMode = acMode | AC_ON; // set power bit to ON
					My_Sender.sendNEC(On_Off,32);
					delay(1000);
				}
				acMode = acMode & MODE_CLR; // set mode bits to 0
				acMode = acMode | MODE_FAN; // set mode bits to FAN mode
				My_Sender.sendNEC(Fan,32);
				powerStatus = 1;
				writeStatus();
			}
			break;
		case CMD_REMOTE_2: // Should only be received in slave AC
			// Set remote AC into powerStatus 2 and switch it on into COOL mode
			if ((acMode & AUTO_ON) == AUTO_ON) { // Handle only if Auto mode is on!
				if ((acMode & AC_ON) != AC_ON) {
					acMode = acMode & AC_CLR; // set power bit to 0
					acMode = acMode | AC_ON; // set power bit to ON
					My_Sender.sendNEC(On_Off,32); // AC is off
					delay(1000);
				}
				acMode = acMode & MODE_CLR; // set mode bits to 0
				acMode = acMode | MODE_DRY; // set mode bits to DRY mode
				My_Sender.sendNEC(Dry,32); // Auto mode not available on FD
				powerStatus = 2;
				writeStatus();
			}
			break;
		case CMD_INIT_AC: // Initialize aircon
			initAC();
			break;
		case CMD_AUTO_ON: // Command to (re)start auto control
			// If AC is on, switch it to FAN low speed and then switch it off
			if ((acMode & AC_ON) == AC_ON) { // AC is on
				// Set mode to FAN
				acMode = acMode & MODE_CLR; // set mode bits to 0
				acMode = acMode | MODE_FAN; // set mode bits to FAN mode
				My_Sender.sendNEC(Fan,32);
				delay(1000);
				// Set fan speed to LOW
				acMode = acMode & FAN_CLR; // set fan bits to 0
				acMode = acMode | FAN_LOW; // set mode bits to FAN LOW mode
				My_Sender.sendNEC(L_Fan,32);
				delay(1000);
				// Switch AC off
				acMode = acMode & AC_CLR; // set power bit to 0
				acMode = acMode | AC_OFF; // set power bit to OFF
				My_Sender.sendNEC(On_Off,32);
			}
			powerStatus = 0; // Independendant from current status it will be set to 0!
			acMode = acMode & AUTO_CLR;
			acMode = acMode | AUTO_ON;
			break;
		case CMD_AUTO_OFF: // Command to stop auto control
			acMode = acMode & AUTO_CLR;
			acMode = acMode | AUTO_OFF;
			powerStatus = 0; // Independendant from current status it will be set to 0!
			break;
		case CMD_ON_OFF: // On-Off
			if ((acMode & AC_ON) == AC_ON) { // AC is on
				acMode = acMode & AC_CLR; // set power bit to 0
				acMode = acMode | AC_OFF; // set power bit to OFF
			} else { // AC is off
				acMode = acMode & AC_CLR; // set power bit to 0
				acMode = acMode | AC_ON; // set power bit to ON
			}
			My_Sender.sendNEC(On_Off,32);
			break;
		case CMD_MODE_COOL: // Cool
			if ((acMode & AC_ON) == AC_ON) {
				acMode = acMode & MODE_CLR; // set mode bits to 0
				acMode = acMode | MODE_COOL; // set mode bits to COOL mode
				My_Sender.sendNEC(Cool,32);
			}
			delay(1000);
debugRPiMsg = "ACSubs - handleCmd - CMD_MODE_COOL before restoreTempSetting - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugRPiMsg,"SF1");
			restoreTempSetting();
debugRPiMsg = "ACSubs - handleCmd - CMD_MODE_COOL after restoreTempSetting - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugRPiMsg,"SF1");
			break;
		case CMD_MODE_DRY: // Dry
			if ((acMode & AC_ON) == AC_ON) {
				if (!savedAcTempByDry) {
					savedAcTemp = acTemp & TEMP_MASK;
					acTemp = acTemp & TEMP_CLR; // Dry mode resets AC temperature to 25
					acTemp += 25;
					savedAcTempByDry = true;
debugRPiMsg = "ACSubs - handleCmd - CMD_MODE_DRY - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugRPiMsg,"SF1");
				}
				acMode = acMode & MODE_CLR; // set mode bits to 0
				acMode = acMode | MODE_DRY; // set mode bits to DRY mode
				My_Sender.sendNEC(Dry,32);
			}
			break;
		case CMD_MODE_FAN: // Fan
			if ((acMode & AC_ON) == AC_ON) {
				acMode = acMode & MODE_CLR; // set mode bits to 0
				acMode = acMode | MODE_FAN; // set mode bits to FAN mode
				My_Sender.sendNEC(Fan,32);
			}
			break;
		case CMD_FAN_HIGH: // H-Fan
			if ((acMode & AC_ON) == AC_ON) {
				acMode = acMode & FAN_CLR; // set fan bits to 0
				acMode = acMode | FAN_HIGH; // set mode bits to FAN HIGH mode
				My_Sender.sendNEC(H_Fan,32);
			}
			break;
		case CMD_FAN_MED: // M-Fan
			if ((acMode & AC_ON) == AC_ON) {
				acMode = acMode & FAN_CLR; // set fan bits to 0
				acMode = acMode | FAN_MED; // set mode bits to FAN MEDIUM mode
				My_Sender.sendNEC(M_Fan,32);
			}
			break;
		case CMD_FAN_LOW: // L-Fan
			if ((acMode & AC_ON) == AC_ON) {
				acMode = acMode & FAN_CLR; // set fan bits to 0
				acMode = acMode | FAN_LOW; // set mode bits to FAN LOW mode
				My_Sender.sendNEC(L_Fan,32);
			}
			break;
		case CMD_TEMP_PLUS: // Temp +
			if ((acMode & AC_ON) == AC_ON) {
				if ((acMode & MODE_COOL) == MODE_COOL) {
					lastTemp = acTemp & TEMP_MASK;
					if (lastTemp < MAX_TEMP) {
						lastTemp++;
						acTemp = acTemp & TEMP_CLR;
						acTemp += lastTemp;
					} else {
						acTemp = acTemp & TEMP_CLR;
						acTemp = MAX_TEMP;
					}
debugRPiMsg = "ACSubs - handleCmd - CMD_TEMP_PLUS - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugRPiMsg,"SF1");
					My_Sender.sendNEC(Plus,32);
				}
			}
			break;
		case CMD_TEMP_MINUS: // Temp -
			if ((acMode & AC_ON) == AC_ON) {
				if ((acMode & MODE_COOL) == MODE_COOL) {
					lastTemp = acTemp & TEMP_MASK;
					if (lastTemp > MIN_TEMP) {
						lastTemp--;
						acTemp = acTemp & TEMP_CLR;
						acTemp += lastTemp;
					} else {
						acTemp = acTemp & TEMP_CLR;
						acTemp = MIN_TEMP;
					}
debugRPiMsg = "ACSubs - handleCmd - CMD_TEMP_MINUS - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugRPiMsg,"SF1");
					My_Sender.sendNEC(Minus,32);
				}
			}
			break;
		case CMD_OTHER_TIMER: // Timer
			if ((acMode & TIM_ON) == TIM_ON) { // TIMER is already on
				timerEndTimer.detach(); // Stop timer
				timerEndTriggered = false;
				acMode = acMode & TIM_CLR; // set timer bit to 0
				// Switch off the aircon
				if ((acMode & AC_ON) == AC_ON) { // // Switch off the aircon if still on
					// Set mode to FAN
					acMode = acMode & MODE_CLR; // set mode bits to 0
					acMode = acMode | MODE_FAN; // set mode bits to FAN mode
					My_Sender.sendNEC(Fan,32);
					delay(1000);
					// Set fan speed to LOW
					acMode = acMode & FAN_CLR; // set fan bits to 0
					acMode = acMode | FAN_LOW; // set mode bits to FAN LOW mode
					My_Sender.sendNEC(L_Fan,32);
					delay(1000);
					// Switch AC off
					acMode = acMode & AC_CLR; // set power bit to 0
					acMode = acMode | AC_OFF; // set power bit to OFF
					My_Sender.sendNEC(On_Off,32);
					delay(1000);
					if (debugOn) {
						String debugMsg = "Timer stopped manually, switch off AC (" + String(hour()) + ":" + getDigits(minute()) + ")";
						sendDebug(debugMsg, OTA_HOST);
					}
				} else {
					if (debugOn) {
						String debugMsg = "Timer stopped manually, AC was already off (" + String(hour()) + ":" + getDigits(minute()) + ")";
						sendDebug(debugMsg, OTA_HOST);
					}
				}
				powerStatus = 0;
			} else { // Timer is not yet on
				acMode = acMode & TIM_CLR; // set timer bit to 0
				acMode = acMode | TIM_ON; // set timer bit to 1 (on)
				// Switch on the aircon
				if (!((acMode & AC_ON) == AC_ON)) { // // Switch on the aircon if off
					// Switch AC on
					acMode = acMode & AC_CLR; // set power bit to 0
					acMode = acMode | AC_ON; // set power bit to ON
					My_Sender.sendNEC(On_Off,32);
					delay(1000);
				}
				// Set fan speed to LOW
				acMode = acMode & FAN_CLR; // set fan bits to 0
				acMode = acMode | FAN_LOW; // set mode bits to FAN LOW mode
				My_Sender.sendNEC(L_Fan,32);
				delay(1000);
				// Set mode to cooling
				acMode = acMode & MODE_CLR; // set mode bits to 0
				acMode = acMode | MODE_COOL; // set mode bits to COOL mode
				My_Sender.sendNEC(Cool,32);
				delay(1000);
debugRPiMsg = "ACSubs - handleCmd - CMD_OTHER_TIMER before restoreTempSetting - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugRPiMsg,"SF1");
				restoreTempSetting();
debugRPiMsg = "ACSubs - handleCmd - CMD_OTHER_TIMER after restoreTempSetting - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugRPiMsg,"SF1");
				powerStatus = 0;
				// Start timer to switch off the aircon after "onTime" (default = 1 hour = 60m x 60s x 1000ms=3600000 milliseconds)
				// Because of 32bit limit (== max 71min) the timer is triggered
				// every hour and a counter is used if timer time is 2 hours or more
				timerCounter = 0;
				timerEndTimer.attach_ms(3600000, triggerTimerEnd);
				int offHour = hour()+onTime;
				if (offHour >= 24) {
					offHour-=24;
				}
				timerEnd = getDigits(offHour) + ":" + getDigits(minute());
				if (debugOn) {
					String debugMsg = "Start of timer, switch on AC (" + timerEnd + ") for " + onTime + "hours";
					sendDebug(debugMsg, OTA_HOST);
				}
			}
			break;
		case CMD_OTHER_TURBO: // Switch on/off turbo (Not avail on FD1)
			break;
		case CMD_OTHER_ION: // Switch on/off ion (Not avail on FD1)
			break;
		case CMD_RESET: // Command to reset device
			writeStatus(); // Save last status before reset
			// Reset the ESP
			delay(3000);
			if (debugOn) {
				sendDebug("Request to reset the module", OTA_HOST);
			}
			pinMode(16, OUTPUT); // Connected to RST pin
			digitalWrite(16,LOW); // Initiate reset
			ESP.reset();
			delay(5000);
			break;
		default: // Invalid commands are ignored
			break;
	}
	irCmd = 9999; // No further actions necessary
	actLedFlashStop();
	writeStatus(); // Save status changes
}

/**
	initAC
	Initialize aircon to fan mode, low fan speed, temperature 25 deg C
	Call this only once after power on and make sure aircon is off when
	ESP8266 is powered off!
	This will fail if aircon is already on and there is no (easy) way
	to detect if the aircon is already on
*/
void initAC() {
	doubleLedFlashStart(0.1);

	/* FIRST: switch on AC */
	acMode = acMode & AC_CLR; // set power bit to 0
	acMode = acMode | AC_ON;
	My_Sender.sendNEC(On_Off,32);
	delay(2000); // Wait 2 seconds to make sure the aircon is on

	/* SECOND: switch to COOL mode */
	acMode = acMode & MODE_CLR; // set mode bits to 0
	acMode = acMode | MODE_COOL; // set mode bits to COOL mode
	My_Sender.sendNEC(Cool,32);
	delay(1000); // Wait 1 second to make sure the aircon mode is switched

	/* THIRD: switch to LOW FAN speed */
	acMode = acMode & FAN_CLR; // set fan bits to 0
	acMode = acMode | FAN_LOW; // set mode bits to FAN LOW mode
	My_Sender.sendNEC(L_Fan,32);
	delay(1000); // Wait 1 second to make sure the aircon mode is switched

	/* FORTH: set temperature to 25 deg Celsius */
	acTemp = acTemp & TEMP_CLR; // set temperature bits to 0
	acTemp = acTemp + 25; // set temperature bits to 25
	/* We do not know the temperature setting of the aircon
			therefor we first raise 17 times (to set 32 degrees)
			then we lower 7 times to get back to 25 degrees
	*/
	for (int i = 0; i < 16; i++) {
		My_Sender.sendNEC(Plus,32);
		delay(1000); // Wait 1 second to make sure the aircon mode is switched
	}
	for (int i = 0; i < 7; i++) {
		My_Sender.sendNEC(Minus,32);
		delay(1000); // Wait 1 second to make sure the aircon mode is switched
	}

	/* FIFTH: switch to FAN mode */
	acMode = acMode & MODE_CLR; // set mode bits to 0
	acMode = acMode | MODE_FAN; // set mode bits to FAN mode
	My_Sender.sendNEC(Fan,32);
	delay(1000); // Wait 1 second to make sure the aircon mode is switched

	/* SIXTH: switch AC off */
	acMode = acMode & AC_CLR; // set status bits to 0
	acMode = acMode | AC_OFF; // set status to aircon off
	My_Sender.sendNEC(On_Off,32);
	writeStatus();
	irCmd = 9999; // No further actions necessary
	doubleLedFlashStop();
String debugMsg = "ACSubs - initAC - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugMsg,"SF1");
}

/**
	switchSlaveAC
	Puts slave AC into requested mode
*/
boolean switchSlaveAC(IPAddress ipSlave, byte reqMode) {
	comLedFlashStart(0.1);
	const int httpPort = 80;
	if (!tcpClientOut.connect(ipSlave, httpPort)) {
		if (debugOn) {
			String debugMsg = "connection to slave AC " + String(ipSlave[0]) + "." + String(ipSlave[1]) + "." + String(ipSlave[2]) + "." + String(ipSlave[3]) + " failed";
			sendDebug(debugMsg, OTA_HOST);
		}
		tcpClientOut.stop();
		comLedFlashStop();
		return false;
	}

	switch (reqMode) {
		case CMD_REMOTE_0:
			tcpClientOut.print("GET /?c=" + String(CMD_REMOTE_0) + " HTTP/1.0\r\n\r\n");
			break;
		case CMD_REMOTE_1:
			tcpClientOut.print("GET /?c=" + String(CMD_REMOTE_1) + " HTTP/1.0\r\n\r\n");
			break;
		case CMD_REMOTE_2:
			tcpClientOut.print("GET /?c=" + String(CMD_REMOTE_2) + " HTTP/1.0\r\n\r\n");
			break;
	}

	String line = "";
	int waitTimeOut = 0;
	while (tcpClientOut.connected()) {
		line = tcpClientOut.readStringUntil('\r');
		delay(1);
		waitTimeOut++;
		if (waitTimeOut > 2000) { // If no more response for 2 seconds return
			tcpClientOut.stop();
			comLedFlashStop();
			return false;
		}
	}
	tcpClientOut.stop();
	/** Buffer for JSON string */
	DynamicJsonBuffer jsonBuffer;
	/** JSON object with parsed JSON data */
	JsonObject& root = jsonBuffer.parseObject(line,2);
	if (!root.success()) {
		if (debugOn) {
			String debugMsg = "parseObject() failed";
			sendDebug(debugMsg, OTA_HOST);
		}
		comLedFlashStop();
		return false;
	}

	if (root["re"] == "ok") { //if (root["result"] == "success") {
		comLedFlashStop();
		return true;
	}

	comLedFlashStop();
	return false;
}

/**
	checkPower
	Uses the power production of the solar panel to switch on/to/off
	aircon in fan or cool mode
*/
void checkPower() {
	/** Flag if we need to send update of status */
	boolean mustBroadcast = false;
	/** Flag if we need to save a new status */
	boolean mustWriteStatus = false;
	/** Calculate average power consumption of the last 10 minutes */
	consPower = 0;

	for (int i = 0; i < 10; i++) {
		consPower += avgConsPower[i];
	}
	consPower = consPower / 10;
	if (debugOn) {
		String debugMsg = "checkPower calculated consPower was " + String(consPower);
		sendDebug(debugMsg, OTA_HOST);
	}

	/** Calculate average solar production of the last 10 minutes */
	solarPower = 0;
	for (int i = 0; i < 10; i++) {
		solarPower += avgSolarPower[i];
	}
	if (debugOn) {
		String debugMsg = "checkPower calculated solarPower was " + String(solarPower/10);
		sendDebug(debugMsg, OTA_HOST);
	}

	/***************************************************/
	/* Below code is for use with one aircon */
	/***************************************************/
	// Check if the solar production of the last 10 minutes was 0 Watt
	if (solarPower == 0) { // No solar panel production
		if (powerStatus != 0) {
			if ((acMode & AC_ON) == AC_ON) {
				acMode = acMode & MODE_CLR; // set mode bits to 0
				acMode = acMode | MODE_FAN; // set mode bits to FAN mode
				My_Sender.sendNEC(Fan,32);
				delay(1000);
				acMode = acMode & AC_CLR; // set power bit to 0
				acMode = acMode | AC_OFF; // set power bit to OFF
				My_Sender.sendNEC(On_Off,32);
				powerStatus = 0;
				mustBroadcast = true;
				mustWriteStatus = true;
				avgConsIndex = 0; // reset average calculation
				if (debugOn) {
					String debugMsg = "Status was " + String(powerStatus) + ", Solar power == 0, switch aircon off, status to 0";
					sendDebug(debugMsg, OTA_HOST);
				}
			} else {
				powerStatus = 0;
				mustBroadcast = true;
				mustWriteStatus = true;
				avgConsIndex = 0; // reset average calculation
				if (debugOn) {
					String debugMsg = "Status was " + String(powerStatus) + ", Solar power == 0, aircon was already off, switch status to 0";
					sendDebug(debugMsg, OTA_HOST);
				}
			}
		}
	} else {
		/** Check current status */
		switch (powerStatus) {
			case 0: // aircon is off
				// at -300 switch directly to cool mode
				// at -75 switch to fan mode
				if (consPower < -300) { // production exceeds 300W
					powerStatus = 3;
					if (debugOn) {
						String debugMsg = "Status was 0, consumption < -300W, switch aircon to cool mode, status to 3";
						sendDebug(debugMsg, OTA_HOST);
					}
				} else if (consPower < -75) { // production exceeds 75W
					powerStatus = 1;
					if (debugOn) {
						String debugMsg = "Status was 0, consumption < -75W, switch aircon to fan mode, status to 1";
						sendDebug(debugMsg, OTA_HOST);
					}
				}
				if (powerStatus != 0) {
					if ((acMode & AC_ON) != AC_ON) { // Aircon might be already on (manually)
						acMode = acMode & AC_CLR; // set power bit to 0
						acMode = acMode | AC_ON; // set power bit to ON
						My_Sender.sendNEC(On_Off,32);
						delay(1000);
					}
					if (powerStatus == 3) {
						acMode = acMode & MODE_CLR; // set mode bits to 0
						acMode = acMode | MODE_COOL; // set mode bits to COOL mode
						My_Sender.sendNEC(Cool,32);
						delay(1000);
String debugMsg = "ACSubs - checkPower case 0 before restoreTempSetting - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugMsg,"SF1");
						restoreTempSetting();
debugMsg = "ACSubs - checkPower case 0 after restoreTempSetting - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugMsg,"SF1");
					} else {
						acMode = acMode & MODE_CLR; // set mode bits to 0
						acMode = acMode | MODE_FAN; // set mode bits to FAN mode
						My_Sender.sendNEC(Fan,32);
					}

					mustBroadcast = true;
					mustWriteStatus = true;
					avgConsIndex = 0; // reset average calculation
				}
				break;
			case 1: // aircon is in fan mode
				// at +200 switch off aircon
				// at -300 switch to cool mode
				if (consPower > 200) { // consuming more than 200W
					powerStatus = 0;
					if (debugOn) {
						String debugMsg = "Status was 1, consumption > 200W, switch aircon off, status to 0";
						sendDebug(debugMsg, OTA_HOST);
					}
				} else if (consPower < -300) { // production exceeds 300W
					powerStatus = 3;
					if (debugOn) {
						String debugMsg = "Status was 1, consumption < -300W, switch aircon to cool mode, status to 3";
						sendDebug(debugMsg, OTA_HOST);
					}
				}
				if (powerStatus != 1) {
					if (powerStatus == 0) {
						acMode = acMode & AC_CLR; // set power bit to 0
						acMode = acMode | AC_OFF; // set power bit to OFF
						My_Sender.sendNEC(On_Off,32);
					} else {
						acMode = acMode & MODE_CLR; // set mode bits to 0
						acMode = acMode | MODE_COOL; // set mode bits to COOL mode
						My_Sender.sendNEC(Cool,32);
						delay(1000);
String debugMsg = "ACSubs - checkPower case 1 before restoreTempSetting - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugMsg,"SF1");
						restoreTempSetting();
debugMsg = "ACSubs - checkPower case 1 after restoreTempSetting - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugMsg,"SF1");
					}
					mustBroadcast = true;
					mustWriteStatus = true;
					avgConsIndex = 0; // reset average calculation
				}
				break;
			case 2: // aircon is in dry mode
				// at +300 switch to fan mode
				// at -75 switch to cool mode
				if (consPower > 300) { // consuming more than 300W
					powerStatus = 1;
					if (debugOn) {
						String debugMsg = "Status was 2, consumption > 300W, switch aircon to fan mode, status to 1";
						sendDebug(debugMsg, OTA_HOST);
					}
				} else if (consPower < -75) { // over production more than 75W
					powerStatus = 3;
					if (debugOn) {
						String debugMsg = "Status was 2, consumption < -75W, switch aircon to cool mode, status to 3";
						sendDebug(debugMsg, OTA_HOST);
					}
				}
				if (powerStatus != 2) { // If changed send command to air con
					if (powerStatus == 1) {
						acMode = acMode & MODE_CLR; // set mode bits to 0
						acMode = acMode | MODE_FAN; // set mode bits to FAN mode
						My_Sender.sendNEC(Fan,32);
					} else {
						acMode = acMode & MODE_CLR; // set mode bits to 0
						acMode = acMode | MODE_COOL; // set mode bits to COOL mode
						My_Sender.sendNEC(Cool,32);
						delay(1000);
String debugMsg = "ACSubs - checkPower case 2 before restoreTempSetting - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugMsg,"SF1");
						restoreTempSetting();
debugMsg = "ACSubs - checkPower case 2 after restoreTempSetting - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugMsg,"SF1");
					}
					mustBroadcast = true;
					mustWriteStatus = true;
					avgConsIndex = 0; // reset average calculation
				}
				break;
			case 3: // aircon is in cool mode
				// at +400 switch to dry mode
				if (consPower > 400) { // consuming more than 400W
					acMode = acMode & MODE_CLR; // set mode bits to 0
					acMode = acMode | MODE_DRY; // set mode bits to DRY mode
					My_Sender.sendNEC(Dry,32);
					if (!savedAcTempByDry) {
						savedAcTemp = acTemp & TEMP_MASK;
						acTemp = acTemp & TEMP_CLR; // Dry mode resets AC temperature to 25
						acTemp += 25;
						savedAcTempByDry = true;
					}
					powerStatus = 2;
					mustBroadcast = true;
					mustWriteStatus = true;
					avgConsIndex = 0; // reset average calculation
					if (debugOn) {
						String debugMsg = "Status was 3, consumption > 400W, switch aircon to dry mode, status to 2";
						sendDebug(debugMsg, OTA_HOST);
					}
String debugMsg = "ACSubs - checkPower case 3 - acTemp=" + String(acTemp) + " - saved=" + String(savedAcTemp);
sendRpiDebug(debugMsg,"SF1");
				}
				break;
		}
	}
	if (mustWriteStatus) {
		// Save status change
		writeStatus();
	}
	if (mustBroadcast) {
		// Broadcast status change
		sendBroadCast();
	}
}

/**
	getPowerVal
	Get current power consumption from spMonitor device on address ipSPM
*/
void getPowerVal(boolean doPowerCheck) {
	String debugMsg;
	comLedFlashStart(0.1);
	const int httpPort = 80;
	if (!tcpClientOut.connect(ipSPM, httpPort)) {
		if (debugOn) {
			String debugMsg = "connection to spm " + String(ipSPM[0]) + "." + String(ipSPM[1]) + "." + String(ipSPM[2]) + "." + String(ipSPM[3]) + " failed";
			sendDebug(debugMsg, OTA_HOST);
		}
		tcpClientOut.stop();
		comLedFlashStop();
		return;
	}

	tcpClientOut.print("GET /data/get HTTP/1.0\r\n\r\n");

	String line = "";
	int waitTimeOut = 0;
	while (tcpClientOut.connected()) {
		line = tcpClientOut.readStringUntil('\r');
		delay(1);
		waitTimeOut++;
		if (waitTimeOut > 2000) { // If no more response for 2 seconds return
			comLedFlashStop();
			return;
		}
	}
	tcpClientOut.stop();
	/** Buffer for JSON string */
	DynamicJsonBuffer jsonBuffer;
	/** JSON object with parsed JSON data */
	JsonObject& root = jsonBuffer.parseObject(line,2);
	if (!root.success()) {
		if (debugOn) {
			sendDebug("parseObject() failed", OTA_HOST);
		}
		comLedFlashStop();
		return;
	}
	comLedFlashStop();

	// Switch between status depending on consumption
	if (root.containsKey("value")) {
		consPower = root["value"]["C"];
		solarPower = root["value"]["S"];
		if (avgConsIndex < 10) {
			avgConsPower[avgConsIndex] = consPower;
			avgSolarPower[avgConsIndex] = solarPower;
			avgConsIndex++;
			if (avgConsIndex != 10) {
				if (debugOn) {
					String debugMsg = "Status " + String(powerStatus) + ", still collecting data, counter = " + String(avgConsIndex);
					sendDebug(debugMsg, OTA_HOST);
				}
			} else {
				if (doPowerCheck && (acMode & AUTO_MASK) == AUTO_ON && dayTime && (acMode & TIM_MASK) == TIM_OFF) {
					checkPower();
					if (debugOn) {
						String debugMsg = "Status " + String(powerStatus) + ", still collecting data, counter = " + String(avgConsIndex);
						sendDebug(debugMsg, OTA_HOST);
					}
				}
			}
		} else {
			for (int i = 0; i < 9; i++) { // Shift values in array
				avgConsPower[i] = avgConsPower[i + 1];
				avgSolarPower[i] = avgSolarPower[i + 1];
			}
			avgConsPower[9] = consPower;
			avgSolarPower[9] = solarPower;

			// If deviation to old power is bigger than 50 Watt send a broadcast
			double powerChange = consPowerOld - consPower;
			if (abs(powerChange) > 100) {
				if (debugOn) {
					String debugMsg = "Status " + String(powerStatus) + " C:" + String(consPower, 0) + " S:" + String(solarPower, 0);
					sendDebug(debugMsg, OTA_HOST);
				}
				sendBroadCast();
				consPowerOld = consPower;
			}
			if (debugOn) {
				String debugMsg = "doPowerCheck = " + String(doPowerCheck) + " Auto:" + String((acMode & AUTO_MASK)) + " dayTime = " + String(dayTime) + " Timer:" + String((acMode & TIM_MASK));
				sendDebug(debugMsg, OTA_HOST);
			}
			if (doPowerCheck && (acMode & AUTO_MASK) == AUTO_ON && dayTime && (acMode & TIM_MASK) == TIM_OFF) {
				if (debugOn) {
					sendDebug("Do checkPower now", OTA_HOST);
				}
				checkPower();
			}
		}
	}
}

/**
	Restore temperature setting after DRY mode
*/
void restoreTempSetting() {
	/** Debug message */
	String debugMsg;
	/** Command for temperature restore */
	int tempCmd;

	if (debugOn) {
		String debugMsg = "Current temperature setting = " + String(acTemp & TEMP_MASK);
		sendDebug(debugMsg, OTA_HOST);
		debugMsg = "Saved temperature setting = " + String(savedAcTemp);
		sendDebug(debugMsg, OTA_HOST);
	}
	byte tempChange = 0;
	if (savedAcTempByDry) { // Temperature changed because of switch to DRY mode?
		if ((acTemp & TEMP_MASK) > savedAcTemp) { // current temp higher than saved temp
			tempChange =  (acTemp & TEMP_MASK) - savedAcTemp;
			tempCmd = Minus;
			// tempCmd = CMD_TEMP_MINUS;
			if (debugOn) {
				String debugMsg = "Temp restore with CMD_TEMP_MINUS - Difference = " + String(tempChange);
				sendDebug(debugMsg, OTA_HOST);
			}
		} else if ((acTemp & TEMP_MASK) < savedAcTemp) { // current temp lower than saved temp
			tempChange =  savedAcTemp - (acTemp & TEMP_MASK);
			tempCmd = Plus;
			// tempCmd = CMD_TEMP_PLUS;
			if (debugOn) {
				String debugMsg = "Temp restore with CMD_TEMP_PLUS - Difference = " + String(tempChange);
				sendDebug(debugMsg, OTA_HOST);
			}
		}
		for (int i=0; i<tempChange; i++) { // Reset old temperature
			My_Sender.sendNEC(tempCmd,32);
			delay(1000);
		}
		acTemp = acTemp & TEMP_CLR;
		acTemp = acTemp + savedAcTemp;
		savedAcTempByDry = false;
	}
}
