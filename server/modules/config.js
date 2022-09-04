const fs = require('fs');

const FILE = 'config.json';
const DEFAULT = {};
let config = fs.existsSync(FILE) ? JSON.parse(fs.readFileSync(FILE)) : DEFAULT;

const write = () => fs.writeFileSync(FILE, JSON.stringify(config, null, 2));

module.exports = {
	config,
	write
};
