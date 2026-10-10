"""Data records for iotsaDataLogger readings, and the pure logic on them.

Two record kinds, matching the two CSV endpoints of the firmware:

- RawReading: one measurement (``/datalogger/data.csv``: ``t,ts,v``).
- DailySummary: min/max over one UTC day (``/datalogger/data_daily.csv``:
  ``date,min_t,max_t,min_v,max_v,n``). ``date`` is the UTC day, ``min_t`` and
  ``max_t`` are device-local time.

The CSV readers/writers here produce exactly the formats used for the
long-term history files, so diffs of those files stay minimal.
"""
import csv
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import IO, Dict, Iterable, List


@dataclass
class RawReading:
    ts: int
    t: str
    v: float


@dataclass
class DailySummary:
    date: str
    min_t: str
    max_t: str
    min_v: float
    max_v: float
    n: int


DailyHistory = Dict[str, DailySummary]


def ts_to_date(ts: int) -> str:
    """UTC day (YYYY-MM-DD) for a Unix timestamp, same as the firmware's FORMAT_DATE."""
    return datetime.fromtimestamp(ts, tz=timezone.utc).strftime("%Y-%m-%d")


def read_raw_csv(lines: Iterable[str]) -> List[RawReading]:
    readings = []
    for row in csv.DictReader(lines):
        ts, t, v = row.get("ts"), row.get("t"), row.get("v")
        if not ts or not v:
            continue
        readings.append(RawReading(int(round(float(ts))), t, float(v)))
    return readings


def write_raw_csv(fp: IO[str], readings: Iterable[RawReading]) -> None:
    fp.write("t,ts,v\n")
    for r in sorted(readings, key=lambda r: r.ts):
        fp.write(f'"{r.t}",{r.ts},{r.v:.6f}\n')


def read_daily_csv(lines: Iterable[str]) -> DailyHistory:
    """Read daily summaries. Rows for the same date are combined.

    The device can send several rows for one date: its on-the-fly summaries
    group consecutive readings, so a clock jump back across midnight splits a
    day (#14). Those rows cover disjoint readings, so their n values add up.
    """
    days: DailyHistory = {}
    for row in csv.DictReader(lines):
        d = DailySummary(
            date=row["date"],
            min_t=row["min_t"],
            max_t=row["max_t"],
            min_v=float(row["min_v"]),
            max_v=float(row["max_v"]),
            n=int(row["n"]),
        )
        e = days.get(d.date)
        if e is not None:
            min_v, min_t = (d.min_v, d.min_t) if d.min_v < e.min_v else (e.min_v, e.min_t)
            max_v, max_t = (d.max_v, d.max_t) if d.max_v > e.max_v else (e.max_v, e.max_t)
            d = DailySummary(d.date, min_t, max_t, min_v, max_v, e.n + d.n)
        days[d.date] = d
    return days


def write_daily_csv(fp: IO[str], days: DailyHistory) -> None:
    fp.write("date,min_t,max_t,min_v,max_v,n\n")
    for date in sorted(days):
        d = days[date]
        fp.write(f'"{d.date}","{d.min_t}","{d.max_t}",{d.min_v:.6f},{d.max_v:.6f},{d.n}\n')


def aggregate_daily(readings: Iterable[RawReading]) -> DailyHistory:
    """Summarise raw readings per UTC day, the same way the firmware's compress() does."""
    days: DailyHistory = {}
    for r in sorted(readings, key=lambda r: r.ts):
        date = ts_to_date(r.ts)
        d = days.get(date)
        if d is None:
            days[date] = DailySummary(date, r.t, r.t, r.v, r.v, 1)
            continue
        d.n += 1
        if r.v < d.min_v:
            d.min_v, d.min_t = r.v, r.t
        if r.v > d.max_v:
            d.max_v, d.max_t = r.v, r.t
    return days


def merge_daily(existing: DailyHistory, new: DailyHistory) -> DailyHistory:
    """Merge two daily histories.

    min/max are idempotent under overlapping raw readings, so taking the
    min-of-mins and max-of-maxes across two pulls of the same day is always
    correct. n is not (we don't keep enough raw history to dedup exactly
    across pulls), so it takes max(old_n, new_n) as a non-decreasing estimate
    rather than summing.
    """
    merged = dict(existing)
    for date, d in new.items():
        e = merged.get(date)
        if e is None:
            merged[date] = d
            continue
        min_v, min_t = (d.min_v, d.min_t) if d.min_v < e.min_v else (e.min_v, e.min_t)
        max_v, max_t = (d.max_v, d.max_t) if d.max_v > e.max_v else (e.max_v, e.max_t)
        merged[date] = DailySummary(date, min_t, max_t, min_v, max_v, max(e.n, d.n))
    return merged
