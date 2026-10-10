"""Bring the local store for a device up to date with the device itself."""
from dataclasses import dataclass

import requests

from . import records
from .device import DataLoggerDevice
from .store import DataStore


@dataclass
class PullResult:
    raw_count: int
    device_days: int
    days_from_raw: bool  # True if the device has no daily endpoint, and days were computed from raw data
    added_days: int
    total_days: int


def pull(dev: DataLoggerDevice, store: DataStore, verbose: bool = False) -> PullResult:
    """Fetch raw readings (stored as detail data) and daily summaries (merged into the history).

    The device's daily summaries cover everything it still has: compressed days
    beyond its raw retention window, plus on-the-fly summaries of the raw days.
    Firmware without the daily endpoint gets its days computed from the raw data.
    """
    readings = dev.fetch_raw()
    if verbose and readings:
        print(f"Got {len(readings)} raw readings "
              f"({records.ts_to_date(readings[0].ts)} .. {records.ts_to_date(readings[-1].ts)})")
    if readings:
        store.save_detail(readings)
        if verbose:
            print(f"Wrote detail data to {store.detail_path}")

    days_from_raw = False
    try:
        new_days = dev.fetch_daily()
    except requests.exceptions.HTTPError as e:
        if e.response is None or e.response.status_code != 404:
            raise
        if verbose:
            print("Device has no daily summaries, computing them from raw data")
        new_days = records.aggregate_daily(readings)
        days_from_raw = True
    if verbose and new_days:
        print(f"Got {len(new_days)} daily summaries ({min(new_days)} .. {max(new_days)})")

    existing_days = store.load_daily()
    merged = records.merge_daily(existing_days, new_days)
    store.save_daily(merged)
    return PullResult(
        raw_count=len(readings),
        device_days=len(new_days),
        days_from_raw=days_from_raw,
        added_days=len(merged) - len(existing_days),
        total_days=len(merged),
    )
