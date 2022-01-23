const express = require('express');
const db = require('./database');
// const analyse = require('./analysis');
db.connect();
const router = express.Router();

// router.get('/sections', (req, res) => {
// 	// hide devices authority keys
// 	const sections = db.getSections();
// 	for(const s in sections) sections[s].key = undefined;
// 	return res.status(200).json(Object.values(sections));
// });

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
