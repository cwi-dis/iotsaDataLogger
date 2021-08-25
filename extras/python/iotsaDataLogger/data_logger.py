import csv
import iotsa
import matplotlib.pyplot as pyplot
import pandas

class DataLogger:
    def __init__(self, verbose=False):
        self.data = []
        self.verbose = verbose
        self.device = None

    def read_device(self, device, archive, kwargs):
        self.device = iotsa.IotsaDevice(device, **kwargs)
        api = self.device.getApi('datalogger?jsonBufSize=20000' if not archive else 'datalogger?jsonBufSize=20000&archive=1')
        api_data = api.getAll()
        data = api_data['data']
        if self.verbose:
            print(f"read_device: got {len(data)} records, time={api_data['now']}")
            print(f'read_file: first={data[0]}, last={data[-1]}')
        self.data = self.data + data
        self._sort()

    def read_file(self, filename):
        data = []
        with open(filename, 'r') as fp:
            csv_reader = csv.DictReader(fp, quoting=csv.QUOTE_NONNUMERIC)
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
        with open(filename, 'w') as fp:
            csv_writer = csv.DictWriter(fp, self.data[0].keys(), quoting=csv.QUOTE_NONNUMERIC)
            csv_writer.writeheader()
            csv_writer.writerows(self.data)
        if self.verbose:
            print(f'write_file: wrote {len(self.data)} records to {filename}')

    def graph(self):
        pd = pandas.DataFrame(self.data)
        pd.v = pandas.to_numeric(pd.v)
        pd.ts = pandas.to_numeric(pd.ts)
        pd.t = pandas.to_datetime(pd.t)
        pd.plot(x='t', y='v')
        pyplot.show()
