import csv
import io
import requests
import iotsa
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import pandas as pd

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

    def _fetch_sunlight(self, location, start_date, end_date):
        geo_url = (
            "https://geocoding-api.open-meteo.com/v1/search"
            f"?name={requests.utils.quote(location)}&count=1&language=en&format=json"
        )
        geo_resp = requests.get(geo_url, timeout=10)
        geo_resp.raise_for_status()
        geo_data = geo_resp.json()
        if not geo_data.get('results'):
            print(f"Could not geocode location: {location}")
            return None, location
        result = geo_data['results'][0]
        lat, lon = result['latitude'], result['longitude']
        place_name = result.get('name', location)
        if self.verbose:
            print(f"Geocoded '{location}' to {place_name} ({lat}, {lon})")

        wx_url = (
            "https://archive-api.open-meteo.com/v1/archive"
            f"?latitude={lat}&longitude={lon}"
            f"&start_date={start_date}&end_date={end_date}"
            "&daily=sunshine_duration,shortwave_radiation_sum"
            "&timezone=auto"
        )
        wx_resp = requests.get(wx_url, timeout=15)
        wx_resp.raise_for_status()
        wx_data = wx_resp.json()

        wx = pd.DataFrame({
            'date': pd.to_datetime(wx_data['daily']['time']),
            'sunshine_h': [s / 3600 for s in wx_data['daily']['sunshine_duration']],
            'radiation': wx_data['daily']['shortwave_radiation_sum'],
        })
        return wx, place_name

    def graph(self, sunlight_location=None):
        df = pd.DataFrame(self.data)

        if self._raw:
            df['v'] = pd.to_numeric(df['v'])
            df['t'] = pd.to_datetime(df['t'])
            date_col = df['t']
        else:
            df['min_v'] = pd.to_numeric(df['min_v'])
            df['max_v'] = pd.to_numeric(df['max_v'])
            df['date'] = pd.to_datetime(df['date'])
            date_col = df['date']

        wx, place_name = None, None
        if sunlight_location:
            start = date_col.min().strftime('%Y-%m-%d')
            end = date_col.max().strftime('%Y-%m-%d')
            wx, place_name = self._fetch_sunlight(sunlight_location, start, end)

        if wx is not None:
            fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 7), sharex=True)
        else:
            fig, ax1 = plt.subplots(1, 1, figsize=(14, 4))

        title = self.device.ipAddress if self.device else ''

        if self._raw:
            ax1.plot(df['t'], df['v'], linewidth=0.8, color='steelblue')
            ax1.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d'))
            ax1.xaxis.set_major_locator(mdates.DayLocator())
            ax1.set_title(f'{title} — raw voltage' if title else 'Raw voltage')
        else:
            ax1.fill_between(df['date'], df['min_v'], df['max_v'], alpha=0.35, color='steelblue')
            ax1.plot(df['date'], df['min_v'], linewidth=0.8, color='steelblue')
            ax1.plot(df['date'], df['max_v'], linewidth=0.8, color='steelblue')
            ax1.xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m'))
            ax1.xaxis.set_major_locator(mdates.MonthLocator())
            ax1.set_title(f'{title} — daily min/max voltage' if title else 'Daily min/max voltage')

        ax1.set_ylabel('Voltage (V)')
        ax1.grid(True, alpha=0.3)

        if wx is not None:
            ax2.bar(wx['date'], wx['sunshine_h'], color='gold', alpha=0.8, label='Sunshine (h/day)', width=1)
            ax2b = ax2.twinx()
            ax2b.plot(wx['date'], wx['radiation'], color='orange', linewidth=0.8, alpha=0.7, label='Radiation (MJ/m²)')
            ax2.set_ylabel('Sunshine hours/day')
            ax2b.set_ylabel('Solar radiation (MJ/m²)')
            ax2.set_ylim(0, 16)
            ax2.grid(True, alpha=0.3)
            ax2.set_title(f'Sunshine — {place_name}')
            lines1, labels1 = ax2.get_legend_handles_labels()
            lines2, labels2 = ax2b.get_legend_handles_labels()
            ax2.legend(lines1 + lines2, labels1 + labels2, loc='upper left', fontsize=8)
            plt.setp(ax2.get_xticklabels(), rotation=45, ha='right')
        else:
            plt.setp(ax1.get_xticklabels(), rotation=45, ha='right')

        plt.tight_layout()
        plt.show()
