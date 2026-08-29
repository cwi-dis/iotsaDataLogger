#include "iotsaDataLogger.h"
#include "iotsaConfigFile.h"
#include <LittleFS.h>

#ifdef IOTSA_WITH_WEB
void
IotsaDataLoggerMod::webHandler() {
  bool anyChanged = false;
  if( api.webService->server->hasArg("interval")) {
    if (needsAuthentication()) return;
    String sInterval = api.webService->server->arg("interval");
    interval = sInterval.toInt();
    anyChanged = true;
  }
  if( api.webService->server->hasArg("adcMultiply")) {
    if (needsAuthentication()) return;
    String sv = api.webService->server->arg("adcMultiply");
    adcMultiply = sv.toFloat();
    anyChanged = true;
  }
  if( api.webService->server->hasArg("adcOffset")) {
    if (needsAuthentication()) return;
    String sv = api.webService->server->arg("adcOffset");
    adcOffset = sv.toFloat();
    anyChanged = true;
  }
  if( api.webService->server->hasArg("deepSleep")) {
    if (needsAuthentication()) return;
    String sv = api.webService->server->arg("deepSleep");
    deepSleep = (bool)sv.toInt();
    anyChanged = true;
  }
  if( api.webService->server->hasArg("rawRetentionDays")) {
    if (needsAuthentication()) return;
    String sv = api.webService->server->arg("rawRetentionDays");
    rawRetentionDays = sv.toInt();
    anyChanged = true;
  }
  if (anyChanged) configSave();

  String message = "<html><head><title>Timed Data Logger Module</title></head><body><h1>Timed Data Logger Module</h1>";

  message += "<h2>Acquisition settings</h2>";
  message += "<form method='get'>Interval (seconds): <input name='interval' value='";
  message += String(interval);
  message += "'><br>ADC multiplication factor: <input name='adcMultiply' value='";
  message += String(adcMultiply, 5);
  message += "'><br>ADC offset: <input name='adcOffset' value='";
  message += String(adcOffset, 5);
  message +="'><br><input type='checkbox' name='deepSleep' value='1'";
  if (deepSleep) message += " checked";
  message += ">Deep Sleep between acquisitions (unless WiFi is available)";
  message += "<br>Raw data retention (days): <input name='rawRetentionDays' value='";
  message += String(rawRetentionDays);
  message += "'>";
  message += "<br><input type='submit'></form>";

  size_t fsTotal = LittleFS.totalBytes();
  size_t fsUsed = LittleFS.usedBytes();
  int fsPct = (int)(100.0f * fsUsed / fsTotal);
  char fsBuf[64];
  snprintf(fsBuf, sizeof(fsBuf), "%d KB / %d KB (%d%%)", (int)(fsUsed/1024), (int)(fsTotal/1024), fsPct);
  message += "<h2>Storage</h2><p>Flash filesystem: ";
  message += fsBuf;
  message += "</p>";

  message += "<h2>Daily measurements</h2>";
  store->toHTMLDaily(message);
  message += "<h2>Recent raw measurements</h2>";
  store->toHTML(message);
  message += "</body></html>";
  api.webService->server->send(200, "text/html", message);
}

String IotsaDataLoggerMod::info() {
  String message = "<p>Timed data logger. See <a href=\"/datalogger\">/datalogger</a> for configuration.</p>"
    "<p><a href=\"/datalogger/data_daily.csv\">/datalogger/data_daily.csv</a> &mdash; daily min/max summaries (older data compressed, one row per day).</p>"
    "<p><a href=\"/datalogger/data.csv\">/datalogger/data.csv</a> &mdash; recent measurements at full resolution.</p>";
  return message;
}
#endif // IOTSA_WITH_WEB

bool IotsaDataLoggerMod::getHandler(const char *path, JsonObject& reply) {
  store->toJSON(reply, true);
  reply["interval"] = interval;
  reply["adcMultiply"] = adcMultiply;
  reply["adcOffset"] = adcOffset;
  reply["deepSleep"] = deepSleep;
  reply["rawRetentionDays"] = rawRetentionDays;
  reply["fsUsed"] = (int)LittleFS.usedBytes();
  reply["fsTotal"] = (int)LittleFS.totalBytes();
  return true;
}

