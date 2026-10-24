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
void IotsaAnnotationsMod::formHandler_args(IotsaWebContext& ctx) {
  IotsaContext::Change change(ctx, "set", IotsaWebContext::Saved);
  if (!change) return;
  String key, value;
  ctx.argPeek("key", key);
  ctx.argPeek("value", value);
  key.trim();
  if (key == "") {
    ctx.error("A key can't be empty.");
    return;
  }
  if (set(key, value)) change.done();
}

void IotsaAnnotationsMod::formHandler_fields(IotsaWebContext& ctx) {
  if (ctx.includesEveryday()) {
    ctx << "<p>Free-form key/value information about this device, for use by tools. The device itself does not use it.</p>";
    if (annotations.empty()) {
      ctx << "<p>No annotations.</p>";
      return;
    }
    String table = "<table><tr><th>Key</th><th>Value</th></tr>";
    for (auto& kv : annotations) {
      table += "<tr><td>" + htmlEncode(kv.first) + "</td><td>" + htmlEncode(kv.second) + "</td></tr>";
    }
    table += "</table>";
    ctx << table;
    return;
  }
  IotsaWebContext::Form form(ctx, "Set", "Set annotation", "set");
  ctx.showField("Key", "key", "", IotsaWebContext::Saved);
  ctx.showField("Value", "value", "", IotsaWebContext::Saved, "empty to remove");
}

String IotsaAnnotationsMod::info() {
  String message = "<p>";
  message += String((int)annotations.size());
  message += " annotations. See <a href=\"/annotations\">/annotations</a> to view or change them.</p>";
  return message;
}
#endif // IOTSA_WITH_WEB

void IotsaAnnotationsMod::getHandler(IotsaApiContext& ctx) {
  for (auto& kv : annotations) {
    ctx.show(kv.first.c_str(), kv.second);
  }
}

void IotsaAnnotationsMod::putHandler(IotsaApiContext& ctx) {
  for (JsonPair kv : ctx.request()) {
    String key(kv.key().c_str());
    if (!ctx.argAction(key.c_str(), IotsaApiContext::Saved)) continue;
    String value;
    if (kv.value().is<const char *>()) {
      value = kv.value().as<const char *>();
    } else {
      // Numbers and booleans are stored in their JSON representation
      serializeJson(kv.value(), value);
    }
    set(key, value);
  }
}

void IotsaAnnotationsMod::setup() {
  configLoad();
}

void IotsaAnnotationsMod::lateSetup() {
  api.setup("annotations", true, true);
  name = "annotations";
  // Annotations describe the device, as its settings do.
  changesNeedRight("config");
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
