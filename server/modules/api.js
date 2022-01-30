const express = require('express');
const db = require('./database');
const stations = require('./stations');
db.connect();
const router = express.Router();

router.get('/stations', (req, res) => {
	return res.status(200).json(stations.get());
});

router.get('/data', (req, res) => {
	return res.sendStatus(501);
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
