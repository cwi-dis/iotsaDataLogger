"""Graphs of iotsaDataLogger data.

The functions return a matplotlib Figure; the caller decides whether to show
or save it. Select a matplotlib backend before calling them if needed.
"""
from typing import Any, Dict, List, Optional, Tuple

import matplotlib.dates as mdates
import pandas as pd

from .records import DailyHistory, RawReading


def _value_label(channel: Dict[str, Any]) -> str:
    unit = channel.get("unit")
    return f'{channel["label"]} ({unit})' if unit else channel["label"]


def plot_daily(
    days: DailyHistory,
    title: str,
    channel: Dict[str, Any],
    sunlight: Optional[Tuple[pd.DataFrame, str]] = None,
):
    """Daily min/max history, optionally with a sunshine panel below it.

    sunlight is (dataframe, place name) as returned by sunlight.fetch_sunlight().
    """
    import matplotlib.pyplot as plt

    df = pd.DataFrame([vars(days[date]) for date in sorted(days)])
    df["date"] = pd.to_datetime(df["date"])

    if sunlight is not None:
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 7), sharex=True)
    else:
        fig, ax1 = plt.subplots(1, 1, figsize=(14, 4))

    ax1.fill_between(df["date"], df["min_v"], df["max_v"], alpha=0.35, color="steelblue")
    ax1.plot(df["date"], df["min_v"], linewidth=0.8, color="steelblue")
    ax1.plot(df["date"], df["max_v"], linewidth=0.8, color="steelblue")
    ax1.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m"))
    ax1.xaxis.set_major_locator(mdates.MonthLocator())
    ax1.set_ylabel(_value_label(channel))
    ax1.set_title(f'{title} — daily min/max {channel["label"].lower()}')
    ax1.grid(True, alpha=0.3)

    if sunlight is not None:
        wx, place_name = sunlight
        ax2.bar(wx["date"], wx["sunshine_h"], color="gold", alpha=0.8, label="Sunshine (h/day)", width=1)
        ax2b = ax2.twinx()
        ax2b.plot(wx["date"], wx["radiation"], color="orange", linewidth=0.8, alpha=0.7, label="Radiation (MJ/m²)")
        ax2.set_ylabel("Sunshine hours/day")
        ax2b.set_ylabel("Solar radiation (MJ/m²)")
        ax2.set_ylim(0, 16)
        ax2.grid(True, alpha=0.3)
        ax2.set_title(f"Sunshine — {place_name}")
        lines1, labels1 = ax2.get_legend_handles_labels()
        lines2, labels2 = ax2b.get_legend_handles_labels()
        ax2.legend(lines1 + lines2, labels1 + labels2, loc="upper left", fontsize=8)
        plt.setp(ax2.get_xticklabels(), rotation=45, ha="right")
    else:
        plt.setp(ax1.get_xticklabels(), rotation=45, ha="right")

    fig.tight_layout()
    return fig


def plot_raw(
    readings: List[RawReading],
    title: str,
    channel: Dict[str, Any],
    last_days: Optional[int] = None,
):
    """Raw readings, with the channel's thresholds as horizontal lines."""
    import matplotlib.pyplot as plt

    df = pd.DataFrame([vars(r) for r in readings])
    df["t"] = pd.to_datetime(df["t"])
    df = df.sort_values("t")
    if last_days:
        cutoff = df["t"].max() - pd.Timedelta(days=last_days)
        df = df[df["t"] > cutoff]

    fig, ax = plt.subplots(figsize=(14, 5))
    ax.plot(df["t"], df["v"], linewidth=0.8, color="steelblue")
    thresholds = channel.get("thresholds", [])
    for th in thresholds:
        ax.axhline(th["value"], color="crimson", linestyle="--", linewidth=0.8, label=th.get("label"))
    ax.xaxis.set_major_formatter(mdates.DateFormatter("%m-%d %H:%M"))
    ax.set_ylabel(_value_label(channel))
    ax.set_title(f'{title} — recent raw {channel["label"].lower()}')
    ax.grid(True, alpha=0.3)
    if any(th.get("label") for th in thresholds):
        ax.legend(loc="lower left", fontsize=8)
    plt.setp(ax.get_xticklabels(), rotation=45, ha="right")
    fig.tight_layout()
    return fig
