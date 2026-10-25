#include "iotsaDataLogger.h"
#include "iotsaConfigFile.h"
#include <LittleFS.h>

// The acquisition settings, the same on the page and in the API. Changing
// them needs "config" (lateSetup()); reading them and the data is open.
template<class Ctx> void IotsaDataLoggerMod::_args(Ctx& ctx) {
  ctx.arg("interval", interval, IotsaContext::Saved);
  ctx.arg("adcMultiply", adcMultiply, IotsaContext::Saved);
  ctx.arg("adcOffset", adcOffset, IotsaContext::Saved);
  ctx.arg("deepSleep", deepSleep, IotsaContext::Saved);
  ctx.arg("rawRetentionDays", rawRetentionDays, IotsaContext::Saved);
}

#ifdef IOTSA_WITH_WEB
void IotsaDataLoggerMod::formHandler_args(IotsaWebContext& ctx) {
  _args(ctx);
}

void IotsaDataLoggerMod::formHandler_fields(IotsaWebContext& ctx) {
  if (ctx.includesSettings()) {
    IotsaWebContext::Form form(ctx, "Save", "Acquisition settings");
    ctx.showField("Interval (seconds)", "interval", interval, IotsaWebContext::Saved);
    ctx.showField("ADC multiplication factor", "adcMultiply", adcMultiply, IotsaWebContext::Saved);
    ctx.showField("ADC offset", "adcOffset", adcOffset, IotsaWebContext::Saved);
    ctx.showCheckbox("Deep sleep", "deepSleep", deepSleep, IotsaWebContext::Saved, "between acquisitions, unless WiFi is available");
    ctx.showField("Raw data retention (days)", "rawRetentionDays", rawRetentionDays, IotsaWebContext::Saved);
    return;
  }
  size_t fsTotal = LittleFS.totalBytes();
  size_t fsUsed = LittleFS.usedBytes();
  int fsPct = (int)(100.0f * fsUsed / fsTotal);
  char fsBuf[64];
  snprintf(fsBuf, sizeof(fsBuf), "%d KB / %d KB (%d%%)", (int)(fsUsed/1024), (int)(fsTotal/1024), fsPct);
  ctx.showStatus("Flash filesystem", "", fsBuf);

  String tables = "<h3>Daily measurements</h3>";
  store->toHTMLDaily(tables);
  tables += "<h3>Recent raw measurements</h3>";
  store->toHTML(tables);
  ctx << tables;
}

String IotsaDataLoggerMod::info() {
  String message = "<p>Timed data logger. See <a href=\"/datalogger\">/datalogger</a> for configuration.</p>"
    "<p><a href=\"/datalogger/data_daily.csv\">/datalogger/data_daily.csv</a> &mdash; daily min/max summaries (older data compressed, one row per day).</p>"
    "<p><a href=\"/datalogger/data.csv\">/datalogger/data.csv</a> &mdash; recent measurements at full resolution.</p>";
  return message;
}
#endif // IOTSA_WITH_WEB

void IotsaDataLoggerMod::getHandler(IotsaApiContext& ctx) {
  JsonObject reply = ctx.reply();
  store->toJSON(reply, true);
  ctx.show("interval", interval);
  ctx.show("adcMultiply", adcMultiply);
  ctx.show("adcOffset", adcOffset);
  ctx.show("deepSleep", deepSleep);
  ctx.show("rawRetentionDays", rawRetentionDays);
  ctx.show("fsUsed", (int)LittleFS.usedBytes());
  ctx.show("fsTotal", (int)LittleFS.totalBytes());
}

void
IotsaDataLoggerMod::dataHandler() {
  store->toCSV(app.server);
}

void
IotsaDataLoggerMod::dailyHandler() {
  store->toCSVDaily(app.server);
}

// forgetBefore (a timestamp) throws away the measurements before it.
void IotsaDataLoggerMod::putHandler(IotsaApiContext& ctx) {
  _args(ctx);
  if (!ctx.argAction("forgetBefore", IotsaApiContext::Transient)) return;
  JsonVariant ts = ctx.request()["forgetBefore"];
  if (!ts.is<timestamp_type>()) {
    ctx.error(400, "forgetBefore: a timestamp");
    return;
  }
  store->forget(ts.as<timestamp_type>());
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
  // /datalogger (the module's own page) is registered by api.setup() below.
  api.setup("datalogger", true, true);
  changesNeedRight("config");
#ifdef IOTSA_WITH_WEB
  // The two CSV downloads answer the request themselves; open, like the data
  // on the page.
  addWebHandler("/datalogger/data.csv", [this]() { dataHandler(); }, IOTSA_RIGHT_NONE, true);
  addWebHandler("/datalogger/data_daily.csv", [this]() { dailyHandler(); }, IOTSA_RIGHT_NONE, true);
#endif
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
    bool hasWifi = iotsaStatus.networkIsUp();
    bool canSleep = iotsaController.canSleep();
    int pin0 = digitalRead(0);
    // Only sleep when WiFi is absent: stay awake while connected so the web UI remains accessible.
    if (!hasWifi && canSleep && pin0) {
      IOTSA_LOG("datalogger", "deep sleep for %d seconds", nextInterval);
      delay(10);
      esp_sleep_enable_timer_wakeup(nextInterval*1000000LL);
      esp_deep_sleep_start();
    }
  }
}
