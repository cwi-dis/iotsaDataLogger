#include "dataStoreFile.h"

void DataStoreFile::add(timestamp_type ts, const dataStoreItem& value)
{
  if (nItem >= DATALOGGERBUFFERSIZE) compact();
  items[nItem].value = value;
  items[nItem].timestamp = ts;
  nItem++;
}

timestamp_type DataStoreFile::latest() {
  if (nItem <= 0) return 0;
  return items[nItem-1].timestamp;
}

void DataStoreFile::compact()
{
  int toRemove = nItem - DATALOGGERBUFFERMINSIZE;
  if (toRemove <= 0) return;
  memmove(items, items+toRemove, DATALOGGERBUFFERMINSIZE*sizeof(DataStoreFileRecord));
  IotsaSerial.printf("DataStoreFile: compact %d items\n", toRemove);
  nItem -= toRemove;
}

bool DataStoreFile::should_compact() {
    return nItem > DATALOGGERBUFFERSIZE;
}

void DataStoreFile::forget(timestamp_type ts) {
    int earliest = 0;
    for(int i=0; i<nItem; i++) {
        if (items[i].timestamp <= ts) earliest = i;
    }
    if (earliest > 0) {
        memmove(items, items+earliest, (nItem-earliest)*sizeof(DataStoreFileRecord));
        IotsaSerial.printf("DataStoreFile: forget %d items\n", earliest);
        nItem -= earliest;
    }
}

void DataStoreFile::toJSON(JsonObject &replyObj)
{
  replyObj["now"] = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  JsonArray values = replyObj.createNestedArray("data");

  for (int i=0; i<nItem; i++) {
    JsonObject curValue = values.createNestedObject();
    curValue["t"] = FORMAT_TIMESTAMP(items[i].timestamp);
    curValue["ts"] = items[i].timestamp;
    curValue["v"] = items[i].value;
  }
}

void DataStoreFile::toHTML(String& reply)
{
  reply += "<p>Current time: ";
  auto ts = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  reply += ts.c_str();
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