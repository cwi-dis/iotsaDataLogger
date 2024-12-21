#ifndef _DATASTOREFILE_H_
#define _DATASTOREFILE_H_

#include "dataStore.h"

typedef struct {
  dataStoreItem value;
  timestamp_type timestamp;
} DataStoreFileRecord;


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
  void archive() override;
  bool should_archive() override;
  void forget(timestamp_type ts) override;
  void toJSON(JsonObject& reply, bool archived, bool summary) override;
  void toHTML(String& reply, bool archived, bool summary) override;
  void toCSV(IotsaWebServer *server, bool archived) override;

private:
  void _storeRec(DataStoreFileRecord rec, JsonObject obj);
  void _sendCSV(DataStoreFileRecord rec, IotsaWebServer* server);
  timestamp_type latestTimestamp;
  int nItem;
};

#endif