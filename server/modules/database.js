const Pool = require('pg').Pool;
let pool, sections = {};

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

const TO_URLENCODED = { temperature: 't', pressure: 'p' };

async function getSections() {
	const res = await pool.query('SELECT * from sections');
	sections = {};
	res.rows.forEach(r => sections[r.key] = r);
	return sections;
}

function validate(data) {
	return Object.keys(sections).includes(data.k)
		&& typeof data.c === 'object' && data.dt;
}

// presumes to be called after validate()
async function insert(data) {
	const row = {};
	for(const f in TO_URLENCODED) row[f] = data[TO_URLENCODED[f]];
	for(const i in data.c) row['c'+i] = data.c[i];
	row.section = sections[data.k];
	const placeholders = Object.keys(Object.keys(row)).map(i=>'$'+i).join();
	const q = `INSERT INTO data (${Object.keys(row).join()}) VALUES (${placeholders})`;
	await pool.query(q, Object.values(row));
}

module.exports = {
	pool: pool,
	insert: insert,
	connect: connect,
	validate: validate,
	getSections: getSections
};
