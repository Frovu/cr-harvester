'use strict';
const config = require('../util/config').config;

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

const tableRaw = (id) => id.replace(' ', '_').replace('-', '_') + '_raw';
const tableCorr = (id) => id.replace(' ', '_').replace('-', '_') + '_corr';

async function initTables() {
	if (!config.devices || !Object.keys(config.devices).length)
		global.log('(!) Zero devices configured, open README for configuration guide.');
	await pool.query(`CREATE TABLE IF NOT EXISTS subscriptions (
		station TEXT,
		email TEXT,
		UNIQUE (station, email)
	)`);
	for (const devId in config.devices) {
		const dev = config.devices[devId];
		const qr = `CREATE TABLE IF NOT EXISTS ${tableRaw(devId)} (
id SERIAL PRIMARY KEY,
server_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
time TIMESTAMP NOT NULL UNIQUE,
${dev.counters.map(c => c+' INTEGER').join(',\n')},
${dev.fields.map(c => c+' REAL').join(',\n')},
uptime INTEGER,
info INTEGER)`;
		const qc = `CREATE TABLE IF NOT EXISTS ${tableCorr(devId)} (
id SERIAL PRIMARY KEY,
server_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
time TIMESTAMP NOT NULL UNIQUE,
${dev.counters.map(c => c+' INTEGER').join(',\n')},
${dev.fields.map(c => c+' REAL').join(',\n')})`;
		await pool.query(qr);
		await pool.query(qc);
	}
}
initTables();

async function selectEmails(stations) {
	const query = `SELECT DISTINCT email FROM subscriptions WHERE station IN (${stations.map((_,i)=>'$'+(i+1)).join()})`;
	const res = await query(query, stations);
	return res.rows.flat();
}

async function select(device, where, limit) {
	const dev = config.devices[device];
	const fields = (dev.counters || []).concat(dev.fields || []);
	const sel = fields.map(f => `COALESCE(c.${f}, r.${f}) as ${f}`).join(', ');
	const query = `SELECT * FROM (SELECT r.server_time as server_time, r.time as time, uptime, info,
		${sel} FROM ${tableRaw(device)} r LEFT OUTER JOIN ${tableCorr(device)} c ON c.time = r.time
		${where ? 'WHERE '+where : ''} ORDER BY time DESC ${limit ? 'LIMIT '+limit : ''}) rev ORDER BY time`;
	return await pool.query(query);
}

async function selectAll(limit) {
	if (!limit || isNaN(limit))
		limit = 60;
	const result = {};
	for (const dev in config.devices) {
		const res = await select(dev, null, limit);
		result[dev] = { rows: res.rows, fields: res.fields.map(f => f.name) };
	}
	return result;
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
	const query = `INSERT INTO ${tableRaw(devId)} (time,${Object.keys(row).join()})
VALUES (to_timestamp(${(time.getTime()/1000).toFixed()}),${Object.keys(row).map((_,i)=>'$'+(i+1)).join()})
ON CONFLICT (time) DO UPDATE SET (server_time,${Object.keys(row).join()}
= (CURRENT_TIMESTAMP,${Object.keys(row).map(c=>'EXCLUDED.'+c).join()})`;
	await pool.query(query, Object.values(row));
	return 200;
}

module.exports = {
	pool,
	selectEmails,
	selectAll,
	select,
	insert
};
