//
// Configurable web/REST server that logs an analog input over time.
//

#include "iotsa.h"
#include "iotsaNtp.h"
#include "iotsaRtc.h"
#include "iotsaFilesBackup.h"
#include "iotsaDataLogger.h"
#include "iotsaAnnotations.h"

// DS1302 RTC wiring
#define PIN_ENA 23
#define PIN_CLK 21
#define PIN_DAT 22

IotsaApplication application("Iotsa Data Logger Server");
IotsaNtpMod ntpMod(application);                                  // network time
IotsaRtcMod rtcMod(application, PIN_ENA, PIN_CLK, PIN_DAT);        // battery-backed time
IotsaFilesBackupMod filesBackupMod(application);                  // config backup/restore
IotsaDataLoggerMod iotsaDataLoggerMod(application);
IotsaAnnotationsMod annotationsMod(application);                  // free-form key/value info for tools

void setup(void){
  application.setup();
  application.lateSetup();
}

void loop(void){
  application.loop();
}
