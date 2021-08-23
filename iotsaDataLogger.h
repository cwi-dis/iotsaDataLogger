#ifndef _IOTSADATALOGGER_H_
#define _IOTSADATALOGGER_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "dataStore.h"
#include "dataStoreMemory.h"

//
// Input pin
//
#define PIN_ANALOG_IN 34

class IotsaDataLoggerMod : public IotsaApiMod {
public:
  IotsaDataLoggerMod(IotsaApplication &_app) : IotsaApiMod(_app) {}
  void setup() override;
  void serverSetup() override;
  void loop() override;
  String info() override;
  using IotsaBaseMod::needsAuthentication;
protected:
  void configLoad() override;
  void configSave() override;
  void handler();
  bool getHandler(const char *path, JsonObject& reply) override;
  bool putHandler(const char *path, const JsonVariant& request, JsonObject& reply) override;
  int interval;
  uint32_t lastReading;
  float adcMultiply;
  float adcOffset;
  DataLoggerBuffer buffer;
};

#endif
