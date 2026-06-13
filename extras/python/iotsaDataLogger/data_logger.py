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
        self._raw = False

    def read_device(self, device, raw=False, kwargs={}):
        self._raw = raw
        self.device = iotsa.IotsaDevice(device, **kwargs)
        ph = self.device.protocolHandler
        filename = 'data.csv' if raw else 'data_daily.csv'
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
            if csv_reader.fieldnames and csv_reader.fieldnames[0] == 'date':
                self._raw = False
            else:
                self._raw = True
            for record in csv_reader:
                data.append(record)
        if self.verbose:
            print(f'read_file: read {len(data)} records from {filename}')
            if data:
                print(f'read_file: first={data[0]}, last={data[-1]}')
        self.data = self.data + data
        self._sort()

    def _sort(self):
        sort_key = 't' if self._raw else 'date'
        data = sorted(self.data, key=lambda r: r[sort_key])
        self.data = []
        prevd = None
        for d in data:
            if d == prevd: continue
            self.data.append(d)
            prevd = d
        if self.verbose:
            print(f'_sort: {len(self.data)} records total.')
            if data:
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
        if self._raw:
            pd['v'] = pandas.to_numeric(pd['v'])
            pd['t'] = pandas.to_datetime(pd['t'])
            pd.plot(x='t', y='v')
        else:
            pd['min_v'] = pandas.to_numeric(pd['min_v'])
            pd['max_v'] = pandas.to_numeric(pd['max_v'])
            pd['date'] = pandas.to_datetime(pd['date'])
            pd.plot(x='date', y=['min_v', 'max_v'])
        pyplot.show()
