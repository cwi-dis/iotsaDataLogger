import sys
import os
import argparse
from .data_logger import DataLogger

def main():
    parser = argparse.ArgumentParser(description="Get data from iotsaDataLogger")
    parser.add_argument("-d", "--device", action="store", metavar="HOST", help="Hostname or IP to get data from")
    parser.add_argument("-D", "--devarg", nargs="*", default=[], metavar="NAME=VALUE", help="iotsa arguments")
    parser.add_argument("-i", "--input", nargs="*", default=[], metavar="FILE", help="Get data from CSV file in stead of from device")
    parser.add_argument("-g", "--graph", action="store_true", help="Graph data")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose messages")
    parser.add_argument("-o", "--output", action="store", metavar="FILE", help="Store data to file as CSV")
    parser.add_argument("-m", "--merge", action="store_true", help="Merge CSV data into output file in stead of appending")
    parser.add_argument("--raw", action="store_true", help="Get raw per-reading data instead of daily summaries from device")
    parser.add_argument("--clean", action="store_true", help="Archive data on device after getting it")
    parser.add_argument("--sunlight", action="store", metavar="LOCATION", help="Overlay sunshine data for the given location (implies --graph)")
    args = parser.parse_args()
    logger = DataLogger(verbose=args.verbose)
    if args.device:
        kwargs = {}
        for pair in args.devarg:
            k, v = pair.split('=')
            kwargs[k] = v
        logger.read_device(args.device, args.raw, kwargs)
    for a in args.input:
        logger.read_file(a)
    if args.merge:
        if not args.output:
            print(f'{parser.prog}: --merge requires --output')
        else:
            logger.read_file(args.output)
    if args.output:
        logger.write_file(args.output)
    elif not args.graph:
        logger.write_file('-')
    if args.graph or args.sunlight:
        logger.graph(sunlight_location=args.sunlight)
    if args.clean:
        print(f'{parser.prog}: --clean not yet implemented')

if __name__ == '__main__':
    main()
