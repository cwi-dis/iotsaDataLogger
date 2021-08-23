#ifndef _DATASTOREMEMORY_H_
#define _DATASTOREMEMORY_H_

#include "dataStore.h"

#define DATALOGGERBUFFERSIZE 1024
#define DATALOGGERBUFFERMINSIZE 512


typedef struct {
  DataLoggerBufferItemValueType value;
  timestamp_type timestamp;
} DataLoggerBufferItem;


class DataLoggerBuffer : public DataStore
{
public:
  DataLoggerBuffer()
  : nItem(0)
  {}
  ~DataLoggerBuffer() {}
  void add(timestamp_type ts, const DataLoggerBufferItemValueType& value) override;
  void compact() override;
  void toJSON(JsonObject& reply) override;
  void toHTML(String& reply) override;
private:
  int nItem;
  DataLoggerBufferItem items[DATALOGGERBUFFERSIZE];
};

#endif