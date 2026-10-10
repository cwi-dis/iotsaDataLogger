"""Interpretation of device annotations (the device's ``annotations`` module).

Annotations are flat string key/value pairs, which the device stores but does
not interpret. This module defines the key convention:

- ``description``: human-readable description, used in graph titles.
- ``location``: ``LAT,LON``, for the sunshine overlay.
- ``location.name``: place name (geocoded if ``location`` is not set).
- ``CH.label``, ``CH.unit``: label and unit of channel ``CH`` (``v`` is the only one for now).
- ``CH.threshold``, ``CH.threshold.label``: a threshold line for channel CH, with optional label.
  More thresholds as ``CH.threshold.N`` / ``CH.threshold.N.label``.
"""
import re
from typing import Any, Dict, List, Tuple

Annotations = Dict[str, str]

# Used for channels (or channel fields) that have no annotations.
DEFAULT_CHANNEL = {"label": "Voltage", "unit": "V"}

_CHANNEL_KEY = re.compile(r"^(?P<ch>[A-Za-z0-9_]+)\.(?P<field>label|unit|threshold(?:\.(?P<n>[0-9]+))?(?P<tlabel>\.label)?)$")


def parse_annotations(annotations: Annotations) -> Tuple[Dict[str, Any], List[str]]:
    """Turn flat annotations into a presentation config.

    Returns (config, warnings). config has "description", "location" (None or a
    dict with "name" and/or "latitude"/"longitude") and "channels" (dict mapping
    channel name to {"label", "unit", "thresholds": [{"value", "label"}]}).
    Keys not following the convention are reported in warnings.
    """
    warnings = []
    config: Dict[str, Any] = {"description": None, "location": None, "channels": {}}
    location: Dict[str, Any] = {}
    thresholds: Dict[Tuple[str, str], Dict[str, Any]] = {}

    def channel(ch):
        return config["channels"].setdefault(ch, {**DEFAULT_CHANNEL, "thresholds": []})

    for key, value in sorted(annotations.items()):
        if key == "description":
            config["description"] = value
        elif key == "location":
            try:
                lat, lon = (float(x) for x in value.split(","))
            except ValueError:
                warnings.append(f"annotation location={value!r}: expected LATITUDE,LONGITUDE")
                continue
            location.update(latitude=lat, longitude=lon)
        elif key == "location.name":
            location["name"] = value
        elif m := _CHANNEL_KEY.match(key):
            ch, field = m.group("ch"), m.group("field")
            if field in ("label", "unit"):
                channel(ch)[field] = value
                continue
            th = thresholds.setdefault((ch, m.group("n") or ""), {"value": None, "label": None})
            if m.group("tlabel"):
                th["label"] = value
            else:
                try:
                    th["value"] = float(value)
                except ValueError:
                    warnings.append(f"annotation {key}={value!r}: expected a number")
        else:
            warnings.append(f"annotation {key}: unknown key")

    for (ch, n), th in sorted(thresholds.items()):
        if th["value"] is None:
            warnings.append(f"annotation {ch}.threshold{'.' + n if n else ''}: no valid value, ignored")
            continue
        channel(ch)["thresholds"].append(th)
    if location:
        config["location"] = location
    return config, warnings
