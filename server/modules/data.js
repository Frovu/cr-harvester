'use strict';
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

const TYPES = { // automatically parsed values
	voltage: 'real',
	temperature_ext: 'real',
	flash_failures: 'real',
	temperature: 'real',
	pressure: 'real',
	uptime: 'int',
	info: 'int'
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
time TIMESTAMP NOT NULL UNIQUE,
${dev.counters.map(c => c+' INTEGER,').join('\n')}
${dev.fields.map(c => c+' REAL,').join('\n')}
uptime INTEGER,
info INTEGER)`;
		const qc = `CREATE TABLE IF NOT EXISTS ${devId}_corrections (
id SERIAL PRIMARY KEY,
server_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
time TIMESTAMP NOT NULL UNIQUE,
${dev.counters.map(c => c+' INTEGER,').join('\n')}
${dev.fields.map(c => c+' REAL,').join('\n')}`;
		await pool.execute(qr);
		await pool.execute(qc);
	}
}
initTables();

function select(device, where, limit) {
	const dev = config.devices[device];
	const fields = (dev.counters || []).concat(dev.fields || []);
	const sel = fields.map(f => `COALESCE(${device}_corrections.${f}, ${device}_raw.${f}) as ${f}`).join(', ');
	const query = `SELECT r.server_time as server_time, r.time as time, uptime, info,
		${sel} FROM ${device}_raw r LEFT OUTER JOIN ${device}_corrections c USING time
		${where ? 'WHERE '+where : ''} ${limit ? 'LIMIT '+limit : ''}`;
	return pool.query(query);
}

async function insert(body) {
	const devId = body.k ?? body.key;
	const dt = body.dt ?? body.time;
	const dev = config.devices[devId];
	const time = new Date(dt?.toString().includes('T') ? dt : parseInt(dt) * 1000);
	if (dev == undefined)
		return 404;
	if (isNaN(time) || time == undefined || devId == undefined)
		return 400;
	if (dev.secret && (body.s ?? body.secret) !== dev.secret)
		return 403;
	const row = {};
	for (const name in TYPES) {
		let val = body[SHORT_NAMES[name]] ?? body[name];
		if (val == undefined || !dev.fields.includes(name))
			continue;
		if (TYPES[name] === 'int')
			val = parseInt(val);
		else if (TYPES[name] === 'real')
			val = parseFloat(val);
		if (isNaN(val))
			continue; // FIXME: log maybe?
		row[name] = val;
	}
	const query = `INSERT INTO ${devId}_raw (time,${Object.keys(row).join()})
VALUES (to_timestamp(${(time.getTime()/1000).toFixed()}),${Object.keys(row).map((v,i)=>'$'+(i+1)).join()})
ON CONFLICT (time) DO UPDATE SET (server_time,${Object.keys(row).join()}
= (CURRENT_TIMESTAMP,${Object.keys(row).map(c=>'EXCLUDED.'+c).join()})`;
	await pool.execute(query, Object.values(row));
	return 200;
}

module.exports = {
	select,
	insert
};
