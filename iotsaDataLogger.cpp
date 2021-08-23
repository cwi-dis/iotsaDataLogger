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
  if (anyChanged) configSave();

  String message = "<html><head><title>Timed Data Logger Module</title></head><body><h1>Timed Data Logger Module</h1>";
  message += "<form method='get'>Interval (seconds): <input name='interval' value='";
  message += String(interval);
  message += "'><br>ADC multiplication factor: <input name='adcMultiply' value='";
  message += String(adcMultiply, 3);
  message += "'><br>ADC offset: <input name='adcOffset' value='";
  message += String(adcOffset, 3);
  message += "'><br><input type='submit'></form>";
  buffer.toHTML(message);
  message += "</body></html>";
  server->send(200, "text/html", message);
}

String IotsaDataLoggerMod::info() {
  String message = "<p>Timed data logger. See <a href=\"/datalogger\">/datalogger</a> for configuration, <a href=\"/api/datalogger\">/api/datalogger</a> for readings.</p>";
  return message;
}
#endif // IOTSA_WITH_WEB

bool IotsaDataLoggerMod::getHandler(const char *path, JsonObject& reply) {
  buffer.toJSON(reply);
  reply["interval"] = interval;
  reply["adcMultiply"] = adcMultiply;
  reply["adcOffset"] = adcOffset;
  return true;
}

bool IotsaDataLoggerMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  if (!request.is<JsonObject>()) return false;
  JsonObject reqObj = request.as<JsonObject>();
  bool anyChanged = false;
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
  if (anyChanged) {
    configSave();
  }
  return anyChanged;
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
 
}

void IotsaDataLoggerMod::configSave() {
  IotsaConfigFileSave cf("/config/datalogger.cfg");
  cf.put("interval", interval);
  cf.put("adcMultiply", adcMultiply);
  cf.put("adcOffset", adcOffset);
}

void IotsaDataLoggerMod::loop() {
  timestamp_type now = GET_TIMESTAMP();
  if (
      now >= lastReading + interval // Normal: interval has passed
      || now < lastReading - interval // Abnormal: clock has gone back in time.
    ) {
    lastReading = now;
    // xxxx save lastReading in NVM
    int iValue = analogRead(PIN_ANALOG_IN);
    float value = iValue * adcMultiply + adcOffset;
    IotsaSerial.printf("xxxjack value=%f\n", value);
    buffer.add(now, value);
  }
}

void DataLoggerBuffer::add(timestamp_type ts, const DataLoggerBufferItemValueType& value)
{
  if (nItem >= DATALOGGERBUFFERSIZE) compact();
  items[nItem].value = value;
  items[nItem].timestamp = ts;
  nItem++;
}

void DataLoggerBuffer::compact()
{
  int toRemove = nItem - DATALOGGERBUFFERMINSIZE;
  if (toRemove <= 0) return;
  memmove(items, items+toRemove, DATALOGGERBUFFERMINSIZE*sizeof(DataLoggerBufferItem));
  nItem -= toRemove;
}

void DataLoggerBuffer::toJSON(JsonObject &replyObj)
{
  replyObj["now"] = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  JsonArray values = replyObj.createNestedArray("data");

  for (int i=0; i<nItem; i++) {
    JsonObject curValue = values.createNestedObject();
    curValue["t"] = FORMAT_TIMESTAMP(items[i].timestamp);
    curValue["v"] = items[i].value;
  }
}

void DataLoggerBuffer::toHTML(String& reply)
{
  reply += "<h2>Recent values</h2><p>Current time: ";
  auto ts = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  reply += ts.c_str();
  reply += "</p>";

  reply += "<table><tr><th>Time</th><th>Value</th></tr>";
  for (int i=0; i<nItem; i++) {
    reply += "<tr><td>";
    auto ts = FORMAT_TIMESTAMP(items[i].timestamp);
    reply += ts.c_str();
    reply += "</td><td>";
    reply += String(items[i].value);
    reply += "</td></tr>";
  }
  reply += "</table>";
}