#ifndef _DATASTOREMEMORY_H_
#define _DATASTOREMEMORY_H_

#include "dataStore.h"

#define DATALOGGERBUFFERSIZE 1024
#define DATALOGGERBUFFERMINSIZE 512


typedef struct {
  dataStoreItem value;
  timestamp_type timestamp;
} DataStoreMemoryRecord;


class DataStoreMemory : public DataStore
{
public:
  DataStoreMemory()
  : nItem(0)
  {}
  ~DataStoreMemory() {}
  void add(timestamp_type ts, const dataStoreItem& value) override;
  timestamp_type latest() override;
  int size() override { return nItem; }
  void forget(timestamp_type ts) override;
  void toJSON(JsonObject& reply, bool summary) override;
  void toHTML(String& reply, bool summary) override;
  void toCSV(IotsaWebServer *server) override;
private:
  void _trim();
  int nItem;
  DataStoreMemoryRecord items[DATALOGGERBUFFERSIZE];
};

#endif
