const stations = require('./stations');

const DATA_WATCHDOG_M   = 5;
const INTEGRITY_CHECK_M = 60; // 1 hour

const DATA_WATCHDOG_MS   = 3000// DATA_WATCHDOG_M * 60000;
const INTEGRITY_CHECK_MS = INTEGRITY_CHECK_M * 60000;

let watchdogs = {};

async function alertIntegrity(devId) {
	try {
		global.log('Not implemented');
	} catch(e) {
		global.log(`Exception in alertDeviceLost: ${e}`);
	}
}

async function alertDeviceLost(devId) {
	try {
		global.log(`Device is lost: ${devId}`);
		await stations.sendDeviceAlert(devId, `<h3>Warning!</h3>
We've stopped receiving data from the <b>${devId}</b> device about ${DATA_WATCHDOG_M.toFixed(0)} minutes ago.`);
	} catch(e) {
		global.log(`Exception in alertDeviceLost: ${e}`);
	}
}

function gotData(data) {
	const devId = data.k;
	if (watchdogs[devId])
		clearInterval(watchdogs[devId]);
	watchdogs[devId] = setTimeout(() =>  alertDeviceLost(devId), DATA_WATCHDOG_MS);
}

module.exports = {
	gotData
};
