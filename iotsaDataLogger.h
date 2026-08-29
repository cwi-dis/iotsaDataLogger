#ifndef _IOTSADATALOGGER_H_
#define _IOTSADATALOGGER_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "dataStore.h"

#undef WITH_MEMORY_STORE

#ifdef WITH_MEMORY_STORE
#include "dataStoreMemory.h"
typedef DataStoreMemory DataStoreImplementation;
#else
#include "dataStoreFile.h"
typedef DataStoreFile DataStoreImplementation;
#endif

//
// Input pin
//
#define PIN_ANALOG_IN 34

class IotsaDataLoggerMod : public IotsaModule {
public:
  IotsaDataLoggerMod(IotsaApplication &_app)
  : IotsaModule(_app),
    store(new DataStoreImplementation())
  {}
  void setup() override;
  void lateSetup() override;
  void loop() override;
  String info() override;
  using IotsaBaseModule::needsAuthentication;
protected:
  void configLoad() override;
  void configSave() override;
#ifdef IOTSA_WITH_WEB
  void webHandler() override;
#endif
  void dataHandler();
  void dailyHandler();
  bool getHandler(const char *path, JsonObject& reply) override;
  bool putHandler(const char *path, const JsonVariant& request, JsonObject& reply) override;
  int interval;
  float adcMultiply;
  float adcOffset;
  int rawRetentionDays = 14;
  const int nSample = 5;
  const int minimumUptimeMillis = 15000;
  bool deepSleep = false;
  DataStore *store;
};

#endif