void
IotsaDataLoggerMod::dataHandler() {
  store->toCSV(app.server);
}

void
IotsaDataLoggerMod::dailyHandler() {
  store->toCSVDaily(app.server);
}

bool IotsaDataLoggerMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  if (!request.is<JsonObject>()) return false;
  JsonObject reqObj = request.as<JsonObject>();
  bool anyChanged = false;
  bool anySet = false;
  if (getFromRequest<int>(reqObj, "interval", interval)) {
    anyChanged = true;
  }
  if (getFromRequest<float>(reqObj, "adcMultiply", adcMultiply)) {
    anyChanged = true;
  }
  if (getFromRequest<float>(reqObj, "adcOffset", adcOffset)) {
    anyChanged = true;
  }
  if (getFromRequest<bool>(reqObj, "deepSleep", deepSleep)) {
    anyChanged = true;
  }
  if (getFromRequest<int>(reqObj, "rawRetentionDays", rawRetentionDays)) {
    anyChanged = true;
  }
  timestamp_type ts;
  if (getFromRequest<timestamp_type>(reqObj, "forgetBefore", ts)) {
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

void IotsaDataLoggerMod::lateSetup() {
  name = "datalogger";
#ifdef IOTSA_WITH_WEB
  // /datalogger (the module's own page) is registered by api.setup() below.
  // The two CSV endpoints are extra routes, registered directly on the server.
  app.server->on("/datalogger/data.csv", std::bind(&IotsaDataLoggerMod::dataHandler, this));
  app.server->on("/datalogger/data_daily.csv", std::bind(&IotsaDataLoggerMod::dailyHandler, this));
#endif
  api.setup("datalogger", true, true);
}

void IotsaDataLoggerMod::configLoad() {
  IotsaConfigFileLoad cf("/config/datalogger.cfg");
  cf.get("interval", interval, 10);
  cf.get("adcMultiply", adcMultiply, 1);
  cf.get("adcOffset", adcOffset, 0);
  cf.get("deepSleep", deepSleep, false);
  cf.get("rawRetentionDays", rawRetentionDays, 14);
}

void IotsaDataLoggerMod::configSave() {
  IotsaConfigFileSave cf("/config/datalogger.cfg");
  cf.put("interval", interval);
  cf.put("adcMultiply", adcMultiply);
  cf.put("adcOffset", adcOffset);
  cf.put("deepSleep", deepSleep);
  cf.put("rawRetentionDays", rawRetentionDays);
}

void IotsaDataLoggerMod::loop() {
  // Must be up for some time, to allow WiFi and other things to stabilise
  if (millis() < minimumUptimeMillis) return;
  timestamp_type now = GET_TIMESTAMP();
  timestamp_type lastReading = store->latest();
  timestamp_type nextReading = lastReading + interval;
  int nextInterval = interval;
  if (
      now >= nextReading // Normal: interval has passed
      || now < lastReading // Abnormal: clock has gone back in time
    ) {
    lastReading = now;
    float value = 0;
    for(int i=0; i<nSample; i++) {
      int iValue = analogRead(PIN_ANALOG_IN);
      value += iValue * adcMultiply + adcOffset;
    }
    value /= nSample;
    store->add(now, value);
    store->compress(now, rawRetentionDays);
  } else {
    // We have not slept long enough.
    nextInterval = (int)(nextReading - now);
    if (nextInterval < 0) nextInterval = 1;
  }
  //
  // Should we go to sleep?
  //
  if (deepSleep) {
    bool hasWifi = iotsaConfig.networkIsUp();
    bool canSleep = iotsaConfig.canSleep();
    int pin0 = digitalRead(0);
    // Only sleep when WiFi is absent: stay awake while connected so the web UI remains accessible.
    if (!hasWifi && canSleep && pin0) {
      IotsaSerial.println("Deep sleep.");
      delay(10);
      esp_sleep_enable_timer_wakeup(nextInterval*1000000LL);
      esp_deep_sleep_start();
    }
  }
}
