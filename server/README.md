## Server part of system

### Backend features
+ connect to sql database
+ accept data from section devices along with identifier
+ store data in one sql table but keeping device relation
+ serve data and devices list publicly

### Frontend features
TBD

### Detailed API description

##### `GET /api/data`

##### `POST /api/data`

refer to root README

##### `GET /api/sections`

### `.env` file example
```py
PORT=3051
DB_USER=
DB_HOST=localhost
DB_NAME=
DB_PASSWORD=
DB_PORT=5432
```

### Postgres tables description

```sql
CREATE ROLE nm LOGIN PASSWORD '';
CREATE DATABASE nm;

CREATE TABLE data (
	id SERIAL PRIMARY KEY,
	at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	section integer NOT NULL,
	dt TIMESTAMP NOT NULL,
	temperature_ext real,
	temperature real,
	pressure real,
	voltage real,
	c0 integer,
	c1 integer,
	c2 integer,
	c3 integer,
	c4 integer,
	c5 integer,
	c6 integer,
	c7 integer,
	c8 integer,
	c9 integer,
	c10 integer,
	c11 integer,
	uptime integer,
	info integer,
	UNIQUE (dt, section)
);

CREATE TABLE sections (
	id SERIAL PRIMARY KEY,
	key TEXT NOT NULL,
	channels INTEGER,
	description TEXT
);

INSERT INTO sections (key, channels, description)
VALUES ('anon', 12, 'An unregistered or deconfigured device');
```
