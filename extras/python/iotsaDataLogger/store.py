"""Local storage of iotsaDataLogger data, one set of files per device.

In the data directory, for device ``<name>``:

- ``<name>.csv``: long-term daily min/max history.
- ``<name>-detail.csv``: raw readings from the most recent pull (overwritten).
- ``<name>.json``: settings for presenting the data (optional, see DEFAULT_CONFIG).
"""
import copy
import json
import os
from typing import Any, Dict, List

from .records import (
    DailyHistory,
    RawReading,
    read_daily_csv,
    read_raw_csv,
    write_daily_csv,
    write_raw_csv,
)

# Missing keys in <name>.json are taken from here.
DEFAULT_CONFIG: Dict[str, Any] = {
    # Human-readable description, used in graph titles.
    "description": None,
    # For the sunshine overlay: {"name": ..., "latitude": ..., "longitude": ...}.
    # Without latitude/longitude the name is geocoded.
    "location": None,
    # The measured values. Only one channel ("v") is supported by the firmware.
    "channels": [
        {
            "name": "v",
            "label": "Voltage",
            "unit": "V",
            # List of {"value": ..., "label": ...}
            "thresholds": [],
        }
    ],
}


class DataStore:
    def __init__(self, datadir: str, name: str):
        self.datadir = datadir
        self.name = name
        self.daily_path = os.path.join(datadir, f"{name}.csv")
        self.detail_path = os.path.join(datadir, f"{name}-detail.csv")
        self.config_path = os.path.join(datadir, f"{name}.json")

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

    def load_config(self) -> Dict[str, Any]:
        config = copy.deepcopy(DEFAULT_CONFIG)
        if os.path.exists(self.config_path):
            with open(self.config_path) as fp:
                config.update(json.load(fp))
        return config
