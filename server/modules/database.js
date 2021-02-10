const Pool = require('pg').Pool;
let pool;
function connect() {
	pool = new Pool({
		user: process.env.DB_USER,
		host: process.env.DB_HOST,
		database: process.env.DB_NAME,
		password: process.env.DB_PASSWORD,
		port: process.env.DB_PORT,
	});
}

let sections;
async function getSections() {
	const res = await pool.query('SELECT * from sections');
	sections = {};
	res.rows.forEach(r => sections[r.key] = r);
	return sections;
}

getSections().then(s => global.log(`Sections auth keys: ${Object.keys(s).join()}`));

module.exports = {
	connect: connect,
	getSections: getSections
};
