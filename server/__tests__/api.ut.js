/* eslint-disable no-undef, no-unused-vars*/
global.log = (a) => {}; // console.log(a);
const db = require('../modules/database');
db.pool.query = jest.fn((q, args) => {
	values = [ '13.37', '13.38', 1, 2, undefined, new Date('1970-01-01T00:00:00.000Z')];
	for (i=0; i<args.length; ++i) {
		if ((i != 5) ? (args[i] != values[i]) : args[i].getTime() != values[i].getTime())
			return false;
	}
	return true;
});
jest.spyOn(db, 'connect').mockImplementation(()=>{});
jest.spyOn(db, 'validate').mockImplementation(d=>d.k === 'valid_key');
jest.spyOn(db, 'getSections').mockImplementation(()=>{return {
	'akey': {key: 'akey', asd: 'asd'}
};});

require('dotenv').config();
const express = require('express');
const request = require('supertest');
const bodyParser = require('body-parser');
const app = express();
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true, inflate: true }));
app.use('/api', require('../modules/api.js'));

describe('api', () => {
	describe('GET /sections', () => {
		it('responds with 200', async () => {
			const res = await request(app).get('/api/sections');
			expect(res.status).toEqual(200);
		});
		it('does not show auth keys', async () => {
			const res = await request(app).get('/api/sections');
			const b = res.body;
			expect(b).toEqual([{asd: 'asd'}]);
		});
	});
	describe('POST /data', () => {
		const data = 'dt=1970-01-01T00:00:00Z&t=13.37&p=13.38&c[]=1&c[]=2&k=valid_key';
		it('responds 400 to malformed or unauthorized data', async () => {
			let res = await request(app).post('/api/data').send('t=1');
			expect(res.status).toEqual(400);
			res = await request(app).post('/api/data').send('t=1&k=valid_key');
			expect(res.status).toEqual(500);
			res = await request(app).post('/api/data').send('t=1&c=1,2&k=valid_key');
			expect(res.status).toEqual(500);
			expect(db.pool.query).not.toHaveBeenCalled();
		});
		it('translates values correctly', async () => {
			let res = await request(app).post('/api/data').send(data);
			expect(res.status).toEqual(200);
			expect(db.pool.query).toHaveLastReturnedWith(true);
		});
	});
});
