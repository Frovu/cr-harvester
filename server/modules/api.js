const express = require('express');
const db = require('./database');
const stations = require('./stations');
db.connect();
const router = express.Router();

router.post('/stations/subscribe', (req, res) => {
	const badRequest = (txt) => res.status(400).json({ error: txt });
	const body = req.body;
	if (!body || !body.station || !body.email || !body.options)
		return badRequest('Bad request!');
	if (!stations.get()[body.station])
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

router.get('/stations', (req, res) => {
	return res.status(200).json(stations.get());
});


router.post('/data', async (req, res) => {
	// log every request to not loose data in case of some protocol validation issues
	global.log(`(${req.headers['x-forwarded-for']}) >>> ${JSON.stringify(req.body)}`);
	if (!req.body || !db.validate(req.body))
		return res.sendStatus(400);
	if (!db.authorize(req.body))
		return res.sendStatus(401);
	try {
		await db.insert(req.body);
		return res.sendStatus(200);
	} catch(e) {
		global.log(`Exception inserting data: ${e}`);
		return res.sendStatus(500);
	}
});

module.exports = router;
