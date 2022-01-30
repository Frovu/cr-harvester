
CREATE TABLE IF NOT EXISTS nm_data (
	id SERIAL PRIMARY KEY,
	device_id integer NOT NULL,
	at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	dt TIMESTAMP NOT NULL,
	temperature real,
	pressure real,
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
	UNIQUE (dt, device_id)
);

CREATE TABLE IF NOT EXISTS muon_data (
	id SERIAL PRIMARY KEY,
	device_id integer NOT NULL,
	at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	dt TIMESTAMP NOT NULL,
	temperature real,
	temperature_ext real,
	pressure real,
	voltage real,
	c0 integer,
	c1 integer,
	n_v integer,
	uptime integer,
	info integer,
	UNIQUE (dt, device_id)
);

CREATE TABLE IF NOT EXISTS devices (
	id SERIAL PRIMARY KEY,
	key TEXT NOT NULL,
	channels INTEGER,
	description TEXT,
	type TEXT NOT NULL DEFAULT 'nm'
);
