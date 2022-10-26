'use strict';
const config = require('../util/config').config;
const db = require('./data');
const validator = require('email-validator');
const mailer = require('../util/mailing');

const DEFAULT_WATCHDOG_MS = 5 * 60000;
const ALERTS_DELAY_MS = 60 * 60 * 1000; // don't send alerts more often than 1 per hour per device

const ipCache = {};
const alertsLog = {};
const watchdogs = {};

async function getSubscriptions() {
	const res = {};
	for (const station in config.stations)
		res[station] = await db.selectEmails([station]);
	return res;
}

async function alertDeviceLost(devId) {
	try {
		global.log(`Device is lost: ${devId}`);
		const stations = Object.keys(config.stations).filter(s => config.stations[s].devices.includes(devId));
		const addresses = await db.selectEmails(stations);
		const lastAlert = alertsLog[devId];
		if (lastAlert && lastAlert - new Date() < ALERTS_DELAY_MS)
			return global.log(`Alert's too soon from '${devId}'`);
		global.log(`Sending '${devId}' alerts to: ${addresses}`);
		await stations.sendDeviceAlert(devId, );
		await mailer.send(addresses, 'device lost', `<h3>Warning!</h3>
We've stopped receiving data from the <b>${devId}</b>.`);
	} catch(e) {
		global.log(`Exception in alertDeviceLost: ${e}`);
	}
}

function gotData(devId, ipAddr) {
	ipCache[devId] = ipAddr;
	const dev = config.devices[devId];
	clearInterval(watchdogs[devId]);
	if (!dev?.watchdog) return;
	const timeout = dev.watchdog === 'default' ? DEFAULT_WATCHDOG_MS : parseInt(dev.watchdog) * 60000;
	if (isNaN(timeout))
		return global.log(`Invalid watchdog value for ${devId}: \`${dev.watchdog}\``);
	watchdogs[devId] = setTimeout(() => alertDeviceLost(devId));
}

function list() {
	const list = {};
	for (const dev in config.devices) {
		list[dev] = {
			description: config.devices[dev].description,
			counters: config.devices[dev].counters,
			fields: config.devices[dev].fields
		};
	}
	return {
		devices: list,
		stations: config.stations
	};
}

module.exports = {
	validate: validator.validate,
	getIp: id => ipCache[id],
	authorize: secret => secret === process.env.SECRET_KEY,
	getSubscriptions,
	gotData,
	list
};
