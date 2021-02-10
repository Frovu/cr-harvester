const express = require('express');
const db = require('./database');
const router = express.Router();

router.get('/sections', (req, res) => {
	// hide devices authority keys
	const sections = db.getSections();
	for(const s in sections) sections[s].key = undefined;
	return res.status(200).json(Object.values(sections));
});

router.get('/data', (req, res) => {
	return res.sendStatus(501);
});

router.post('/data', (req, res) => {
	return res.sendStatus(501);
});

module.exports = router;
