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
    parser.add_argument("-a", "--archive", action="store_true", help="Get archived data in stead of current data from device")
    parser.add_argument("--clean", action="store_true", help="Archive data one device after getting it")
    args = parser.parse_args()
    logger = DataLogger(verbose=args.verbose)
    if args.device:
        kwargs = {}
        for pair in args.devarg:
            k, v = pair.split('=')
            kwargs[k] = v
        logger.read_device(args.device, args.archive, kwargs)
    for a in args.input:
        logger.read_file(a)
    if args.merge:
        if not args.output:
            print(f'{parser.prog}: --merge requires --output')
        else:
            logger.read_file(args.output)
    if args.output:
        logger.write_file(args.output)
    if args.graph:
        logger.graph()
    if args.clean:
        if args.archive:
            print(f'{parser.prog}: cannot use --clean with --archive')
        else:
            print(f'{parser.prog}: --clean not yet implemented')

if __name__ == '__main__':
    main()