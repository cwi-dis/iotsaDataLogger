"""Historical sunshine data from open-meteo.com, for overlaying on solar-charged data."""
from typing import Any, Dict, Tuple

import pandas as pd
import requests


def geocode(name: str) -> Dict[str, Any]:
    """Look up a place name: returns {"name", "latitude", "longitude"}."""
    resp = requests.get(
        "https://geocoding-api.open-meteo.com/v1/search",
        params={"name": name, "count": 1, "language": "en", "format": "json"},
        timeout=10,
    )
    resp.raise_for_status()
    results = resp.json().get("results")
    if not results:
        raise ValueError(f"Could not geocode location: {name}")
    r = results[0]
    return {"name": r.get("name", name), "latitude": r["latitude"], "longitude": r["longitude"]}


def fetch_sunlight(location: Dict[str, Any], start_date: str, end_date: str) -> Tuple[pd.DataFrame, str]:
    """Daily sunshine hours and radiation for a location between two YYYY-MM-DD dates.

    location is {"name", "latitude", "longitude"}; without latitude/longitude the
    name is geocoded. Returns (dataframe with date/sunshine_h/radiation columns, place name).
    """
    if "latitude" not in location or "longitude" not in location:
        location = geocode(location["name"])
    resp = requests.get(
        "https://archive-api.open-meteo.com/v1/archive",
        params={
            "latitude": location["latitude"],
            "longitude": location["longitude"],
            "start_date": start_date,
            "end_date": end_date,
            "daily": "sunshine_duration,shortwave_radiation_sum",
            "timezone": "auto",
        },
        timeout=15,
    )
    resp.raise_for_status()
    daily = resp.json()["daily"]
    df = pd.DataFrame({
        "date": pd.to_datetime(daily["time"]),
        "sunshine_h": [s / 3600 for s in daily["sunshine_duration"]],
        "radiation": daily["shortwave_radiation_sum"],
    })
    place_name = location.get("name") or f'{location["latitude"]}, {location["longitude"]}'
    return df, place_name
