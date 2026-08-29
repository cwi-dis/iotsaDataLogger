//
// Configurable web/REST server that logs an analog input over time.
//
// The server always includes the Wifi configuration module. Other modules are
// enabled with the preprocessor defines below.
//

#include "iotsa.h"
#include "iotsaWifi.h"

#define WITH_NTP    // Use network time protocol to synchronize the clock.
#define WITH_RTC    // Backup time from an RTC module
#define WITH_OTA    // Enable Over The Air updates. Needs at least 1MB flash.
#undef WITH_FILES  // Enable static files webserver
#undef WITH_FILESUPLOAD  // Enable upload of static files for webserver
#define WITH_FILESBACKUP  // Enable backup of all files including config files and webserver files

IotsaApplication application("Iotsa Data Logger Server");
IotsaWifiMod wifiMod(application);

#ifdef WITH_NTP
#include "iotsaNtp.h"
IotsaNtpMod ntpMod(application);
#endif

#ifdef WITH_RTC
#define PIN_ENA 23
#define PIN_CLK 21
#define PIN_DAT 22

#include "iotsaRtc.h"
IotsaRtcMod rtcMod(application, PIN_ENA, PIN_CLK, PIN_DAT);
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
  application.lateSetup();
#ifndef ESP32
  ESP.wdtEnable(WDTO_120MS);
#endif
}

void loop(void){
  application.loop();
}
