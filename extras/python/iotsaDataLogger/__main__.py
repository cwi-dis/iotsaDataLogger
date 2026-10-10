"""iotsaDataLogger command line tool: retrieve, store and graph data from iotsaDataLogger devices."""
import argparse
import sys

from . import records
from .annotations import DEFAULT_CHANNEL, parse_annotations
from .device import DataLoggerDevice, device_name
from .store import DataStore


def _open_device(args) -> DataLoggerDevice:
    auth = tuple(args.credentials.split(":", 1)) if args.credentials else None
    return DataLoggerDevice(
        args.device,
        protocol=args.protocol,
        port=args.port,
        noverify=args.noverify,
        bearer=args.bearer,
        auth=auth,
        verbose=args.verbose,
    )


def _load_config(store: DataStore):
    """Presentation config from the store's annotations, and the "v" channel from it."""
    config, warnings = parse_annotations(store.load_annotations())
    for w in warnings:
        print(f"{store.annotations_path}: {w}", file=sys.stderr)
    channel = config["channels"].get("v", {**DEFAULT_CHANNEL, "thresholds": []})
    return config, channel


def _title(name, config) -> str:
    description = config.get("description")
    return f"{name} ({description})" if description else name


def _show_or_save(fig, args) -> None:
    if args.save:
        fig.savefig(args.save, dpi=130)
        print(f"Saved to {args.save}")
    else:
        import matplotlib.pyplot as plt
        plt.show()


def cmd_pull(args) -> None:
    from .pull import pull

    dev = _open_device(args)
    store = DataStore(args.datadir, dev.name)
    result = pull(dev, store, verbose=args.verbose)
    if not result.device_days:
        raise SystemExit(f"No data returned from {dev.host}")
    source = "computed from raw data" if result.days_from_raw else "from device"
    print(f"{dev.name}: {result.device_days} day(s) {source}, "
          f"{result.added_days} new day(s) added, {result.total_days} day(s) total in {store.daily_path}")


def cmd_dump(args) -> None:
    dev = _open_device(args)
    if args.raw:
        records.write_raw_csv(sys.stdout, dev.fetch_raw())
    else:
        records.write_daily_csv(sys.stdout, dev.fetch_daily())


def cmd_annotate(args) -> None:
    dev = _open_device(args)
    store = DataStore(args.datadir, dev.name)
    changes = {}
    if args.from_local:
        changes.update(store.load_annotations())
    for setting in args.settings:
        key, sep, value = setting.partition("=")
        if not sep:
            raise SystemExit(f"Expected KEY=VALUE, got {setting!r}")
        changes[key] = value
    if dev.fetch_annotations() is None:
        raise SystemExit(f"{dev.name}: firmware has no annotations module")
    if changes:
        dev.set_annotations(changes)
    annotations = dev.fetch_annotations()
    for key in sorted(annotations):
        print(f"{key}={annotations[key]}")
    for w in parse_annotations(annotations)[1]:
        print(f"{dev.name}: {w}", file=sys.stderr)
    store.save_annotations(annotations)


def cmd_plot(args) -> None:
    from .plot import plot_daily
    from .sunlight import fetch_sunlight

    name = device_name(args.device)
    store = DataStore(args.datadir, name)
    config, channel = _load_config(store)
    days = store.load_daily()
    if not days:
        raise SystemExit(f"No data in {store.daily_path}")

    location = config.get("location")
    if args.sunlight is not None:
        location = {"name": args.sunlight} if args.sunlight else None
    sunlight = None
    if location:
        sunlight = fetch_sunlight(location, min(days), max(days))

    fig = plot_daily(days, _title(name, config), channel, sunlight)
    _show_or_save(fig, args)


def cmd_recent(args) -> None:
    from .plot import plot_raw

    name = device_name(args.device)
    store = DataStore(args.datadir, name)
    config, channel = _load_config(store)
    readings = store.load_detail()
    if not readings:
        raise SystemExit(f"No detail data at {store.detail_path} — run 'pull {name}' first")

    fig = plot_raw(readings, _title(name, config), channel, last_days=args.days)
    _show_or_save(fig, args)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-v", "--verbose", action="store_true", help="Print what is happening")
    subparsers = parser.add_subparsers(dest="command", required=True, metavar="COMMAND")

    # Options shared by all sub-commands
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("device", metavar="DEVICE",
                        help="iotsa device name (e.g. accugroot or accugroot.local) or hostname")

    # Options for sub-commands that store or read local data
    local = argparse.ArgumentParser(add_help=False)
    local.add_argument("--datadir", default=".", metavar="DIR",
                       help="Directory holding DEVICE.csv, DEVICE-detail.csv and DEVICE.json (annotations) (default: current directory)")

    # Options for sub-commands that talk to the device. Names match the iotsa command line tool.
    remote = argparse.ArgumentParser(add_help=False)
    remote.add_argument("--protocol", metavar="PROTO", help="Access protocol (default: sniff the device; allowed: http, https)")
    remote.add_argument("--port", type=int, metavar="PORT", help="Port number (default depends on protocol)")
    remote.add_argument("--noverify", action="store_true", help="Do not verify HTTPS certificates")
    remote.add_argument("--bearer", metavar="TOKEN", help="Add Authorization: Bearer TOKEN header line")
    remote.add_argument("--credentials", metavar="USER:PASS", help="Add Authorization: Basic header line with given credentials")

    # Options for sub-commands that produce a graph
    graph = argparse.ArgumentParser(add_help=False)
    graph.add_argument("--save", metavar="FILE", help="Save the graph to FILE (e.g. a .png) instead of showing it")

    p = subparsers.add_parser(
        "pull", parents=[common, local, remote],
        help="Fetch the device's raw data into DEVICE-detail.csv, and merge its daily summaries into DEVICE.csv",
    )
    p.set_defaults(func=cmd_pull)

    p = subparsers.add_parser(
        "dump", parents=[common, remote],
        help="Print the device's daily summaries (or raw readings) as CSV, without storing anything",
    )
    p.add_argument("--raw", action="store_true", help="Raw per-reading data instead of daily summaries")
    p.set_defaults(func=cmd_dump)

    p = subparsers.add_parser(
        "annotate", parents=[common, local, remote],
        help="Show or change the device's annotations (and update the local copy in DEVICE.json)",
    )
    p.add_argument("--from-local", action="store_true",
                   help="Upload the annotations from DEVICE.json to the device (e.g. after reflashing)")
    p.add_argument("settings", nargs="*", metavar="KEY=VALUE",
                   help="Annotations to set; an empty VALUE removes KEY")
    p.set_defaults(func=cmd_annotate)

    p = subparsers.add_parser(
        "plot", parents=[common, local, graph],
        help="Graph the long-term daily min/max history from DEVICE.csv, with sunshine overlay",
    )
    p.add_argument("--sunlight", metavar="LOCATION",
                   help="Location for the sunshine overlay, overriding the annotations; pass '' to disable")
    p.set_defaults(func=cmd_plot)

    p = subparsers.add_parser(
        "recent", parents=[common, local, graph],
        help="Graph the raw readings from DEVICE-detail.csv (run pull first to refresh)",
    )
    p.add_argument("--days", type=int, metavar="N", help="Only show the last N days (default: everything)")
    p.set_defaults(func=cmd_recent)

    args = parser.parse_args()
    if getattr(args, "save", None):
        import matplotlib
        matplotlib.use("Agg")
    args.func(args)


if __name__ == "__main__":
    main()
