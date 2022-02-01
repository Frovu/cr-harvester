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

SMTP_SERVER=mail.izmiran.ru
SMTP_USER=
SMTP_PASS=

ADMIN_SECERT=1234
```

### Postgres tables description

```sql
CREATE ROLE nm LOGIN PASSWORD '';
CREATE DATABASE nm;

CREATE TABLE devices (
	id SERIAL PRIMARY KEY,
	key TEXT NOT NULL,
	channels INTEGER,
	description TEXT,
	type TEXT NOT NULL DEFAULT 'nm'
);

INSERT INTO devices (key, channels, description)
VALUES ('anon', 12, 'An unregistered or deconfigured device');
```

### stations.json content example

```json
{
	"Moscow Neutron Monitor": {
		"description": "Neutron monitor located at IZMIRAN, has 5 sections, 6 channels in each. <link>",
		"devices": [""],
		"mailing": {
			"events": ["sfrovis@gmail.com"],
			"issues": ["sfrovis@gmail.com"]
		}
	},
	"Muon Pioneer": {
		"description": "Single direction muon telescope. <link>",
		"devices": ["muon-anon"],
		"mailing": {
			"events": ["sfrovis@gmail.com"],
			"issues": ["sfrovis@gmail.com"]
		}
	}
}
```
