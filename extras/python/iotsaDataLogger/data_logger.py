import csv
import io
import requests
import iotsa
import matplotlib.pyplot as pyplot
import pandas

class DataLogger:
    def __init__(self, verbose=False):
        self.data = []
        self.verbose = verbose
        self.device = None

    def read_device(self, device, archive, kwargs={}):
        self.device = iotsa.IotsaDevice(device, **kwargs)
        ph = self.device.protocolHandler
        filename = 'archive.csv' if archive else 'data.csv'
        url = ph.baseURL + f'datalogger/{filename}'
        headers = {}
        if ph.bearer:
            headers['Authorization'] = 'Bearer ' + ph.bearer
        if self.verbose:
            print(f"read_device: fetching {url}")
        response = requests.get(url, auth=ph.auth, verify=not ph.noverify, headers=headers)
        response.raise_for_status()
        reader = csv.DictReader(io.StringIO(response.text))
        data = list(reader)
        if self.verbose:
            print(f"read_device: got {len(data)} records")
            if data:
                print(f'read_device: first={data[0]}, last={data[-1]}')
        self.data = self.data + data
        self._sort()

    def read_file(self, filename):
        data = []
        with open(filename, 'r') as fp:
            csv_reader = csv.DictReader(fp)
            for record in csv_reader:
                data.append(record)
        if self.verbose:
            print(f'read_file: read {len(data)} records from {filename}')
            print(f'read_file: first={data[0]}, last={data[-1]}')
        self.data = self.data + data
        self._sort()

    def _sort(self):
        data = sorted(self.data, key=lambda r: r['t'])
        self.data = []
        prevd = None
        for d in data:
            if d == prevd: continue
            self.data.append(d)
            prevd = d
        if self.verbose:
            print(f'_sort: {len(self.data)} records total.')
            print(f'_sort: first={data[0]}, last={data[-1]}')

    def write_file(self, filename):
        import sys
        fp = sys.stdout if filename == '-' else open(filename, 'w')
        try:
            csv_writer = csv.DictWriter(fp, self.data[0].keys(), quoting=csv.QUOTE_NONNUMERIC)
            csv_writer.writeheader()
            csv_writer.writerows(self.data)
        finally:
            if fp is not sys.stdout:
                fp.close()
        if self.verbose:
            print(f'write_file: wrote {len(self.data)} records to {filename}')

    def graph(self):
        pd = pandas.DataFrame(self.data)
        pd.v = pandas.to_numeric(pd.v)
        pd.ts = pandas.to_numeric(pd.ts)
        pd.t = pandas.to_datetime(pd.t)
        pd.plot(x='t', y='v')
        pyplot.show()
