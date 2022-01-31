const fs = require('fs');

const JSON_PATH = 'stations.json';

const stations = fs.existsSync(JSON_PATH) ? JSON.parse(fs.readFileSync(JSON_PATH)) : {};

function get() {
	return stations;
}

module.exports = {
	get
};
