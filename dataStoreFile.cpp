#include "dataStoreFile.h"


const char* datastoreFilename = "/littlefs/datastore.dat";
const char* datastoreDailyFilename = "/littlefs/datastore_daily.dat";

// Raw readings older than this are compressed into daily summaries.
static const timestamp_type RAW_RETENTION_SECS = 14 * 24 * 3600;

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

void DataStoreFile::forget(timestamp_type ts) {
  const char* backup = "/littlefs/datastore.001";
  (void)remove(backup);
  int sts = rename(datastoreFilename, backup);
  if (sts != 0) {
    IotsaSerial.println("DataStoreFile: forget: cannot rename datastore file");
    return;
  }
  FILE *ifp = fopen(backup, "rb");
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
  latestTimestamp = 0;
  nItem = -1;
  fclose(ifp);
  fclose(ofp);
}

void DataStoreFile::compress(timestamp_type now) {
  FILE *fp = fopen(datastoreFilename, "rb");
  if (fp == NULL) return;
  DataStoreFileRecord first;
  size_t sz = fread(&first, sizeof(first), 1, fp);
  fclose(fp);
  if (sz != 1) return;

  timestamp_type cutoff = now - RAW_RETENTION_SECS;
  int cutoff_day = (int)(cutoff / 86400);
  if ((int)(first.timestamp / 86400) >= cutoff_day) return;

  const char* backup = "/littlefs/datastore.001";
  (void)remove(backup);
  if (rename(datastoreFilename, backup) != 0) {
    IotsaSerial.println("DataStoreFile: compress: cannot rename");
    return;
  }
  FILE *ifp = fopen(backup, "rb");
  FILE *ofp = fopen(datastoreFilename, "wb");
  FILE *dfp = fopen(datastoreDailyFilename, "ab");
  if (ifp == NULL || ofp == NULL || dfp == NULL) {
    IotsaSerial.println("DataStoreFile: compress: cannot open files");
    if (ifp) fclose(ifp);
    if (ofp) fclose(ofp);
    if (dfp) fclose(dfp);
    return;
  }

  int current_day = -1;
  DataStoreDailyFileRecord drec;

  while (true) {
    DataStoreFileRecord rec;
    if (fread(&rec, sizeof(rec), 1, ifp) != 1) break;
    int day = (int)(rec.timestamp / 86400);
    if (day < cutoff_day) {
      if (day != current_day) {
        if (current_day >= 0) fwrite(&drec, sizeof(drec), 1, dfp);
        current_day = day;
        drec.min_t = rec.timestamp;
        drec.max_t = rec.timestamp;
        drec.min_v = rec.value;
        drec.max_v = rec.value;
        drec.n = 1;
      } else {
        if (rec.value < drec.min_v) { drec.min_v = rec.value; drec.min_t = rec.timestamp; }
        if (rec.value > drec.max_v) { drec.max_v = rec.value; drec.max_t = rec.timestamp; }
        drec.n++;
      }
    } else {
      fwrite(&rec, sizeof(rec), 1, ofp);
    }
  }
  if (current_day >= 0) fwrite(&drec, sizeof(drec), 1, dfp);

  fclose(ifp);
  fclose(ofp);
  fclose(dfp);
  (void)remove(backup);
  latestTimestamp = 0;
  nItem = -1;
  IotsaSerial.println("DataStoreFile: compress: done");
}

void DataStoreFile::toCSVDaily(IotsaWebServer *server) {
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(200, "text/csv", "");
  server->sendContent("date,min_t,max_t,min_v,max_v,n\r\n");

  // Stream completed daily records
  FILE *fp = fopen(datastoreDailyFilename, "rb");
  if (fp != NULL) {
    while (true) {
      DataStoreDailyFileRecord rec;
      if (fread(&rec, sizeof(rec), 1, fp) != 1) break;
      _sendDailyCSV(rec, server);
    }
    fclose(fp);
  }

  // On-the-fly aggregation of remaining raw records
  fp = fopen(datastoreFilename, "rb");
  if (fp == NULL) return;

  int current_day = -1;
  DataStoreDailyFileRecord drec;

  while (true) {
    DataStoreFileRecord rec;
    if (fread(&rec, sizeof(rec), 1, fp) != 1) break;
    int day = (int)(rec.timestamp / 86400);
    if (day != current_day) {
      if (current_day >= 0) _sendDailyCSV(drec, server);
      current_day = day;
      drec.min_t = rec.timestamp;
      drec.max_t = rec.timestamp;
      drec.min_v = rec.value;
      drec.max_v = rec.value;
      drec.n = 1;
    } else {
      if (rec.value < drec.min_v) { drec.min_v = rec.value; drec.min_t = rec.timestamp; }
      if (rec.value > drec.max_v) { drec.max_v = rec.value; drec.max_t = rec.timestamp; }
      drec.n++;
    }
  }
  if (current_day >= 0) _sendDailyCSV(drec, server);
  fclose(fp);
}

void DataStoreFile::_sendDailyCSV(DataStoreDailyFileRecord rec, IotsaWebServer* server) {
  char buf[200];
  std::string date = FORMAT_DATE(rec.min_t);
  std::string min_ts = FORMAT_TIMESTAMP(rec.min_t);
  std::string max_ts = FORMAT_TIMESTAMP(rec.max_t);
  snprintf(buf, sizeof(buf), "\"%s\",\"%s\",\"%s\",%f,%f,%d\r\n",
    date.c_str(),
    min_ts.c_str(),
    max_ts.c_str(),
    (float)rec.min_v,
    (float)rec.max_v,
    (int)rec.n
  );
  server->sendContent(String(buf));
}

void DataStoreFile::toJSON(JsonObject &replyObj, bool summary)
{
  FILE *fp = fopen(datastoreFilename, "rb");
  if (fp == NULL) {
    IotsaSerial.println("DataStoreFile: toJSON: fopen failed");
    return;
  }
  replyObj["now"] = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  JsonArray values = replyObj.createNestedArray("data");

  if (summary) {
    int count = 0;
    DataStoreFileRecord rec;
    size_t sz = fread(&rec, sizeof(rec), 1, fp);
    if (sz != 1) {
      replyObj["count"] = 0;
      fclose(fp);
      return;
    }
    count++;
    JsonObject curValue = values.createNestedObject();
    _storeRec(rec, curValue);
    while(fread(&rec, sizeof(rec), 1, fp) == 1) {
      count++;
    }
    curValue = values.createNestedObject();
    _storeRec(rec, curValue);
    replyObj["count"] = count;
    fclose(fp);
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

void DataStoreFile::toHTML(String& reply, bool summary)
{
  FILE *fp = fopen(datastoreFilename, "rb");
  if (fp == NULL) {
    IotsaSerial.println("DataStoreFile: toHTML: fopen failed");
    return;
  }
  reply += "<p>Current time: ";
  auto ts = FORMAT_TIMESTAMP(GET_TIMESTAMP());
  reply += ts.c_str();
  reply += ", " + String(size()) + " raw entries.";
  reply += "<br>Raw data: <a href='/datalogger/data.csv'>/datalogger/data.csv</a>.";
  reply += "<br>Daily summary: <a href='/datalogger/data_daily.csv'>/datalogger/data_daily.csv</a>.";
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

void DataStoreFile::toCSV(IotsaWebServer *server) {
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(200, "text/csv", "");
  server->sendContent("t,ts,v\r\n");
  FILE *fp = fopen(datastoreFilename, "rb");
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
