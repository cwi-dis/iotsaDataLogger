#ifndef _DATASTOREFILE_H_
#define _DATASTOREFILE_H_

#include "dataStore.h"

#define DATALOGGERBUFFERSIZE 1024
#define DATALOGGERBUFFERMINSIZE 512


typedef struct {
  dataStoreItem value;
  timestamp_type timestamp;
} DataStoreFileRecord;


class DataStoreFile : public DataStore
{
public:
  DataStoreFile()
  : nItem(0)
  {}
  ~DataStoreFile() {}
  void add(timestamp_type ts, const dataStoreItem& value) override;
  timestamp_type latest() override;
  void compact() override;
  bool should_compact() override;
  void forget(timestamp_type ts) override;
  void toJSON(JsonObject& reply) override;
  void toHTML(String& reply) override;
private:
  int nItem;
  DataStoreFileRecord items[DATALOGGERBUFFERSIZE];
};

#endif