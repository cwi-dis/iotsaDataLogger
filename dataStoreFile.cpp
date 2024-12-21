#include "dataStoreFile.h"

const int COMPACT_THRESHOLD = 1000; // At that size this are difficult to fit into memory for toHTML and toJSON.

const char* datastoreFilename = "/littlefs/datastore.dat";
const char* datastoreBackup = "/littlefs/datastore.001";

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
  if (nItem >= 0) nItem++;
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
  long pos = ftell(fp);
  nItem = (pos / sizeof(rec))+1;
  size_t sz = fread(&rec, sizeof(rec), 1, fp);
  if (sz != 1) {
    IotsaSerial.println("DataStoreFile: latest: fread failed");
  }
  fclose(fp);
  return rec.timestamp;
}

int DataStoreFile::size() {
  if (nItem <= 0) {
    // Getting most recent timestamp also initializes nItem
    (void)latest();
  }
  return nItem;
}

void DataStoreFile::archive()
{
  (void)remove(datastoreBackup);
  int sts = rename(datastoreFilename, datastoreBackup);
  if (sts != 0) {
    IotsaSerial.println("DataStoreFile: compact: cannot rename datastore file");
    return;
  }
  latestTimestamp = 0;
  nItem = -1;
}

bool DataStoreFile::should_archive() {
    return size() > COMPACT_THRESHOLD;
}

void DataStoreFile::forget(timestamp_type ts) {
  (void)remove(datastoreBackup);
  int sts = rename(datastoreFilename, datastoreBackup);
  if (sts != 0) {
    IotsaSerial.println("DataStoreFile: forget: cannot rename datastore file");
    return;
  }
  FILE *ifp = fopen(datastoreBackup, "rb");
  FILE *ofp = fopen(datastoreFilename, "wb");
  if (ifp == NULL || ofp == NULL) {
    IotsaSerial.println("DataStoreFile: forget: cannot open files");
    return;
  }
  while(true) {
    DataStoreFileRecord rec;
    size_t sz = fread(&rec, sizeof(rec), 1, ifp);
    if (sz != 1) break;
    if (rec.timestamp >= ts) {
      (void)fwrite(&rec, sizeof(rec), 1, ofp);
    }
  }
  IotsaSerial.println("DataStoreFile: moved datastore to backup");
  latestTimestamp = 0;
  nItem = -1;
  fclose(ifp);
  fclose(ofp);
}

void DataStoreFile::toJSON(JsonObject &replyObj, bool archived, bool summary)
{
  FILE *fp = fopen(archived ? datastoreBackup : datastoreFilename, "rb");
  if (fp == NULL) {
    IotsaSerial.println("DataStoreFile: toJSON: fopen failed");
    return;
  }
  if (!archived) {
    replyObj["now"] = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  }
  JsonArray values = replyObj.createNestedArray("data");

  if (summary) {
    int count = 0;
    // Only output first and last record
    DataStoreFileRecord rec;
    size_t sz = fread(&rec, sizeof(rec), 1, fp);
    if (sz != 1) {
      replyObj["count"] = 0;
      return;
    }
    count++;
    JsonObject curValue = values.createNestedObject();
    _storeRec(rec, curValue);
    while(fread(&rec, sizeof(rec), 1, fp) == 1) {
      // Do nothing
      count++;
    }
    // Now rec has the last record.
    curValue = values.createNestedObject();
    _storeRec(rec, curValue);
    replyObj["count"] = count;
    return;
  }
  while(true) {
    DataStoreFileRecord rec;
    size_t sz = fread(&rec, sizeof(rec), 1, fp);
    if (sz != 1) break;
    JsonObject curValue = values.createNestedObject();
    _storeRec(rec, curValue);
  }
  fclose(fp);
}

void DataStoreFile::_storeRec(DataStoreFileRecord rec, JsonObject obj) {
    obj["t"] = FORMAT_TIMESTAMP(rec.timestamp);
    obj["ts"] = rec.timestamp;
    obj["v"] = rec.value;
}

void DataStoreFile::toHTML(String& reply, bool archived, bool summary)
{
  FILE *fp = fopen(archived ? datastoreBackup : datastoreFilename, "rb");
  if (fp == NULL) {
    IotsaSerial.println("DataStoreFile: toHTML: fopen failed");
    return;
  }
  reply += "<p>Current time: ";
  auto ts = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  reply += ts.c_str();
  reply += ", " + String(size()) + " entries.";
  if (should_archive()) {
    reply += "<br>You should compact the datastore.";
  }
  reply += "<br>Full data available at ";
  if (archived) {
    reply += "<a href='/datalogger/archive.csv'>/datalogger/archive.csv</a>.";
  } else {
    reply += "<a href='/datalogger/data.csv'>/datalogger/data.csv</a>.";
  }
  reply += "</p>";

  reply += "<table><tr><th>Time</th><th>Timestamp</th><th>Value</th></tr>";
  if (summary) {
    DataStoreFileRecord rec;
    size_t sz = fread(&rec, sizeof(rec), 1, fp);
    if (sz == 1) {
      reply += "<tr><td>";
      std::string ts = FORMAT_TIMESTAMP(rec.timestamp);
      reply += ts.c_str();
      reply += "</td><td>";
      reply += String(rec.timestamp);
      reply += "</td><td>";
      reply += String(rec.value);
      reply += "</td></tr>";

      while (fread(&rec, sizeof(rec), 1, fp) == 1) { }

      reply += "<tr><td>";
      ts = FORMAT_TIMESTAMP(rec.timestamp);
      reply += ts.c_str();
      reply += "</td><td>";
      reply += String(rec.timestamp);
      reply += "</td><td>";
      reply += String(rec.value);
      reply += "</td></tr>";
    }
  } else {
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
  }
  fclose(fp);
  reply += "</table>";
}

void DataStoreFile::toCSV(IotsaWebServer *server, bool archived) {
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(200, "text/csv", "");
  server->sendContent("t,ts,v\r\n");
  FILE *fp = fopen(archived ? datastoreBackup : datastoreFilename, "rb");
  if (fp == NULL) {
    IotsaSerial.println("DataStoreFile: toCSV: fopen failed");
    return;
  }

  while(true) {
    DataStoreFileRecord rec;
    size_t sz = fread(&rec, sizeof(rec), 1, fp);
    if (sz != 1) break;
    _sendCSV(rec, server);
  }
  fclose(fp);
}

void DataStoreFile::_sendCSV(DataStoreFileRecord rec, IotsaWebServer* server) {
  char buf[100];
  std::string tstring = FORMAT_TIMESTAMP(rec.timestamp);
  snprintf(buf, sizeof(buf), "\"%s\",%ld,%f\r\n",
    tstring.c_str(),
    (long)rec.timestamp,
    (float)rec.value
  );
  String sBuf(buf);
  server->sendContent(sBuf);
}
