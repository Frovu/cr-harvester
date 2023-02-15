'use strict';
const config = require('../util/config').config;

const pg = require('pg');
pg.types.setTypeParser(1114, function(stringValue) {
	return new Date(stringValue + '+0000'); // interpret pg 'timestamp without time zone' as utc
});

const STUB_VALUE = -9999;

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

const tableRaw = (id) => id.replace(/\s|-/g, '_') + '_raw';
const tableCorr = (id) => id.replace(/\s|-/g, '_') + '_corr';

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
	const text = `SELECT DISTINCT email FROM subscriptions WHERE station IN (${stations.map((_,i)=>'$'+(i+1)).join()})`;
	const res = await pool.query({ text, values: stations, rowMode: 'array' });
	return res.rows.flat();
}

async function subscribe(station, email) {
	const text = 'INSERT INTO subscriptions(station, email) VALUES ($1, $2) ON CONFLICT (station, email) DO NOTHING';
	await pool.query(text, [station, email]);
}

async function unsubscribe(station, email) {
	const text = 'DELETE FROM subscriptions WHERE station=$1 AND email=$2';
	await pool.query(text, [station, email]);
}

async function select({device, fields: what=null, limit=null, serverTime=false, from, to}) {
	const dev = config.devices[device];
	const fields = (dev.counters || []).concat(dev.fields || []).filter(f => !what || what.includes(f));
	const sel = fields.map(f =>
		`CASE WHEN c.${f} IS NULL THEN r.${f} WHEN c.${f} = ${STUB_VALUE} ` +
		`THEN NULL ELSE c.${f} END as ${f}`).join(', ') + (what ? '' : ', uptime, info');
	const where = (from && to) ? `r.time >= to_timestamp(${from}) AND r.time <= to_timestamp(${to})` : null;
	const corrected = `SELECT ${serverTime ? 'EXTRACT(EPOCH FROM r.server_time)::integer as server_time, ':''}EXTRACT(EPOCH FROM r.time)::integer as time,
		${sel} FROM ${tableRaw(device)} r LEFT OUTER JOIN ${tableCorr(device)} c ON c.time = r.time
		${where ? 'WHERE '+where : ''} ${limit ? 'ORDER BY time DESC LIMIT '+limit : ''}`;
	return await pool.query({ text: `SELECT * FROM (${corrected}) corr ORDER BY time`, rowMode: 'array' });
}

async function deleteCorrections(device, from, to) {
	const text = `DELETE FROM ${tableCorr(device)} WHERE
		time >= to_timestamp(${(from/1000).toFixed()}) AND time <= to_timestamp(${(to/1000).toFixed()})`;
	return await pool.query(text);
}

async function insertCorrections(device, corrs, rawFields) {
	const dev = config.devices[device];
	const fields = rawFields.filter(f => dev.counters.includes(f) || dev.fields.includes(f));
	const values = corrs.map(row =>
		`(to_timestamp(${parseInt(row[0])}),${
			row[1].map(v => v == null ? STUB_VALUE : parseInt(v)).join()})`).join(',\n');

	const text = `INSERT INTO ${tableCorr(device)} (time,${fields.join()}) VALUES\n${values}
	ON CONFLICT (time) DO UPDATE SET (${fields.join()}) = (${fields.map(f=>'EXCLUDED.'+f).join()})`;
	return await pool.query(text);
}

async function selectAll(limit) {
	if (!limit || isNaN(limit))
		limit = 60;
	const result = {};
	for (const dev in config.devices) {
		const res = await select({ device: dev, limit, serverTime: true});
		result[dev] = { rows: res.rows, fields: res.fields.map(f => f.name) };
	}
	return result;
}

async function selectInterval(device, dfrom, dto, period, fields) {
	const from = Math.floor(dfrom/period) * period;
	const to = Math.ceil(dto/period) * period;
	const res = await select({ device, fields, from, to });
	const rows = period === 60 ? res.rows : Array(Math.ceil((to-from)/period)).fill().map(() => Array(res.fields.length).fill());
	if (period !== 60) {
		const data = res.rows, flen = res.fields.length;
		for (let columnIdx = 1; columnIdx < flen; ++columnIdx) {
			let cursor = 0;
			for (let rowIdx=0; rowIdx < rows.length; ++rowIdx) {
				let accum = 0, count = 0;
				const time = from + period * rowIdx;
				rows[rowIdx][0] = time;
				while(cursor < data.length && data[cursor][0] < time + period) {
					if (data[cursor][columnIdx] != null) {
						accum += data[cursor][columnIdx];
						++count;
					}
					++cursor;
				}
				console.log(columnIdx, cursor, count)
				rows[rowIdx][columnIdx] = count > 0 ? Math.round(accum / count * 1000) / 1000 : null;
			}

		}
	}
	return { rows, fields: res.fields.map(f => f.name) };
}

async function insert(body) {
	const devId = body.k ?? body.key;
	const dt = body.dt ?? body.time;
	const dev = config.devices[devId];
	const counts = body.counts ?? body[SHORT_NAMES.counts];
	const time = new Date(dt?.toString().includes('T') ? dt : parseInt(dt) * 1000);
	if (dev == undefined)
		return 404;
	if (!counts || isNaN(time) || time == undefined || devId == undefined)
		return 400;
	if (dev.secret && (body.s ?? body.secret) !== dev.secret)
		return 403;
	const row = {};
	for (const [i, name] of dev.counters.entries()) {
		const val = parseInt(counts[i]);
		if (!isNaN(val))
			row[name] = val;
	}
	for (const name of dev.fields.concat(['uptime', 'info'])) {
		let val = body[SHORT_NAMES[name]] ?? body[name];
		if (val == undefined)
			continue;
		if (TYPES[name] === 'int')
			val = parseInt(val);
		else if (TYPES[name] === 'real')
			val = parseFloat(val);
		if (isNaN(val))
			continue; // FIXME: log maybe?
		row[name] = val;
	}
	const fields = Object.keys(row);
	const query = `INSERT INTO ${tableRaw(devId)} (time,${fields.join()})
VALUES (to_timestamp(${(time/1000).toFixed()}),${fields.map((_,i)=>'$'+(i+1)).join()})
ON CONFLICT (time) DO UPDATE SET (server_time,${fields.join()})
= (CURRENT_TIMESTAMP,${fields.map(c=>'EXCLUDED.'+c).join()})`;
	await pool.query(query, Object.values(row));
	return 200;
}

module.exports = {
	pool,
	subscribe,
	unsubscribe,
	selectEmails,
	selectAll,
	selectInterval,
	select,
	insert,
	deleteCorrections,
	insertCorrections
};
