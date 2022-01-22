const fs = require('fs');
const Pool = require('pg').Pool;
let pool = {}, devices = {};

function connect() {
	module.exports.pool = pool = new Pool({
		user: process.env.DB_USER,
		host: process.env.DB_HOST,
		database: process.env.DB_NAME,
		password: process.env.DB_PASSWORD,
		port: process.env.DB_PORT,
	});
	prepareTables().then(() => {
		getDevices().then(s => global.log(`DB connected, auth keys: ${Object.keys(s).join()}`));
	});
}

// convert between db column name and protocol short token, see project root README
const DB_TO_PROTOCOL = {
	voltage: 'v',
	temperature_ext: 'te',
	temperature: 't',
	pressure: 'p',
	uptime: 'upt',
	info: 'inf'
};

const MUON_CHANNELS = [ 'c0', 'c1', 'n_v' ];

const COLUMN_TYPE = {
	voltage: 'float',
	temperature_ext: 'float',
	temperature: 'float',
	pressure: 'float',
	uptime: 'int',
	info: 'int',
};

async function prepareTables() {
	const q = fs.readFileSync('tables.sql', 'utf8');
	await pool.query(q);
}

async function getDevices() {
	const res = await pool.query('SELECT * from devices');
	devices = {};
	res.rows.forEach(r => devices[r.key] = r);
	return devices;
}

function validate(data) { // TODO: validate better
	return data.k && Object.keys(devices).includes(data.k)
		&& typeof data.c === 'object' && data.dt;
}

// presumes to be called after validate()
async function insert(data) {
	const row = {};
	for (const f in DB_TO_PROTOCOL) {
		let val = data[DB_TO_PROTOCOL[f]];
		if(typeof val === 'undefined')
			continue;
		else if(COLUMN_TYPE[f] === 'float')
			val = parseFloat(val);
		else if(COLUMN_TYPE[f] === 'int')
			val = parseInt(val);
		row[f] = val;
	}
	const type = devices[data.k].type;
	if (type === 'muon') {
		for (const i in MUON_CHANNELS) {
			const val = parseInt(data.c[i]);
			if (val) row[MUON_CHANNELS[i]] = val;
		}
	} else if (type === 'nm') {
		for (let i=0; i < devices[data.k].channels; ++i) {
			const val = parseInt(data.c[i]);
			if (val) row['c'+i] = val;
		}
	} else {
		throw new Error(`Unknown device type: ${type}`) ;
	}
	row.device_id = devices[data.k].id;
	row.dt = new Date(data.dt.toString().includes('T') ? data.dt : parseInt(data.dt * 1000));
	for (const i in row)
		if (typeof row[i] === 'undefined')
			delete row[i];
	const columns = Object.keys(row);
	const placeholders = [...columns.keys()].map(i=>'$'+(i+1)).join();
	const q = `INSERT INTO ${type}_data (${Object.keys(row).join()}) VALUES (${placeholders}) ` +
		`ON CONFLICT (dt, device_id) DO UPDATE SET (${columns.join()}) = (${columns.map(c => 'EXCLUDED.'+c).join()})`;
	await pool.query(q, Object.values(row));
}

module.exports = {
	pool,
	insert,
	connect,
	validate,
	getDevices
};
