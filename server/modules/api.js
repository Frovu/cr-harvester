const express = require('express');
const db = require('./data');
const stations = require('./stations');
const router = express.Router();

router.post('/subscriptions', async (req, res) => {
	const badRequest = (txt) => res.status(400).json({ error: txt });
	const body = req.body;
	if (!body || !body.station || !body.email)
		return badRequest('Bad request!');
	if (!stations.list().stations[body.station])
		return badRequest('Invalid station name!');
	if (!stations.validate(body.email))
		return badRequest('Invalid email address!');
	if (!stations.authorize(body.secret))
		return badRequest('Wrong secret key!');
	try {
		if (body.action === 'unsub')
			await db.unsubscribe(body.station, body.email);
		else
			await db.subscribe(body.station, body.email);
		global.log(`Subscriptions: ${body.station} ${body.action === 'sub' ? '++' : '--'} ${body.email}`);
		return res.sendStatus(200);
	} catch(e) {
		global.log(`Exception in subscribe: ${e}`);
		return res.sendStatus(500);
	}
});

router.get('/subscriptions', async (req, res) => {
	const subs = await stations.getSubscriptions();
	return res.status(200).json(subs);
});

router.get('/status', async (req, res) => {
	try {
		const devData = await db.selectAll(parseInt(req.query.limit));
		for (const dev in devData)
			devData[dev].ip = stations.getIp(dev);
		return res.status(200).json(devData);
	} catch(e) {
		global.log(`Exception in get stations: ${e.stack}`);
		return res.sendStatus(500);
	}
});

router.get('/stations', (req, res) => {
	return res.status(200).json(stations.list());
});

router.post('/corrections', async (req, res) => {
	const body = req.body;
	if (!body.secret || !stations.authorize(body.secret))
		return res.sendStatus(401);
	if (!body.fields || !body.corrections)
		return res.sendStatus(400);
	if (!stations.list().devices[body.device])
		return res.sendStatus(404);
	try {
		await db.insertCorrections(body.device, body.corrections, body.fields);
		global.log(`Corrections: ${body.device} <= [${body.corrections.length}] ${body.fields.join()}`);
		return res.sendStatus(200);
	} catch(e) {
		global.log(`Exception in corrections: ${e.stack}`);
		return res.sendStatus(500);
	}
});

router.delete('/corrections', async (req, res) => {
	const dev = req.body.device;
	const from = new Date(parseInt(req.body.from) * 1000);
	const to   = new Date(parseInt(req.body.to) * 1000);
	if (!req.body.secret || !stations.authorize(req.body.secret))
		return res.sendStatus(401);
	if (!dev || !stations.list().devices[dev])
		return res.sendStatus(404);
	if (isNaN(from) || isNaN(to) || to <= from)
		return res.sendStatus(400);
	try {
		await db.deleteCorrections(dev, from, to);
		global.log(`Corrections: ${dev} delete [${[from, to].map(d => d.toISOString().replace(/\..*/,'')).join(' ')}]`);
		return res.sendStatus(200);
	} catch(e) {
		global.log(`Exception in corrections delete: ${e.stack}`);
		return res.sendStatus(500);
	}
});

router.get('/data', async (req, res) => {
	try {
		const dev = req.query.device || req.query.dev;
		const from = parseInt(req.query.from);
		const to   = parseInt(req.query.to);
		const period = req.query.period ? parseInt(req.query.period) : 3600;
		const fields = (req.query.fields ?? req.query.what)?.split(',') ?? ['upper', 'lower', 'vertical', 'pressure', 'temperature_ext', 'voltage'];
		if (![60, 300, 600, 3600, 86400].includes(period))
			return res.status(400).send('Supported periods are 60, 300, 600, 3600, 86400');
		if (!dev || !stations.list().devices[dev])
			return res.status(404).send('Device not found');
		if (isNaN(from) || isNaN(to) || to - from < period)
			return res.status(400).send('Bad period');
		return res.status(200).json(await db.selectInterval(dev, from, to, period, fields));
	} catch(e) {
		global.log(`Exception in get data: ${e.stack}`);
		return res.sendStatus(500);
	}
});

router.get('/data/product', async (req, res) => {
	try {
		const station = stations.list().stations[req.query.station];
		const from = parseInt(req.query.from);
		const to   = parseInt(req.query.to);
		const period = req.query.period ? parseInt(req.query.period) : 3600;
		if (![60, 300, 600, 3600, 86400].includes(period))
			return res.status(400).send('Supported periods are 60, 300, 600, 3600, 86400');
		if (!station)
			return res.status(404).send('Station not found');
		if (isNaN(from) || isNaN(to) || to - from < period)
			return res.status(400).send('Bad period');
		/* if (station.nm) {
			console.log('Not implemented');
			return res.sendStatus(500);
		} else */ if (station.devices.length === 1) {
			const devKey = station.devices[0];
			const dev = stations.list().devices[devKey];
			const fields = dev.counters.concat(dev.fields);
			return res.status(200).json(await db.selectInterval(devKey, from, to, period, fields));
		} else {
			return res.status(500).send('Station configuration error');
		}
	} catch(e) {
		global.log(`Exception in get data: ${e.stack}`);
		return res.sendStatus(500);
	}
});

router.post('/data', async (req, res) => {
	const from = req.headers['x-forwarded-for'];
	try {
		const status = await db.insert(req.body);
		stations.gotData(req.body.k, from);
		global.log(`[${status}] (${from}) ${JSON.stringify(req.body)}`);
		res.sendStatus(status);
	} catch(e) {
		global.log(`Exception inserting data: ${e}`);
		global.log(`[500] (${from}) ${JSON.stringify(req.body)}`);
		res.sendStatus(500);
	}
});

module.exports = router;
