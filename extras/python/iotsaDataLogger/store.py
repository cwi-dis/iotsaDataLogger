"""Local storage of iotsaDataLogger data, one set of files per device.

In the data directory, for device ``<name>``:

- ``<name>.csv``: long-term daily min/max history.
- ``<name>-detail.csv``: raw readings from the most recent pull (overwritten).
- ``<name>.json``: the device's annotations (see annotations.py): cached by
  pull from devices that have them, otherwise maintained by hand. Optional.
"""
import json
import os
from typing import List

from .annotations import Annotations
from .records import (
    DailyHistory,
    RawReading,
    read_daily_csv,
    read_raw_csv,
    write_daily_csv,
    write_raw_csv,
)


class DataStore:
    def __init__(self, datadir: str, name: str):
        self.datadir = datadir
        self.name = name
        self.daily_path = os.path.join(datadir, f"{name}.csv")
        self.detail_path = os.path.join(datadir, f"{name}-detail.csv")
        self.annotations_path = os.path.join(datadir, f"{name}.json")

    def load_daily(self) -> DailyHistory:
        """The daily history, or {} if there is none yet."""
        if not os.path.exists(self.daily_path):
            return {}
        with open(self.daily_path, newline="") as fp:
            return read_daily_csv(fp)

    def save_daily(self, days: DailyHistory) -> None:
        with open(self.daily_path, "w", newline="") as fp:
            write_daily_csv(fp, days)

    def load_detail(self) -> List[RawReading]:
        """Raw readings from the last pull, or [] if there are none."""
        if not os.path.exists(self.detail_path):
            return []
        with open(self.detail_path, newline="") as fp:
            return read_raw_csv(fp)

    def save_detail(self, readings: List[RawReading]) -> None:
        with open(self.detail_path, "w", newline="") as fp:
            write_raw_csv(fp, readings)

    def load_annotations(self) -> Annotations:
        """The annotations, or {} if there are none."""
        if not os.path.exists(self.annotations_path):
            return {}
        with open(self.annotations_path) as fp:
            return json.load(fp)

    def save_annotations(self, annotations: Annotations) -> None:
        with open(self.annotations_path, "w") as fp:
            json.dump(annotations, fp, indent=2, sort_keys=True)
            fp.write("\n")
