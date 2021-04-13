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
	for(const f in DB_TO_PROTOCOL) row[f] = data[DB_TO_PROTOCOL[f]];
	for(const i in data.c) row['c'+i] = parseInt(data.c[i]);
	row.section = sections[data.k];
	row.dt = new Date(data.dt.toString().includes('T') ? data.dt : parseInt(data.dt * 1000));
	const columns = Object.keys(row);
	const placeholders = Object.keys(columns).map(i=>'$'+i).join();
	const q = `INSERT INTO data (${Object.keys(row).join()}) VALUES (${placeholders}) ` +
		`ON CONFLICT (dt, section) DO UPDATE SET (${columns.join()}) = (${columns.map(c => 'EXCLUDED.'+c).join()})`;
	console.log(q)
	await pool.query(q, Object.values(row));
}

module.exports = {
	pool: pool,
	insert: insert,
	connect: connect,
	validate: validate,
	getSections: getSections
};
