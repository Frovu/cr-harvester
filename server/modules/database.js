const Pool = require('pg').Pool;
let pool, sections;

function connect() {
	pool = new Pool({
		user: process.env.DB_USER,
		host: process.env.DB_HOST,
		database: process.env.DB_NAME,
		password: process.env.DB_PASSWORD,
		port: process.env.DB_PORT,
	});
	getSections().then(s => global.log(`DB connected, auth keys: ${Object.keys(s).join()}`));
}

async function getSections() {
	const res = await pool.query('SELECT * from sections');
	sections = {};
	res.rows.forEach(r => sections[r.key] = r);
	return sections;
}

module.exports = {
	connect: connect,
	getSections: getSections
};
