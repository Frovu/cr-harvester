const express = require('express');
const db = require('./database');
const stations = require('./stations');
const watchdogs = require('./watchdogs');
db.connect();
const router = express.Router();

router.post('/stations/subscribe', (req, res) => {
	const badRequest = (txt) => res.status(400).json({ error: txt });
	const body = req.body;
	if (!body || !body.station || !body.email || !body.options)
		return badRequest('Bad request!');
	if (!stations.get(body.station))
		return badRequest('Invalid station name!');
	if (body.options.length && !stations.validate(body.email))
		return badRequest('Invalid email address!');
	if (!stations.authorize(body.secret))
		return badRequest('Wrong secret key!');
	try {
		stations.subscribe(body.station, body.email, body.options);
		return res.sendStatus(200);
	} catch(e) {
		global.log(`Exception in subscribe: ${e}`);
		return res.sendStatus(500);
	}
});

router.get('/stations/:id', async (req, res) => {
	try {
		if (!stations.get(req.params.id))
			return res.sendStatus(404);
		const stat = await stations.stats(req.params.id);
		return res.status(200).json(stat);
	} catch(e) {
		global.log(`Exception in get station: ${e}`);
		return res.sendStatus(500);
	}
});


router.get('/stations', (req, res) => {
	return res.status(200).json(stations.list());
});


router.post('/data', async (req, res) => {
	// log every request to not loose data in case of some protocol validation issues
	const from = req.headers['x-forwarded-for'];
	const sendStatus = status => {
		res.sendStatus(status);
		global.log(`[${status}] (${from}) ${JSON.stringify(req.body)}`);
	};
	if (!req.body || !db.validate(req.body))
		return sendStatus(400);
	if (from)
		stations.logIp(req.body.k, from);
	if (!db.authorize(req.body))
		return sendStatus(401);
	try {
		await db.insert(req.body);
		watchdogs.gotData(req.body);
		return sendStatus(200);
	} catch(e) {
		global.log(`Exception inserting data: ${e}`);
		return sendStatus(500);
	}
});

module.exports = router;
