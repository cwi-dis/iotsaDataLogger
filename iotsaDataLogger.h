#ifndef _IOTSADATALOGGER_H_
#define _IOTSADATALOGGER_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "dataStore.h"
#include "dataStoreFile.h"

typedef DataStoreFile DataStoreImplementation;

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
protected:
  void configLoad() override;
  void configSave() override;
#ifdef IOTSA_WITH_WEB
  String webTitle() override { return "Timed Data Logger"; }
  void formHandler_args(IotsaWebContext& ctx) override;
  void formHandler_fields(IotsaWebContext& ctx) override;
#endif
  void dataHandler();
  void dailyHandler();
  void getHandler(IotsaApiContext& ctx) override;
  void putHandler(IotsaApiContext& ctx) override;
  template<class Ctx> void _args(Ctx& ctx);
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
