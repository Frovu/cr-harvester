'use strict';
const nodemailer = require('nodemailer');
let transport;

async function connect() {
	transport = nodemailer.createTransport({
		host: process.env.SMTP_SERVER,
		port: 587,
		secure: false,
		auth: {
			user: process.env.SMTP_USER,
			pass: process.env.SMTP_PASS,
		},
	});
}

async function send(to, subject, html) {
	if (!transport) await connect();
	return await transport.sendMail({
		from: '"cr-sentinel" <noreply@izmiran.ru>',
		to: to.join(', '),
		subject,
		html
	});
}

module.exports = {
	send
};
