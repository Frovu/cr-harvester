'use strict';
const nodemailer = require('nodemailer');
let transport;

async function connect() {
	transport = nodemailer.createTransport({
		host: process.env.SMTP_SERVER,
		port: 587,
		secure: false, // true for 465, false for other ports
		auth: {
			user: process.env.SMTP_USER, // generated ethereal user
			pass: process.env.SMTP_PASS, // generated ethereal password
		},
	});
}

async function send(to, subject, html) {
	if (!transport) await connect();

	let info = await transport.sendMail({
		from: '"neutron-monitor" <noreply@izmiran.ru>',
		to: to.join(', '),
		subject,
		html
	});

	console.log('Message sent: %s', info.messageId);
}

module.exports = {
	send
};
