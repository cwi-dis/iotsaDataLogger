#include "dataStoreMemory.h"

void DataStoreMemory::add(timestamp_type ts, const dataStoreItem& value)
{
  if (nItem >= DATALOGGERBUFFERSIZE) compact();
  items[nItem].value = value;
  items[nItem].timestamp = ts;
  nItem++;
}

void DataStoreMemory::compact()
{
  int toRemove = nItem - DATALOGGERBUFFERMINSIZE;
  if (toRemove <= 0) return;
  memmove(items, items+toRemove, DATALOGGERBUFFERMINSIZE*sizeof(DataStoreMemoryRecord));
  nItem -= toRemove;
}

bool DataStoreMemory::should_compact() {
    return nItem > DATALOGGERBUFFERSIZE;
}

void DataStoreMemory::forget(timestamp_type ts) {
    IotsaSerial.println("DataStoreMemory::forget not yet implemented");
}

void DataStoreMemory::toJSON(JsonObject &replyObj)
{
  replyObj["now"] = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  JsonArray values = replyObj.createNestedArray("data");

  for (int i=0; i<nItem; i++) {
    JsonObject curValue = values.createNestedObject();
    curValue["t"] = FORMAT_TIMESTAMP(items[i].timestamp);
    curValue["v"] = items[i].value;
  }
}

void DataStoreMemory::toHTML(String& reply)
{
  reply += "<h2>Recent values</h2><p>Current time: ";
  auto ts = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  reply += ts.c_str();
  reply += "</p>";

  reply += "<table><tr><th>Time</th><th>Value</th></tr>";
  for (int i=0; i<nItem; i++) {
    reply += "<tr><td>";
    auto ts = FORMAT_TIMESTAMP(items[i].timestamp);
    reply += ts.c_str();
    reply += "</td><td>";
    reply += String(items[i].value);
    reply += "</td></tr>";
  }
  reply += "</table>";
}