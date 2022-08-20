#include "iotsaDataLogger.h"
#include "iotsaConfigFile.h"

#ifdef IOTSA_WITH_WEB
void
IotsaDataLoggerMod::handler() {
  bool archived = false;
  bool anyChanged = false;
  if( server->hasArg("interval")) {
    if (needsAuthentication()) return;
    String sInterval = server->arg("interval");
    interval = sInterval.toInt();
    anyChanged = true;
  }
  if( server->hasArg("adcMultiply")) {
    if (needsAuthentication()) return;
    String sv = server->arg("adcMultiply");
    adcMultiply = sv.toFloat();
    anyChanged = true;
  }
  if( server->hasArg("adcOffset")) {
    if (needsAuthentication()) return;
    String sv = server->arg("adcOffset");
    adcOffset = sv.toFloat();
    anyChanged = true;
  }
  if( server->hasArg("deepSleep")) {
    if (needsAuthentication()) return;
    String sv = server->arg("deepSleep");
    deepSleep = (bool)sv.toInt();
    anyChanged = true;
  }
  if (server->hasArg("forgetBefore")) {
    if (needsAuthentication()) return;
    String sv = server->arg("forgetBefore");
    if (sv != "") {
      timestamp_type ts = sv.toInt();
      store->forget(ts);
    }
  }
  if (server->hasArg("doArchive")) {
    if (needsAuthentication()) return;
    String sv = server->arg("doArchive");
    if (sv != "") {
      store->archive();
    }
  }
  if (server->hasArg("archived")) {
    String sv = server->arg("archived");
    if (sv != "") {
      archived = true;
    }
  }
  if (anyChanged) configSave();

  String message = "<html><head><title>Timed Data Logger Module</title></head><body><h1>Timed Data Logger Module</h1>";
  message += "<form method='get'><input type='submit' value='Refresh'><input type='checkbox' name='archived' value='1'>Show archived data in stead of current data</form><br>";

  message += "<h2>Acquisition settings</h2>";
  message += "<form method='get'>Interval (seconds): <input name='interval' value='";
  message += String(interval);
  message += "'><br>ADC multiplication factor: <input name='adcMultiply' value='";
  message += String(adcMultiply, 3);
  message += "'><br>ADC offset: <input name='adcOffset' value='";
  message += String(adcOffset, 3);
  message +="'><br><input type='checkbox' name='deepSleep' value='1'";
  if (deepSleep) message += " checked";
  message += ">Deep Sleep between acquisitions (unless WiFi is available)";
  message += "<br><input type='submit'></form>";

  message += "<h2>Acquisition buffer</h2>";
  message += "<form method='get'>Archive before (unix timestamp): <input name='forgetBefore'><input type='submit' value='Forget'></form><br>";
  message += "<form method='get'>Archive all data: <input type='hidden' name='doArchive' value='1'><input type='submit' value='Archive Data Store'></form><br>";
  store->toHTML(message, archived);
  message += "</body></html>";
  server->send(200, "text/html", message);
}

String IotsaDataLoggerMod::info() {
  String message = "<p>Timed data logger. See <a href=\"/datalogger\">/datalogger</a> for configuration, <a href=\"/api/datalogger\">/api/datalogger</a> for readings.</p>";
  return message;
}
#endif // IOTSA_WITH_WEB

bool IotsaDataLoggerMod::getHandler(const char *path, JsonObject& reply) {
  store->toJSON(reply);
  reply["interval"] = interval;
  reply["adcMultiply"] = adcMultiply;
  reply["adcOffset"] = adcOffset;
  reply["deepSleep"] = deepSleep;
  return true;
}

