const config = require('./config').config;

const pg = require('pg');
pg.types.setTypeParser(1114, function(stringValue) {
	return new Date(stringValue + '+0000'); // interpret pg 'timestamp without time zone' as utc
});

const SHORT_NAMES = {
	key: 'k',
	time: 'dt',
	counts: 'c',
	voltage: 'v',
	temperature_ext: 'te',
	flash_failures: 'ff',
	temperature: 't',
	pressure: 'p',
	uptime: 'upt',
	info: 'inf'
};

const pool = module.exports.pool = new pg.Pool({
	user: process.env.DB_USER,
	host: process.env.DB_HOST,
	database: process.env.DB_NAME,
	password: process.env.DB_PASSWORD,
	port: process.env.DB_PORT,
});

async function initTables() {
	await pool.execute(`CREATE TABLE IF NOT EXISTS subscriptions (
		station TEXT,
		email TEXT,
		UNIQUE (startion, email)
	)`);
	for (const devId in config.devices) {
		const dev = config.devices[devId];
		const qr = `CREATE TABLE IF NOT EXISTS ${devId}_raw (
id SERIAL PRIMARY KEY,
server_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
time TIMESTAMP NOT NULL,
${dev.counters.map(c => c+' INTEGER,').join('\n')}
${dev.fields.map(c => c+' REAL,').join('\n')}
uptime INTEGER,
info INTEGER)`;
		const qc = `CREATE TABLE IF NOT EXISTS ${devId}_corrections (
id SERIAL PRIMARY KEY,
server_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
${dev.counters.map(c => c+' INTEGER,').join('\n')}
${dev.fields.map(c => c+' REAL,').join('\n')}`;
		await pool.execute(qr);
		await pool.execute(qc);
	}
}
initTables();

async function select(device, where, limit) {
	const dev = config.devices[device];
	const fields = (dev.counters || []).concat(dev.fields || []);
	const sel = fields.map(f => `COALESCE(${device}_corrections.${f}, ${device}_raw.${f})`).join(', ');
	const query = `SELECT server_time, time, uptime, info, ${sel} FROM
		${device}_raw r LEFT OUTER JOIN ${device}_corrections c ON r.time = c.time
		${where ? 'WHERE '+where : ''} ${limit ? 'LIMIT '+limit : ''}`;
	return pool.query(query);
}
