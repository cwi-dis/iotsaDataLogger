#ifndef _DATASTORE_H_
#define _DATASTORE_H_

#include "iotsa.h"
#include "iotsaApi.h"

//
// Definitions for using NTP/RTC/Unix time.
// May still be changed back to using millis() timestamps from boot,
// but needs work.
//
typedef time_t timestamp_type;
#define GET_TIMESTAMP() (time(nullptr))

inline std::string FORMAT_TIMESTAMP(timestamp_type ts) {
  struct tm *tm = localtime(&ts);
  char buf[25];
  size_t sz = strftime(buf,  sizeof buf, "%Y-%m-%dT%H:%M:%S", tm);
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
  virtual void archive() = 0;
  virtual bool should_archive() = 0;
  virtual void forget(timestamp_type ts) = 0;
  virtual void toJSON(JsonObject& reply, bool archived=false) = 0;
  virtual void toHTML(String& reply, bool archived=false) = 0;
};

#endif