bool IotsaDataLoggerMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  if (!request.is<JsonObject>()) return false;
  JsonObject reqObj = request.as<JsonObject>();
  bool anyChanged = false;
  bool anySet = false;
  if (reqObj.containsKey("interval")) {
    interval = reqObj["interval"];
    anyChanged = true;
  }
  if (reqObj.containsKey("adcMultiply")) {
    adcMultiply = reqObj["adcMultiply"];
    anyChanged = true;
  }
  if (reqObj.containsKey("adcOffset")) {
    adcOffset = reqObj["adcOffset"];
    anyChanged = true;
  }
  if (reqObj.containsKey("deepSleep")) {
    deepSleep = reqObj["deepSleep"];
    anyChanged = true;
  }
  if (reqObj.containsKey("forgetBefore")) {
    timestamp_type ts = reqObj["forgetBefore"].as<timestamp_type>();
    store->forget(ts);
    anySet = true;
  }
  if (anyChanged) {
    configSave();
  }
  return anyChanged||anySet;
}

void IotsaDataLoggerMod::setup() {
  configLoad();
  //
  // The esp32 ADC seems to have pretty bad linearity.
  // Attempting to fix based on https://www.esp32.com/viewtopic.php?t=2881
  // and https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html
  //
  analogSetWidth(10);
  analogSetPinAttenuation(PIN_ANALOG_IN, ADC_6db);
}

void IotsaDataLoggerMod::serverSetup() {
#ifdef IOTSA_WITH_WEB
  server->on("/datalogger", std::bind(&IotsaDataLoggerMod::handler, this));
#endif
  api.setup("/api/datalogger", true, true);
  name = "datalogger";
}

void IotsaDataLoggerMod::configLoad() {
  IotsaConfigFileLoad cf("/config/datalogger.cfg");
  cf.get("interval", interval, 10);
  cf.get("adcMultiply", adcMultiply, 1);
  cf.get("adcOffset", adcOffset, 0);
  cf.get("deepSleep", deepSleep, false);
}

void IotsaDataLoggerMod::configSave() {
  IotsaConfigFileSave cf("/config/datalogger.cfg");
  cf.put("interval", interval);
  cf.put("adcMultiply", adcMultiply);
  cf.put("adcOffset", adcOffset);
  cf.put("deepSleep", deepSleep);
}

void IotsaDataLoggerMod::loop() {
  // Must be up for some time, to allow WiFi and other things to stabilise
  if (millis() < minimumUptimeMillis) return;
  timestamp_type now = GET_TIMESTAMP();
  timestamp_type lastReading = store->latest();
  timestamp_type nextReading = lastReading + interval;
  int nextInterval = interval;
  IotsaSerial.printf("xxxjack loop now=%ld lastReading=%ld\n", now, lastReading);
  if (
      now >= nextReading // Normal: interval has passed
      || now < lastReading // Abnormal: clock has gone back in time.
    ) {
    lastReading = now;
    // xxxx save lastReading in NVM
    float value = 0;
    for(int i=0; i<nSample; i++) {
      int iValue = analogRead(PIN_ANALOG_IN);
      value += iValue * adcMultiply + adcOffset;
    }
    value /= nSample;
    IotsaSerial.printf("xxxjack value=%f\n", value);
    store->add(now, value);
  } else {
    // We have not slept long enough.
    nextInterval = (int)(nextReading - now);
    if (nextInterval < 0) nextInterval = 1;
  }
  //
  // Should we go to sleep?
  //
  if (deepSleep) {
    const char *bootReason = iotsaConfig.getBootReason();
    bool hasWifi = iotsaConfig.networkIsUp();
    bool canSleep = iotsaConfig.canSleep();
    int pin0 = digitalRead(0);
    IotsaSerial.printf("xxxjack bootReason=%s hasWifi=%d pin0=%d deepSleep=%d canSleep=%d nextInterval=%d\n", bootReason, hasWifi, pin0, deepSleep, canSleep, nextInterval);
    if (!hasWifi && canSleep && pin0) {
      IotsaSerial.println("Deep sleep.");
      delay(10);
      esp_sleep_enable_timer_wakeup(nextInterval*1000000LL);
      esp_deep_sleep_start();
    }
  }
}
