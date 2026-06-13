# iotsaDataLogger - web server to record time series from a sensor

![build-platformio](https://github.com/cwi-dis/iotsaDataLogger/workflows/build-platformio/badge.svg)
![build-arduino](https://github.com/cwi-dis/iotsaDataLogger/workflows/build-arduino/badge.svg)

IotsaDataLogger reads an analog sensor repeatedly at a configurable interval and records readings in a LittleFS-backed file on the device. Readings can be retrieved over the network as CSV data.

The device is intended to be battery-operated, taking readings at long intervals (minutes to hours) and using deep sleep between readings to conserve power. A DS1302 RTC module keeps wall-clock time across deep-sleep cycles and syncs from NTP whenever WiFi is available.

When the device wakes from sleep it takes a measurement and goes back to sleep, _unless the configured WiFi network is available_. When WiFi is available the device stays awake and can be read out.

The use case this was built for is monitoring a solar-powered holiday home: the device reads battery voltage once an hour, running on the solar battery itself. Whenever you visit and enable the WiFi network, you can retrieve weeks of readings.

Home page is <https://github.com/cwi-dis/iotsaDataLogger>.

## Hardware requirements

- An ESP32-based board (tested: `esp32thing`, `pico32`).
- An analog sensor connected to GPIO 34.
- A DS1302 RTC module (optional but recommended for accurate timestamps).

## Software requirements

- PlatformIO (recommended) or Arduino IDE.
- The [iotsa framework](https://github.com/cwi-dis/iotsa).

## Building and configuring

Build using PlatformIO and flash to the board. Then configure the device — see the [iotsa](https://github.com/cwi-dis/iotsa) instructions for general guidance:

- WiFi network and hostname
- Timezone and NTP server

Configure the acquisition module:

- Interval between readings
- Whether to use deep sleep
- ADC calibration: `adcMultiply` (scale factor) and `adcOffset` (offset). The ESP32 ADC is not very linear, and there is also the voltage divider to account for. Start with `factor=1` and `offset=0`, supply known voltages and iterate until readings match.

## Python tool

The `extras/python/` directory contains the `iotsaDataLogger` Python tool for retrieving, storing, merging, and graphing data.

### Setup

From the repo root:

```sh
python3 -m venv .venv
source .venv/bin/activate
pip install -e ../iotsa/extras/python/
pip install -e extras/python/
```

Or, if you have the `requirements_dev.txt`:

```sh
python3 -m venv .venv
source .venv/bin/activate
pip install -r extras/python/requirements_dev.txt
pip install -e extras/python/
```

### Usage

Retrieve current data from a device and print as CSV:

```sh
iotsaDataLogger -d yourdevice.local
```

Save to a file:

```sh
iotsaDataLogger -d yourdevice.local -o readings.csv
```

Merge new readings into an existing file (deduplicates and sorts by timestamp):

```sh
iotsaDataLogger -d yourdevice.local -o readings.csv -m
```

Graph the data:

```sh
iotsaDataLogger -d yourdevice.local -g
```

Read from a saved CSV file and graph it:

```sh
iotsaDataLogger -i readings.csv -g
```

Retrieve archived data instead of current data:

```sh
iotsaDataLogger -d yourdevice.local -a
```

### Full options

```
iotsaDataLogger [-h] [-d HOST] [-D [NAME=VALUE ...]] [-i [FILE ...]]
                [-g] [-v] [-o FILE] [-m] [-a] [--clean]

  -d HOST          Hostname or IP of the device
  -D NAME=VALUE    Extra iotsa connection arguments (e.g. bearer token)
  -i FILE          Read from CSV file instead of device (repeatable)
  -o FILE          Write output to CSV file (default: stdout)
  -m               Merge into output file instead of overwriting
  -a               Retrieve archived data instead of current data
  -g               Show a graph of the data
  -v               Verbose output
```

## Sample data

Some sample CSV files are in `extras/sandbox/`.
