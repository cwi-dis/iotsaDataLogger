#ifndef _IOTSADATALOGGER_H_
#define _IOTSADATALOGGER_H_
#include "iotsa.h"
#include "iotsaApi.h"

typedef time_t timestamp_type;
#define GET_TIMESTAMP() (time(nullptr))
inline std::string FORMAT_TIMESTAMP(timestamp_type ts) {
  struct tm *tm = localtime(&ts);
  char buf[25];
  size_t sz = strftime(buf,  sizeof buf, "%Y-%m-%dT%H:%M:%S", tm);
  return std::string(buf, sz);
}

typedef uint16_t DataLoggerBufferItemValueType;

typedef struct {
  DataLoggerBufferItemValueType value;
  timestamp_type timestamp;
} DataLoggerBufferItem;

#define DATALOGGERBUFFERSIZE 1024
#define DATALOGGERBUFFERMINSIZE 512
class DataLoggerBuffer
{
public:
  DataLoggerBuffer()
  : nItem(0)
  {}
  void add(timestamp_type ts, DataLoggerBufferItemValueType value);
  void compact();
  void toJSON(JsonObject& reply);
  void toHTML(String& reply);
  int nItem;
  DataLoggerBufferItem items[DATALOGGERBUFFERSIZE];
};

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
  DataLoggerBuffer buffer;
};

#endif
