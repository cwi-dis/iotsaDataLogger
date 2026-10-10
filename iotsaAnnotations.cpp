#include "iotsaAnnotations.h"
#include "iotsaConfigFile.h"

static const char *configFilename = "/config/annotations.cfg";

String IotsaAnnotationsMod::get(const String& key) {
  auto it = annotations.find(key);
  if (it == annotations.end()) return "";
  return it->second;
}

// Set (or, with an empty value, remove) an annotation. Returns true if anything changed.
bool IotsaAnnotationsMod::set(const String& key, const String& value) {
  if (key == "") return false;
  auto it = annotations.find(key);
  if (value == "") {
    if (it == annotations.end()) return false;
    annotations.erase(it);
    return true;
  }
  if (it != annotations.end() && it->second == value) return false;
  annotations[key] = value;
  return true;
}

#ifdef IOTSA_WITH_WEB
void
IotsaAnnotationsMod::webHandler() {
  if (api.webService->server->hasArg("key")) {
    if (needsAuthentication("annotations")) return;
    String key = api.webService->server->arg("key");
    key.trim();
    String value = api.webService->server->arg("value");
    if (set(key, value)) configSave();
  }

  String message = "<html><head><title>Annotations</title></head><body><h1>Annotations</h1>";
  message += "<p>Free-form key/value information about this device, for use by tools. The device itself does not use it.</p>";
  if (annotations.empty()) {
    message += "<p>No annotations.</p>";
  } else {
    message += "<table><tr><th>Key</th><th>Value</th></tr>";
    for (auto& kv : annotations) {
      message += "<tr><td>" + htmlEncode(kv.first) + "</td><td>" + htmlEncode(kv.second) + "</td></tr>";
    }
    message += "</table>";
  }
  message += "<h2>Set annotation</h2>";
  message += "<form method='get'>Key: <input name='key'><br>Value: <input name='value'> (empty to remove)<br>";
  message += "<input type='submit'></form>";
  message += "</body></html>";
  api.webService->server->send(200, "text/html", message);
}

String IotsaAnnotationsMod::info() {
  String message = "<p>";
  message += String((int)annotations.size());
  message += " annotations. See <a href=\"/annotations\">/annotations</a> to view or change them.</p>";
  return message;
}
#endif // IOTSA_WITH_WEB

bool IotsaAnnotationsMod::getHandler(const char *path, JsonObject& reply) {
  for (auto& kv : annotations) {
    reply[kv.first] = kv.second;
  }
  return true;
}

bool IotsaAnnotationsMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  if (!request.is<JsonObject>()) return false;
  bool anyChanged = false;
  for (JsonPair kv : request.as<JsonObject>()) {
    String value;
    if (kv.value().isNull()) {
      value = "";
    } else if (kv.value().is<const char *>()) {
      value = kv.value().as<const char *>();
    } else {
      // Numbers and booleans are stored in their JSON representation
      serializeJson(kv.value(), value);
    }
    if (set(String(kv.key().c_str()), value)) anyChanged = true;
  }
  if (anyChanged) configSave();
  return true;
}

void IotsaAnnotationsMod::setup() {
  configLoad();
}

void IotsaAnnotationsMod::lateSetup() {
  api.setup("annotations", true, true);
  name = "annotations";
}

// The config file has no way to enumerate keys, so all annotations are
// stored as a single JSON object.
void IotsaAnnotationsMod::configLoad() {
  IotsaConfigFileLoad cf(configFilename);
  String json;
  cf.get("annotations", json, "{}");
  JsonDocument doc;
  annotations.clear();
  if (deserializeJson(doc, json) != DeserializationError::Ok) {
    IotsaSerial.println("annotations: cannot parse config");
    return;
  }
  for (JsonPair kv : doc.as<JsonObject>()) {
    annotations[String(kv.key().c_str())] = kv.value().as<const char *>();
  }
}

void IotsaAnnotationsMod::configSave() {
  JsonDocument doc;
  for (auto& kv : annotations) {
    doc[kv.first] = kv.second;
  }
  String json;
  serializeJson(doc, json);
  IotsaConfigFileSave cf(configFilename);
  cf.put("annotations", json);
}
