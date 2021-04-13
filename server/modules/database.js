const Pool = require('pg').Pool;
let pool = {}, sections = {};

function connect() {
	module.exports.pool = pool = new Pool({
		user: process.env.DB_USER,
		host: process.env.DB_HOST,
		database: process.env.DB_NAME,
		password: process.env.DB_PASSWORD,
		port: process.env.DB_PORT,
	});
	getSections().then(s => global.log(`DB connected, auth keys: ${Object.keys(s).join()}`));
}

// convert between db column name and protocol short token, see project root README
const DB_TO_PROTOCOL = {
	temperature: 't',
	pressure: 'p',
	uptime: 'upt',
	info: 'inf'
};

const COLUMN_TYPE = {
	temperature: 'float',
	pressure: 'float',
	uptime: 'int',
	info: 'int',
};

async function getSections() {
	const res = await pool.query('SELECT * from sections');
	sections = {};
	res.rows.forEach(r => sections[r.key] = r);
	return sections;
}

function validate(data) {
	return data.k && Object.keys(sections).includes(data.k)
		&& typeof data.c === 'object' && data.dt;
}

// presumes to be called after validate()
async function insert(data) {
	const row = {};
	for(const f in DB_TO_PROTOCOL) {
		let val = data[DB_TO_PROTOCOL[f]];
		if(typeof val === 'undefined')
			continue;
		else if(COLUMN_TYPE[f] === 'float')
			val = parseFloat(val);
		else if(COLUMN_TYPE[f] === 'int')
			val = parseInt(val);
		row[f] = val;
	}
	for(const i in data.c) row['c'+i] = parseInt(data.c[i]);
	row.section = sections[data.k].id;
	row.dt = new Date(data.dt.toString().includes('T') ? data.dt : parseInt(data.dt * 1000));
	for(const i in row)
		if(typeof row[i] === 'undefined')
			delete row[i];
	const columns = Object.keys(row);
	const placeholders = [...columns.keys()].map(i=>'$'+(i+1)).join();
	const q = `INSERT INTO data (${Object.keys(row).join()}) VALUES (${placeholders}) ` +
		`ON CONFLICT (dt, section) DO UPDATE SET (${columns.join()}) = (${columns.map(c => 'EXCLUDED.'+c).join()})`;
	await pool.query(q, Object.values(row));
}

module.exports = {
	pool: pool,
	insert: insert,
	connect: connect,
	validate: validate,
	getSections: getSections
};
