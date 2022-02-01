
let interval;
let statInterval;

function intervalToString(seconds) {
	const days = Math.floor(seconds / 86400);
	const hours = Math.floor(seconds / 3600);
	const minutes = Math.floor(seconds / 60);
	const n = days || hours || minutes || seconds.toFixed(0);
	return `${n} ${days ? 'day' : hours ? 'hour' : minutes ? 'minute' : 'second'}${n>1?'s':''}`;
}

function devStatusHtml(name, status, uptime, ip, statusPlus='') {
	return `
<h4>${name}</h4>
<p>Status: <span class="${status==='Online'?'ok':'error'}">${status}</span>${statusPlus}</p>
<p>Uptime: ${uptime?intervalToString(uptime*60):'N/A'}</p>
<p>IP: ${ip||'N/A'}</p>
`;
}

async function update(stationId, elements, plots) {
	try {
		const res = await fetch('api/stations/'+stationId);
		if (res.status !== 200)
			throw new Error('not OK');
		const data = await res.json();
		let ok = 0;
		for (const dev in data) {
			const el = elements[dev];
			const stat = data[dev];
			const plot = plots[dev];
			if (stat.lastLine) {
				const timePast = new Date().getTime() - new Date(stat.lastLine.at).getTime();
				const online = timePast < 100000; // 1.5 minutes
				if (online) {
					ok += 1;
					el.innerHTML = devStatusHtml(dev, 'Online', stat.lastLine.uptime, stat.lastIp);
				} else {
					el.innerHTML = devStatusHtml(dev, 'Lost', null, null, `&nbsp;(${intervalToString(timePast/1000)} ago)`);
				}
			} else {
				// TODO
				plot.destroy();
				el.innerHTML = devStatusHtml(dev, 'N/A');
			}
		}
		const statusEl = document.getElementById('status');
		const status = ok===Object.keys(data).length ? 'Online' : ok > 0 ? 'Online partially' : 'Offline';
		const sclass = ok===Object.keys(data).length ? 'ok' : ok > 0 ? 'warn' : 'error';
		const html = s => `<span class="${sclass}">${status}</span> (${s.toFixed(0)} second${s==1?'':'s'} ago)`;
		statusEl.innerHTML = html(0);
		const updated = new Date();
		clearInterval(statInterval);
		statInterval = setInterval(() => {
			statusEl.innerHTML = html((new Date() - updated) / 1000);
		}, 1000);
	} catch (e) {
		console.error(e);
		const parent = document.getElementById('devices');
		parent.innerHTML = '<a class="error">Failed to update status!</a>';
		if (interval) clearInterval(interval);
		return setTimeout(() => init(stationId), 3000);
	}
}

export async function init(stationId) {
	if (interval) clearInterval(interval);
	const parent = document.getElementById('devices');
	parent.innerHTML = '';
	const res = await fetch('api/stations/'+stationId);
	if (res.status !== 200) {
		parent.innerHTML = '<a class="error">Failed to fetch station!</a>';
		return setTimeout(() => init(stationId), 3000);
	}
	const stats = await res.json();
	const elements = {};
	const plots = {};
	for (const dev in stats) {
		const el = document.createElement('div');
		const devStEl = elements[dev] = document.createElement('div');
		devStEl.classList.add('device-status');
		const graphEl = document.createElement('div');
		el.append(devStEl, graphEl);
		parent.append(el);
	}
	update(stationId, elements, plots);
	interval = setInterval(() => update(stationId, elements, plots), 20000);
}
