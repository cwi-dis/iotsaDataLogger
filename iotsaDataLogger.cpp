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
  if (anyChanged) configSave();

  String message = "<html><head><title>Timed Data Logger Module</title></head><body><h1>Timed Data Logger Module</h1>";
  message += "<form method='get'>Interval (ms): <input name='interval' value='";
  message += String(interval);
  message += "'><br><input type='submit'></form>";
  server->send(200, "text/html", message);
}

String IotsaDataLoggerMod::info() {
  String message = "<p>Timed sensor readings. See <a href=\"/datalogger\">/datalogger</a> for configuration, <a href=\"/api\">/api</a> for readings.</p>";
  return message;
}
#endif // IOTSA_WITH_WEB

bool IotsaDataLoggerMod::getHandler(const char *path, JsonObject& reply) {
  buffer.toJSON(reply);
  reply["interval"] = interval;
  return true;
}

bool IotsaDataLoggerMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  if (!request.is<JsonObject>()) return false;
  JsonObject reqObj = request.as<JsonObject>();
  if (!reqObj.containsKey("interval")) return false;
  interval = reqObj["interval"];
  configSave();
  return true;
}

void IotsaDataLoggerMod::setup() {
  configLoad();
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
  cf.get("interval", interval, 1000);
 
}

void IotsaDataLoggerMod::configSave() {
  IotsaConfigFileSave cf("/config/datalogger.cfg");
  cf.put("interval", interval);
}

void IotsaDataLoggerMod::loop() {
  if (millis() >= lastReading + interval) {
    lastReading = millis();
    int value = analogRead(A0);
    buffer.add(value);
  }
}

void DataLoggerBuffer::add(DataLoggerBufferItemValueType value)
{
  if (nItem >= DATALOGGERBUFFERSIZE) compact();
  items[nItem].value = value;
  items[nItem].timestamp = millis();
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
  if (nItem == 0) return;
  uint32_t curTime = items[0].timestamp;
  replyObj["timestamp"] = curTime;
  JsonArray values = replyObj.createNestedArray("data");

  for (int i=0; i<nItem; i++) {
    uint32_t delta = items[i].timestamp-curTime;
    JsonObject curValue = values.createNestedObject();
    curValue["dt"] = delta;
    curValue["v"] = items[i].value;
    curTime = items[i].timestamp;
  }
}

