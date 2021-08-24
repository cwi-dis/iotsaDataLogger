#include "dataStoreMemory.h"

void DataStoreMemory::add(timestamp_type ts, const dataStoreItem& value)
{
  if (nItem >= DATALOGGERBUFFERSIZE) archive();
  items[nItem].value = value;
  items[nItem].timestamp = ts;
  nItem++;
}

timestamp_type DataStoreMemory::latest() {
  if (nItem <= 0) return 0;
  return items[nItem-1].timestamp;

}

void DataStoreMemory::archive()
{
  int toRemove = nItem - DATALOGGERBUFFERMINSIZE;
  if (toRemove <= 0) return;
  memmove(items, items+toRemove, DATALOGGERBUFFERMINSIZE*sizeof(DataStoreMemoryRecord));
  IotsaSerial.printf("DataStoreMemory: compact %d items\n", toRemove);
  nItem -= toRemove;
}

bool DataStoreMemory::should_archive() {
    return nItem > DATALOGGERBUFFERSIZE;
}

void DataStoreMemory::forget(timestamp_type ts) {
    int earliest = 0;
    for(int i=0; i<nItem; i++) {
        if (items[i].timestamp <= ts) earliest = i;
    }
    if (earliest > 0) {
        memmove(items, items+earliest, (nItem-earliest)*sizeof(DataStoreMemoryRecord));
        IotsaSerial.printf("DataStoreMemory: forget %d items\n", earliest);
        nItem -= earliest;
    }
}

void DataStoreMemory::toJSON(JsonObject &replyObj, bool archived)
{
  replyObj["now"] = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  JsonArray values = replyObj.createNestedArray("data");
  if (archived) return;

  for (int i=0; i<nItem; i++) {
    JsonObject curValue = values.createNestedObject();
    curValue["t"] = FORMAT_TIMESTAMP(items[i].timestamp);
    curValue["ts"] = items[i].timestamp;
    curValue["v"] = items[i].value;
  }
}

void DataStoreMemory::toHTML(String& reply, bool archived)
{
  reply += "<p>Current time: ";
  auto ts = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  reply += ts.c_str();
  if (archived) {
    reply += ", no archived data.</p>";
    return;
  }
  reply += ", " + String(size()) + " entries.";
  reply += "</p>";

  reply += "<table><tr><th>Time</th><th>Timestamp</th><th>Value</th></tr>";
  for (int i=0; i<nItem; i++) {
    reply += "<tr><td>";
    auto ts = FORMAT_TIMESTAMP(items[i].timestamp);
    reply += ts.c_str();
    reply += "</td><td>";
    reply += String(items[i].timestamp);
    reply += "</td><td>";
    reply += String(items[i].value);
    reply += "</td></tr>";
  }
  reply += "</table>";
}