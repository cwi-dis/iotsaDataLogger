#include "dataStoreMemory.h"

void DataStoreMemory::add(timestamp_type ts, const dataStoreItem& value)
{
  if (nItem >= DATALOGGERBUFFERSIZE) _trim();
  items[nItem].value = value;
  items[nItem].timestamp = ts;
  nItem++;
}

timestamp_type DataStoreMemory::latest() {
  if (nItem <= 0) return 0;
  return items[nItem-1].timestamp;
}

void DataStoreMemory::_trim()
{
  int toRemove = nItem - DATALOGGERBUFFERMINSIZE;
  if (toRemove <= 0) return;
  memmove(items, items+toRemove, DATALOGGERBUFFERMINSIZE*sizeof(DataStoreMemoryRecord));
  IotsaSerial.printf("DataStoreMemory: trim %d items\n", toRemove);
  nItem -= toRemove;
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

void DataStoreMemory::toJSON(JsonObject &replyObj, bool summary)
{
  replyObj["now"] = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  JsonArray values = replyObj.createNestedArray("data");

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

void DataStoreMemory::toCSV(IotsaWebServer *server) {
  server->send(200, "text/csv", "");
  server->sendContent("t,ts,v\r\n");
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
}

void DataStoreMemory::toHTML(String& reply)
{
  reply += "<p>Current time: ";
  reply += FORMAT_TIMESTAMP(GET_TIMESTAMP()).c_str();
  reply += ", " + String(size()) + " entries.</p>";

  if (nItem == 0) return;
  reply += "<table><tr><th>Time</th><th>Value</th></tr>";
  reply += "<tr><td>";
  reply += FORMAT_TIMESTAMP(items[0].timestamp).c_str();
  reply += "</td><td>";
  reply += String(items[0].value);
  reply += "</td></tr>";
  if (nItem > 1) {
    reply += "<tr><td>";
    reply += FORMAT_TIMESTAMP(items[nItem-1].timestamp).c_str();
    reply += "</td><td>";
    reply += String(items[nItem-1].value);
    reply += "</td></tr>";
  }
  reply += "</table>";
}
