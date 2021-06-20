//
// Boilerplate for configurable web server (probably RESTful) running on ESP8266.
//
// The server always includes the Wifi configuration module. You can enable
// other modules with the preprocessor defines. With the default defines the server
// will allow serving of web pages and other documents, and of uploading those.
//

#include "iotsa.h"
#include "iotsaWifi.h"

// CHANGE: Add application includes and declarations here

#undef WITH_NTP    // Use network time protocol to synchronize the clock.
#define WITH_OTA    // Enable Over The Air updates from ArduinoIDE. Needs at least 1MB flash.
#undef WITH_FILES  // Enable static files webserver
#undef WITH_FILESUPLOAD  // Enable upload of static files for webserver
#undef WITH_FILESBACKUP  // Enable backup of all files including config files and webserver files

IotsaApplication application("Iotsa Data Logger Server");
IotsaWifiMod wifiMod(application);

#ifdef WITH_NTP
#include "iotsaNtp.h"
IotsaNtpMod ntpMod(application);
#endif

#ifdef WITH_OTA
#include "iotsaOta.h"
IotsaOtaMod otaMod(application);
#endif

#ifdef WITH_FILES
#include "iotsaFiles.h"
IotsaFilesMod filesMod(application);
#endif

#ifdef WITH_FILESUPLOAD
#include "iotsaFilesUpload.h"
IotsaFilesUploadMod filesUploadMod(application);
#endif

#ifdef WITH_FILESBACKUP
#include "iotsaFilesBackup.h"
IotsaFilesBackupMod filesBackupMod(application);
#endif

#include "iotsaDataLogger.h"
IotsaDataLoggerMod iotsaDataLoggerMod(application);

void setup(void){
  application.setup();
  application.serverSetup();
#ifndef ESP32
  ESP.wdtEnable(WDTO_120MS);
#endif
}
 
void loop(void){
  application.loop();
}

