#include <ESP8266WiFi.h>

/** Input from PIR sensor */
#define pirPort 4
/** Output to activate Relay */
#define relayPort 5
/** Output to loudspeaker or piezo */
// #define speakerPin 15
#define speakerPin 12
/** Definition of data pin for DHT sensor */
#define DHTPIN 14
/** Definition of DHT sensor type */
#define DHTTYPE DHT11

/**********************************************
When doing breadboard test, enable this define
***********************************************/
//#define BREADBOARD

#ifdef BREADBOARD
	#define DEVICE_ID "sbb" // ID for security in front yard
	#define OTA_HOST "sbb" // Host name for OTA updates
#else
	#define DEVICE_ID "sb1" // ID for security in front yard
	#define OTA_HOST "sb1" // Host name for OTA updates
#endif

/** Build time */
extern const char compileDate [];
/** WiFiServer class to create TCP socket server on port tcpComPort */
extern WiFiServer tcpServer;
/** FTPServer class to create simple ftp server */
extern FtpServer ftpSrv;
/** IP address of this module */
extern IPAddress ipAddr;

/** Timer to switch off the relay */
extern Ticker relayOffTimer;
/** Timer for alarm siren */
extern Ticker alarmTimer;

/** Flag for alarm activity */
extern boolean alarmOn;
/** Flag if heart beat was triggered */
extern boolean heartBeatTriggered;
/** Flag if panic button was pressed */
extern boolean panicOn;
/** Flag for debugging */
extern bool debugOn;
/** Relay on delay time in seconds */
extern int onTime;
/** Flag for WiFi connection */
extern bool wmIsConnected;
/** Bug capture trial year of last good NTP time received */
extern int lastKnownYear;

/** Flag if lights should be switched on after movement detection */
extern boolean switchLights;
/** Flag for PIR status change */
extern boolean pirTriggered;
/** Flag for request to read out light sensor */
extern boolean weatherUpdateTriggered;
/** Flag for detection status */
extern boolean hasDetection;
/** Flag for light switched off */
extern boolean lightOffTriggered;

/** Flag for boot status */
extern boolean inSetup;
/** String with reboot reason */
extern String rebootReason;
/** String with last known reboot reason */
extern String lastRebootReason;

/** Instance of the DHT sensor */
extern DHT dht; // 11 works fine for ESP8266
/** Result of last temperature measurement */
extern float tempValue;
/** Result of last humidity measurement */
extern float humidValue;

/** Flag for OTA update */
extern bool otaRunning;

/** Flag for automatic activation/deactivation of alarm */
extern boolean hasAutoActivation;
/** Hour for automatic activation of alarm (24h format) */
extern int autoActivOn;
/** Hour for automatic deactivation of alarm (24h format) */
extern int autoActivOff;
