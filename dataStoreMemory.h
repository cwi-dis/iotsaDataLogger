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
  void compact() override;
  bool should_compact() override;
  void forget(timestamp_type ts) override;
  void toJSON(JsonObject& reply) override;
  void toHTML(String& reply) override;
private:
  int nItem;
  DataStoreMemoryRecord items[DATALOGGERBUFFERSIZE];
};

#endif