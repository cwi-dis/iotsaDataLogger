#ifndef _DATASTOREFILE_H_
#define _DATASTOREFILE_H_

#include "dataStore.h"

typedef struct {
  dataStoreItem value;
  timestamp_type timestamp;
} DataStoreFileRecord;

typedef struct {
  timestamp_type min_t;
  timestamp_type max_t;
  dataStoreItem min_v;
  dataStoreItem max_v;
  int32_t n;
} DataStoreDailyFileRecord;


class DataStoreFile : public DataStore
{
public:
  DataStoreFile()
  : latestTimestamp(0),
    nItem(0)
  {}
  ~DataStoreFile() {}
  void add(timestamp_type ts, const dataStoreItem& value) override;
  timestamp_type latest() override;
  int size() override;
  void forget(timestamp_type ts) override;
  void toJSON(JsonObject& reply, bool summary) override;
  void toHTML(String& reply, bool summary) override;
  void toCSV(IotsaWebServer *server) override;
  void compress(timestamp_type now) override;
  void toCSVDaily(IotsaWebServer *server) override;

private:
  void _storeRec(DataStoreFileRecord rec, JsonObject obj);
  void _sendCSV(DataStoreFileRecord rec, IotsaWebServer* server);
  void _sendDailyCSV(DataStoreDailyFileRecord rec, IotsaWebServer* server);
  timestamp_type latestTimestamp;
  int nItem;
};

#endif
