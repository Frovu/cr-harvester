const db = require('./database');

const CHUNK_SIZE = 600;
const PERIOD_LEN_S = 60;
const PERIOD_LEN_MS = PERIOD_LEN_S * 1000;
const ALIGNED_WINDOW_MS = 100;

async function analyse(device, periodStart, table='nm_data') {
	const result = {
		missing: 0,
		unaligned: 0,
		untrusted: 0,
		channelAbsence: 0,
	};
	periodStart = new Date(periodStart.toString().includes('T') ? periodStart : periodStart);
	if (isNaN(periodStart.getTime())) return null;
	const periodStartMs =  Math.floor(periodStart.getTime() / PERIOD_LEN_MS) * PERIOD_LEN_MS;

	const lq = `SELECT * FROM ${table} WHERE device_id = $1 ORDER BY at DESC LIMIT 1`;
	const lastLine = await db.pool.query(lq, [device]);
	const dataLast = lastLine.rows[0].dt;
	for (let chunkLast, chunk = periodStartMs; chunk <= dataLast; chunk=chunkLast+PERIOD_LEN_MS) {
		chunkLast = chunk + PERIOD_LEN_MS * CHUNK_SIZE;
		const subq = `SELECT * FROM ${table} WHERE device_id = $1 AND dt >= $2 AND dt <= $3`;
		const res = await db.pool.query({
			text: `SELECT * FROM (${subq} ORDER BY ID ASC LIMIT $4) as t ORDER BY dt`,
			values: [device, new Date(chunk), new Date(chunkLast), CHUNK_SIZE],
			// rowMode: 'array'
		});
		for (const row of res.rows) {
			if (row.info % 2 == 1)
				result.untrusted++;
			if (Math.abs(row.at - row.dt - PERIOD_LEN_MS) > 1000)
				result.channelAbsence++;
			if (Math.abs(row.dt - Math.round(row.dt/PERIOD_LEN_MS)*PERIOD_LEN_MS) > ALIGNED_WINDOW_MS) {
				result.unaligned++;
			}
		}
		const expectedAmount = chunkLast < dataLast ? CHUNK_SIZE : (1+Math.floor((dataLast - chunk) / PERIOD_LEN_MS));
		result.missing += expectedAmount - res.rows.length;
	}
	return result;
}

module.exports = {
	analyse: analyse
};
