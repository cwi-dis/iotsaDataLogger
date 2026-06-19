#ifndef _DATASTORE_H_
#define _DATASTORE_H_

#include "iotsa.h"
#include "iotsaApi.h"

//
// Definitions for using NTP/RTC/Unix time.
//
typedef time_t timestamp_type;
#define GET_TIMESTAMP() (time(nullptr))

inline std::string FORMAT_TIMESTAMP(timestamp_type ts) {
  struct tm *tm = localtime(&ts);
  char buf[25];
  size_t sz = strftime(buf,  sizeof buf, "%Y-%m-%dT%H:%M:%S", tm);
  return std::string(buf, sz);
}

inline std::string FORMAT_DATE(timestamp_type ts) {
  struct tm *tm = gmtime(&ts);
  char buf[12];
  size_t sz = strftime(buf, sizeof buf, "%Y-%m-%d", tm);
  return std::string(buf, sz);
}

typedef float dataStoreItem;

class DataStore
{
public:
  virtual ~DataStore() {}
  virtual void add(timestamp_type ts, const dataStoreItem& value) = 0;
  virtual timestamp_type latest() = 0;
  virtual int size() = 0;
  virtual void forget(timestamp_type ts) = 0;
  virtual void toJSON(JsonObject& reply, bool summary) = 0;
  virtual void toHTML(String& reply) = 0;
  virtual void toHTMLDaily(String& reply) {}
  virtual void toCSV(IotsaWebServer *server) = 0;
  virtual void compress(timestamp_type now, int rawRetentionDays) {}
  virtual void toCSVDaily(IotsaWebServer *server) {}
};

#endif
