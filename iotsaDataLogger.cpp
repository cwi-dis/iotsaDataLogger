#include "iotsaDataLogger.h"
#include "iotsaConfigFile.h"

#ifdef IOTSA_WITH_WEB
void
IotsaDataLoggerMod::handler() {
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
  if( server->hasArg("rawRetentionDays")) {
    if (needsAuthentication()) return;
    String sv = server->arg("rawRetentionDays");
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

  message += "<h2>Daily measurements</h2>";
  store->toHTMLDaily(message);
  message += "<h2>Recent raw measurements</h2>";
  store->toHTML(message);
  message += "</body></html>";
  server->send(200, "text/html", message);
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
  return true;
}

void
IotsaDataLoggerMod::dataHandler() {
  store->toCSV(server);
}

void
IotsaDataLoggerMod::dailyHandler() {
  store->toCSVDaily(server);
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
  if (reqObj.containsKey("rawRetentionDays")) {
    rawRetentionDays = reqObj["rawRetentionDays"];
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
  server->on("/datalogger/data.csv", std::bind(&IotsaDataLoggerMod::dataHandler, this));
  server->on("/datalogger/data_daily.csv", std::bind(&IotsaDataLoggerMod::dailyHandler, this));
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
