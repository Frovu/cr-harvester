const fs = require('fs');
const validator = require('email-validator');
const mailer = require('./mailing');
const db = require('./database');

const STATS_H = process.env.STATS_H || 1;

const JSON_PATH = 'stations.json';
const stations = fs.existsSync(JSON_PATH) ? JSON.parse(fs.readFileSync(JSON_PATH)) : {};
const commit = () => {
	fs.writeFile(JSON_PATH, JSON.stringify(stations, null, 2), err => {
		if (err) global.log('ERROR: writhing '+JSON_PATH);
	});
};

const ALERTS_DELAY_MS = 30 * 60 * 1000; // don't send alerts more often than 1 per hour per station
const OPTIONS = [ 'failures', 'events' ];
const ipCache = {};
const alertsLog = { failures: {}, events: {} };

function authorize(key) {
	return process.env.SECRET_KEY === key;
}

function list() {
	return stations;
}

function get(id) {
	return stations[id];
}

async function sendDeviceAlert(devId, html, type='failures') {
	const addresses = [];
	let parentStation;
	for (const station of Object.values(stations)) {
		if (station.devices.includes(devId) && station.mailing) {
			for (const mail of station.mailing[type])
				if (!addresses.includes(mail))
					addresses.push(mail);
			parentStation = station.name;
		}
	}
	const lastAlert = alertsLog[type][parentStation];
	if (lastAlert && lastAlert - new Date() < ALERTS_DELAY_MS)
		return global.log(`Alert's too soon for '${parentStation}'`);
	global.log(`Sending '${parentStation}' alerts to: ${addresses}`);
	alertsLog[type][parentStation] = new Date();
	await mailer.send(addresses, parentStation, html);
}

function logIp(dev, addr) {
	ipCache[dev] = addr;
}

async function stats(station) {
	const devs = stations[station].devices;
	const stat = {};
	for (const devKey of devs) {
		const dev = db.devices[devKey];
		const q = `SELECT * FROM ${dev.type}_data WHERE device_id=${dev.id}
			AND dt >= CURRENT_TIMESTAMP - '${STATS_H} h'::interval ORDER BY dt LIMIT ${STATS_H*60}`;
		const res = await db.pool.query(q);
		let lastLine = res.rows[res.rows.length-1];
		if (!lastLine) {
			const lres = await db.pool.query(`SELECT at, dt, uptime FROM ${dev.type}_data WHERE device_id=${dev.id} ORDER BY dt DESC LIMIT 1`);
			lastLine = lres.rows[0];
		}
		stat[devKey] = {
			lastLine,
			data: res.rows,
			lastIp: ipCache[devKey]
		};
	}
	return stat;
}

function subscribe(station, email, options=[]) {
	const mailing = stations[station].mailing;
	for (const opt of OPTIONS) {
		if (options.includes(opt)) {
			if (!mailing[opt].includes(email))
				mailing[opt].push(email);
		} else {
			mailing[opt] = mailing[opt].filter(e => e !== email);
		}
	}
	commit();
}

module.exports = {
	validate: validator.validate,
	sendDeviceAlert,
	authorize,
	subscribe,
	logIp,
	stats,
	list,
	get
};
