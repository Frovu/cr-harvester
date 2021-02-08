const express = require('express');
const db = require('./database');
const router = express.Router();

router.get('sections/', (req, res) => {
	return res.sendStatus(501);
});

router.get('data/', (req, res) => {
	return res.sendStatus(501);
});

router.post('data/', (req, res) => {
	return res.sendStatus(501);
});

module.exports = router;
