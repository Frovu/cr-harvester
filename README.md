# neutron-monitor
Embedded system for neutron monitor data collection.

## brief description

System consists of N counter devices of 1-12 channels connected to neutron monitor sections and a master web server which serves mostly as proxy to database.

Each device has internet (over ethernet) connection to server and is sending data at every UTC (minute) beginning.

Each device has onboard real time clock (battery backed) and keeps its time in sync via NTP.

Each device has external flash unit of (32 mbits) where data is stored in case of server/connection failure, when connection is restored data is resent to server.

# Protocol description

To save collected data into database, device interacts with server part of the system.
Device sends HTTP POST request with `urlencoded` or `json` body. Which includes following values:
+ `k` - `device key/id` - unique device identifier to distinguish several sections
+ `dt` - `date/timestamp` - timestamp of counting period beginning
+ `upt` - `uptime` - device uptime at the moment of counting period start (in minutes)
+ `inf` - `info` - different debugging info, if LSB set (number is odd), dev time is trusted, other bits are reserved
+ `ff` - `flash failures` - count of device external flash program/erase failures
+ `t` - `temperature` - air temperature in Celsius with 2 decimal signs after point
+ `p` - `pressure` - air temperature in hPa with 2 decimal signs after point
+ `c` - `counts` - array of 12 integer containing channels impulse counts

note: time (`dt`) is interpreted either as epoch (count of seconds since 1970) or as `ISO 8601` string if it includes `T` character.

# Device description

## device LED's behavior

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

## device LED's most common patterns
1. **green and blue** leds are **lit** - whole flash memory is being read
2. **red and green** leds are **lit** - device failed to initialize RTC peripheral on start
3. **red** led **blinks** with period of ~500ms - something is wrong with device peripherals
4. **blue** led is **lit** and **red** led **blinks shortly** -
device fails to send data to server
