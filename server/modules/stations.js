const fs = require('fs');
const validator = require('email-validator');

const JSON_PATH = 'stations.json';
const stations = fs.existsSync(JSON_PATH) ? JSON.parse(fs.readFileSync(JSON_PATH)) : {};
const commit = () => {
	fs.writeFile(JSON_PATH, JSON.stringify(stations, null, 2), err => {
		if (err) global.log('ERROR: writhing '+JSON_PATH);
	});
};

const OPTIONS = [ 'failures', 'events' ];

function authorize(key) {
	return process.env.SECRET_KEY === key;
}

function get() {
	return stations;
}

function subscribe(station, email, options=[]) {
	const mailing = stations[station].mailing;
	for (const opt of OPTIONS) {
		if (options.includes(opt)) {
			if (!mailing[opt].includes(email))
				mailing[opt].push(email);
		} else {
			mailing[opt] = mailing[opt].filter(e => e !== email);
		}
	}
	commit();
}

module.exports = {
	validate: validator.validate,
	authorize,
	subscribe,
	get
};
