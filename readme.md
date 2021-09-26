# iotsaDataLogger - web server to record time series from a sensor

IotsaDataLogger reads an analog sensor repeatedly at a settable interval and records these readings in a buffer.
The buffer can then be read over the web as JSON data.

The intention is that the data logger is battery operated, reads values with a fairly long interval between readings, and uses deep sleep to conserve batteries. An RTC will keep the time during sleep (but the internal ESP32 ULP governs wakeup from deep sleep). Average power consumption is about 1 mA (depending on interval, of course) with the given schematic, so more aimed at big batteries than at penlites or coin cells.

When the device wakes from sleep it takes a measurement, stores it and goes to sleep again, _unless the configured wifi network is available_. If the wifi network is available the device remains awake and can be read out. It will also try and synchronize the RTC using NTP.

The use case this sensor was built for is monitoring a solar powered holiday home. The sensor is powered from the solar battery and takes readings of the battery voltage every hour. Once in a while I come by and start the WiFi network. After an hour I can get the readings of how the battery charged and discharged over the last couple of weeks.

Home page is <https://github.com/cwi-dis/iotsaSensor>.

## Software requirements

* PlatformIO or Arduino IDE.
* The iotsa framework, download from <https://github.com/cwi-dis/iotsa>.

## Hardware requirements

* an esp8266 board, such as an ESP-12, ESP-201 or iotsa board.
* An analog sensor.



## Building and Configuring

- Build using PlatformIO, flash to the board.
- Configure the board, see the [iotsa](https://github.com/cwi-dis/iotsa) instructions for general guidance:
	- WiFi network
	- hostname
	- timezone and NTP server
- Configure the acquisition module:
	- Interval between readings
	- Whether to use deep sleep
	- ADC multiplication factor and offset. The ESP32 ADC is not very linear, and there is also the voltage divider resistors to cater for. Start with `factor=1` and `offset=0`, supply minimum expected voltage and maximum expected voltage. From these readings determine an initial `factor` (from delta-voltage and delta-reading). Read min and max again, and determine initial `offset`. Repeat until happy.

## Operation


To read the values use the web interface, or the Python package in `extras/python/iotsaDataLogger`. It can read the recorded values to a CSV file, optionally merging, and graph the results.

There is an issue with the REST API with large datasets. You can supply the `--bufsize` parameter to the Python script (or a `jsonBufSize` URL query parameter to the REST URL) to enlarge the buffer size, but this buffer is on the ESP32 board, so things do have to fit in the ESP32 RAM.

After reading the values and saving them on the computer you can archive the readings (either all readings or up to a given timestamp), so from that point on the archived readings are no longer returned and don't contribute to the data size. The most recently archived set of readings is still available with `--archive` (until they are overwritten by the next archival).

There are some sample CSV files in `extras/sandbox`.