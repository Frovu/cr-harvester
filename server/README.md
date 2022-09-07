
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

### config.json content example

Note that device id must use underscores for spaces

```json
{
	"devices": {
		"device_id": {
			"description": "text",
			"counters": ["c1", "c2", "c3", "c4"],
			"fields": ["temperature"],
			"secret": "1234"
		}
	},
	"stations": {
		"station_id": {
			"name": "",
			"description": "text",
			"devices": ["device_id"]
		}
	}
}
```
