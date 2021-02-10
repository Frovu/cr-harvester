/* eslint-disable no-undef, no-unused-vars*/
global.log = () => {};
const db = require('../modules/database');
jest.spyOn(db, 'connect').mockImplementation(()=>{});
jest.spyOn(db, 'getSections').mockImplementation(()=>{return {
	'akey': {key: 'akey', asd: 'asd'}
};});

require('dotenv').config();
const express = require('express');
const request = require('supertest');
const bodyParser = require('body-parser');
const app = express();
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));
app.use('/api', require('../modules/api.js'));

describe('api', () => {
	describe('GET sections', () => {
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
});
