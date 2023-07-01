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

void DataStoreMemory::toJSON(JsonObject &replyObj, bool archived, bool summary)
{
  if (!archived) {
    replyObj["now"] = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  }
  JsonArray values = replyObj.createNestedArray("data");
  if (archived) return;

  if (summary) {
    replyObj["count"] = nItem;
    if (nItem == 0) return;
    JsonObject curValue = values.createNestedObject();
    curValue["t"] = FORMAT_TIMESTAMP(items[0].timestamp);
    curValue["ts"] = items[0].timestamp;
    curValue["v"] = items[0].value;
    curValue = values.createNestedObject();
    curValue["t"] = FORMAT_TIMESTAMP(items[nItem-1].timestamp);
    curValue["ts"] = items[nItem-1].timestamp;
    curValue["v"] = items[nItem-1].value;
    return;
  }
  for (int i=0; i<nItem; i++) {
    JsonObject curValue = values.createNestedObject();
    curValue["t"] = FORMAT_TIMESTAMP(items[i].timestamp);
    curValue["ts"] = items[i].timestamp;
    curValue["v"] = items[i].value;
  }
}

void DataStoreMemory::toCSV(IotsaWebServer *server, bool archived) {
  server->send(200, "text/csv", "");
  server->sendContent("t,ts,v\r\n");
  if (archived) return;
  for(int i=0; i<nItem; i++) {
    char buf[100];
    std::string tstring = FORMAT_TIMESTAMP(items[i].timestamp);
    snprintf(buf, sizeof(buf), "\"%s\",%ld,%f\r\n",
      tstring.c_str(),
      (long)items[i].timestamp,
      (float)items[i].value
    );
    String sBuf(buf);
    server->sendContent(sBuf);
  }
  server->sendContent("\"2023-06-26T08:30:18\",0,12.42\r\n");
}

void DataStoreMemory::toHTML(String& reply, bool archived, bool summary)
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
  int step = 1;
  if (summary) {
    step = nItem-1;
  }
  for (int i=0; i<nItem; i+=step) {
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