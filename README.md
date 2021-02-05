# neutron-monitor
Embedded system for neutron monitor data collection.

## brief description

System consists of N counter devices of 1-12 channels connected to neutron monitor sections and a master web server which serves mostly as proxy to database.

Each device has internet (over ethernet) connection to server and is sending data at every UTC (minute) beginning.

Each device has onboard real time clock (battery backed) and keeps its time in sync via NTP.

Each device has external flash unit of (32 mbits) where data is stored in case of server/connection failure, when connection is restored data is resent to server.
