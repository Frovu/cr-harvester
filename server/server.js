const express = require('express');
const bodyParser = require('body-parser');
require('dotenv').config();

global.log = require('./util/logging.js');

const app = express();

app.use(bodyParser.json()); // support json encoded bodies
app.use(bodyParser.urlencoded({ extended: true })); // support url-encoded

app.use((req, res, next) => {
	res.setHeader('Access-Control-Allow-Origin', 'http://localhost:3000');
	res.setHeader('Access-Control-Allow-Headers', 'X-Requested-With,content-type');
	res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS, PUT, PATCH, DELETE');
	next();
});

app.use('/api', require('./modules/api.js'));

app.listen(process.env.PORT, () => global.log(`Listening to port ${process.env.PORT}`));
