# cr-harvester
Scalable embedded system for cosmic rays data collection.

## System features

+ Unlimited number of devices
+ Web page with all devices statuses
+ Email alerts when a device stops to send data
+ Supported device types: nm, muon

## Data API

When data is collected, the device sends HTTP POST request with `urlencoded` or `json` body. Which includes following values:
+ `k` - `key` - unique device identifier
+ `s` - `secret` - device secret key (only if configured)
+ `dt` - `date/timestamp` - timestamp of counting period beginning
+ `upt` - `uptime` - device uptime at the moment of counting period start (in minutes)
+ `inf` - `info` - different debugging info, if LSB set (number is odd), dev time is trusted, other bits are reserved
+ `ff` - `flash failures` - count of device external flash program/erase failures
+ `t` - `temperature` - air temperature in Celsius .2f near the device core (bmp280)
+ `te` - `temperature_ext` - air temperature in Celsius .2f temperature measured by ds18b20 (optional)
+ `p` - `pressure` - air pressure in mb .2f
+ `v` - `voltage` - device supply voltage (optional)
+ `c` - `counts` - array of N integers

note: time (`dt`) is interpreted either as epoch (count of seconds since 1970) or as `ISO 8601` string if it includes `T` character.

# Device description

## Features

+ Timekeeping functionality: internal or external RTC
+ Internet connection: Ethernet, WiFi or GSM
+ Time syncing functionality: NTP or GPS
+ Onboard BMP280 pressure sensor
+ (optional) DS18B20 External temperature sensor
+ (optional) External flash memory for data persistence in case of transmission failures
+ (optional) ADC for device supply voltage measurements
+ (optional) Device configuration HTTP server

## LED's behavior

Device has 3 informative LED's:
- green led on stm32 board
- blue led (referred to as DATA_LED)
- red led (referred to as ERROR_LED)

Green led behaves as follows:
- it's **lit** when device is being initialized
- it **blinks** shortly at the moment of data registration period transition

DATA_LED (blue) led behaves as follows:
- it's **lit** when device data storage is being read (it happens on flash initialization - green led is on)
- it's **lit** when device have any data stored
- it **blinks** (inverted) at the moment of data sending try

ERROR_LED (red)  led behaves as follows:
- it's **lit** when device peripherals initialization happens
- it **blinks** shortly after data sending try is failed
- it **blinks** with period of ~500ms when some of device peripherals is lost or fails to answer

## LED's most common patterns
1. **green and blue** leds are **lit** - whole flash memory is being read
2. **red and green** leds are **lit** - device failed to initialize RTC peripheral on start
3. **red** led **blinks** with period of ~500ms - something is wrong with device peripherals
4. **blue** led is **lit** and **red** led **blinks shortly** -
device fails to send data to server
