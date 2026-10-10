#ifndef _IOTSAANNOTATIONS_H_
#define _IOTSAANNOTATIONS_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include <map>

//
// Free-form key/value annotations, a bit like DNS TXT records. The device
// stores and serves them but does not interpret them: they are for tools
// talking to the device (descriptions, location, how to present data, etc).
//
// REST: GET /api/annotations returns an object with all annotations (string
// values). PUT merges the request object into it; an empty or null value
// removes the key.
//
// Generic: candidate for moving into iotsa itself, see cwi-dis/iotsa#290.
//
class IotsaAnnotationsMod : public IotsaModule {
public:
  using IotsaModule::IotsaModule;
  void setup() override;
  void lateSetup() override;
  void loop() override {}
#ifdef IOTSA_WITH_WEB
  String info() override;
#endif
  // Returns the annotation, or an empty string if it is not set.
  String get(const String& key);
protected:
  bool getHandler(const char *path, JsonObject& reply) override;
  bool putHandler(const char *path, const JsonVariant& request, JsonObject& reply) override;
  void configLoad() override;
  void configSave() override;
#ifdef IOTSA_WITH_WEB
  void webHandler() override;
#endif
  bool set(const String& key, const String& value);
  std::map<String, String> annotations;
};

#endif
