#include "dataStoreFile.h"

const char* datastoreFilename = "/spiffs/datastore.dat";

void DataStoreFile::add(timestamp_type ts, const dataStoreItem& value)
{
  FILE *fp = fopen(datastoreFilename, "ab");
  if (fp == NULL) {
    IotsaSerial.println("DataStoreFile: add: fopen failed");
    return;
  }
  DataStoreFileRecord rec;
  rec.value = value;
  rec.timestamp = ts;
  size_t sz = fwrite(&rec, sizeof(rec), 1, fp);
  if (sz != 1) {
    IotsaSerial.println("DataStoreFile: add: fwrite failed");
  }
  fclose(fp);
  latestTimestamp = ts;
}

timestamp_type DataStoreFile::latest() {
  if (latestTimestamp != 0) return latestTimestamp;
  FILE *fp = fopen(datastoreFilename, "rb");
  if (fp == NULL) {
    IotsaSerial.println("DataStoreFile: latest: fopen failed");
    return 0;
  }
  DataStoreFileRecord rec;
  rec.timestamp = 0;
  int sts = fseek(fp, -sizeof(rec), SEEK_END);
  if (sts != 0) {
    IotsaSerial.println("DataStoreFile: latest: fseek failed");
  }
  size_t sz = fread(&rec, sizeof(rec), 1, fp);
  if (sz != 1) {
    IotsaSerial.println("DataStoreFile: latest: fread failed");
  }
  fclose(fp);
  return rec.timestamp;
}

void DataStoreFile::compact()
{
#if 0
  int toRemove = nItem - DATALOGGERBUFFERMINSIZE;
  if (toRemove <= 0) return;
  memmove(items, items+toRemove, DATALOGGERBUFFERMINSIZE*sizeof(DataStoreFileRecord));
  IotsaSerial.printf("DataStoreFile: compact %d items\n", toRemove);
  nItem -= toRemove;
#else
  IotsaSerial.println("DataStoreFile: compact not yet implemented");
#endif
}

bool DataStoreFile::should_compact() {
    return false;
}

void DataStoreFile::forget(timestamp_type ts) {
#if 0
  int earliest = 0;
  for(int i=0; i<nItem; i++) {
      if (items[i].timestamp <= ts) earliest = i;
  }
  if (earliest > 0) {
      memmove(items, items+earliest, (nItem-earliest)*sizeof(DataStoreFileRecord));
      IotsaSerial.printf("DataStoreFile: forget %d items\n", earliest);
      nItem -= earliest;
  }
#else
  IotsaSerial.println("DataStoreFile: forget not yet implemented");
#endif
}

void DataStoreFile::toJSON(JsonObject &replyObj)
{
  FILE *fp = fopen(datastoreFilename, "rb");
  if (fp == NULL) {
    IotsaSerial.println("DataStoreFile: toJSON: fopen failed");
    return;
  }
  replyObj["now"] = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  JsonArray values = replyObj.createNestedArray("data");

  while(true) {
    DataStoreFileRecord rec;
    size_t sz = fread(&rec, sizeof(rec), 1, fp);
    if (sz != 1) break;
    JsonObject curValue = values.createNestedObject();
    curValue["t"] = FORMAT_TIMESTAMP(rec.timestamp);
    curValue["ts"] = rec.timestamp;
    curValue["v"] = rec.value;
  }
  fclose(fp);
}

void DataStoreFile::toHTML(String& reply)
{
  FILE *fp = fopen(datastoreFilename, "rb");
  if (fp == NULL) {
    IotsaSerial.println("DataStoreFile: toHTML: fopen failed");
    return;
  }
  reply += "<p>Current time: ";
  auto ts = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  reply += ts.c_str();
  reply += "</p>";

  reply += "<table><tr><th>Time</th><th>Timestamp</th><th>Value</th></tr>";
  while(true) {
    DataStoreFileRecord rec;
    size_t sz = fread(&rec, sizeof(rec), 1, fp);
    if (sz != 1) break;
    reply += "<tr><td>";
    auto ts = FORMAT_TIMESTAMP(rec.timestamp);
    reply += ts.c_str();
    reply += "</td><td>";
    reply += String(rec.timestamp);
    reply += "</td><td>";
    reply += String(rec.value);
    reply += "</td></tr>";
  }
  fclose(fp);
  reply += "</table>";
}