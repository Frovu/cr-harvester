const express = require('express');
const db = require('./data');
const stations = require('./stations');
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
	if (!stations.get(req.params.id))
		return res.sendStatus(404);
	try {
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
	const from = req.headers['x-forwarded-for'];
	try {
		const status = await db.insert(req.body);
		res.sendStatus(status);
		stations.gotData(req.body.k, from);
		global.log(`[${status}] (${from}) ${JSON.stringify(req.body)}`);
	} catch(e) {
		global.log(`Exception inserting data: ${e}`);
		global.log(`[500] (${from}) ${JSON.stringify(req.body)}`);
		res.sendStatus(500);
	}
});

module.exports = router;
