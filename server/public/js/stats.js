import uPlot from './uPlot.esm.js';

let interval;

const delay = ms => new Promise(res => setTimeout(res, ms));

function getPlotSize(parent) {
	let height = parent.offsetHeight - 40;
	return { width: Math.floor(height * 1.5), height: height};
}
function createPlot(parent, sizeEl, config, data=null) {
	const el = document.createElement('div');
	el.classList.add('device-plot');
	const color = config.color || 'rgba(200,200,100,1)';
	const gray = 'rgba(255,255,255,0.1)';
	const plot = new uPlot({
		title: config.name,
		...getPlotSize(sizeEl),
		// tzDate: ts => uPlot.tzDate(new Date(ts), 'UTC'),
		cursor: {
			drag: { dist: 12 },
			points: { size: 6, fill: (self, i) => self.series[i]._stroke }
		},
		series: [
			{ value: '{HH}:{mm} UTC' }, // '{YYYY}-{MM}-{DD} {HH}:{mm} UTC'
			{
				stroke: color,
				label: config.name,
				value: (u, v) => v == null ? '-' : v.toFixed(config.precision||0) + (!config.units ? '' : '\n'+config.units),
				points: { size: 7 , fill: 'transparent', stroke: color }
			}
		],
		axes: [
			{
				stroke: 'darkgrey',
				grid: { stroke: gray, width: 1 },
				values: '{HH}:{mm}',
				size: 30

			},
			{
				// space: 32,
				grid: { stroke: gray, width: 1 },
				ticks: { stroke: gray, width: 1 },
				stroke: 'darkgrey',
				values: (u, vals) => vals.map(v => v.toFixed(config.precision||0) + (!config.units ? '' : '\n'+config.units))
			}
		],
		legend: { show: false }
	}, data, el);
	parent.addEventListener('resize', () => {
		plot.setSize(getPlotSize(sizeEl));
	});
	parent.append(el);
	return plot;
}

function intervalToString(seconds) {
	const days = Math.floor(seconds / 86400);
	const hours = Math.floor(seconds / 3600);
	const minutes = Math.floor(seconds / 60);
	const n = days || hours || minutes || seconds.toFixed(0);
	return `${n} ${days ? 'day' : hours ? 'hour' : minutes ? 'minute' : 'second'}${n>1?'s':''}`;
}

function devStatusHtml(name, status, stats, statusPlus='') {
	const ll = stats && stats.lastLine;
	const uptime = ll && ll.uptime*60;
	const dt = ll && ll.dt;
	return `
<h4>${name}</h4>
<p><span class="time">${dt}</span></p>
<p>Status: <span class="${status==='Online'?'ok':'error'}">${status}</span>${statusPlus}</p>
<p>Uptime: ${uptime?intervalToString(uptime):'N/A'}</p>
<p>IP: ${stats&&stats.lastIp||'N/A'}</p>

`;
}

async function update(stationId, elements, plots) {
	try {
		const res = await fetch('api/stations/'+stationId);
		if (res.status !== 200)
			throw new Error('not OK');
		const devices = await res.json();
		let ok = 0;
		for (const dev in devices) {
			const el = elements[dev];
			const stat = devices[dev];
			const plot = plots[dev];
			if (stat.lastLine) {
				const timePast = new Date().getTime() - new Date(stat.lastLine.at).getTime();
				const online = timePast < 100000; // 1.5 minutes
				if (online) {
					ok += 1;
					el.innerHTML = devStatusHtml(dev, 'Online', stat);
				} else {
					el.innerHTML = devStatusHtml(dev, 'Lost', stat, `&nbsp;(${intervalToString(timePast/1000)} ago)`);
				}
				if (plots[dev]) {
					const data = devices[dev].data;
					const time = data.map(r => new Date(r.dt).getTime()/1000);
					for (const f in plots[dev]) {
						const row = data.map(r => r[f]);
						plots[dev][f].setData([time, row]);
					}
				}
			} else {
				plot && plot.destroy();
				el.innerHTML = devStatusHtml(dev, 'N/A');
			}
			el.parentNode.dispatchEvent(new Event('resize'));
		}
		const statusEl = document.getElementById('status');
		const status = ok===Object.keys(devices).length ? 'Online' : ok > 0 ? 'Online partially' : 'Offline';
		const sclass = ok===Object.keys(devices).length ? 'ok' : ok > 0 ? 'warn' : 'error';
		for (const a of '\\|/-\\|/') {
			statusEl.innerHTML = `<span class="warn">-${a}${a}${a}-</span>`;
			await delay(125);
		}
		statusEl.innerHTML = `<span class="${sclass}">${status}</span>`;
	} catch (e) {
		console.error(e);
		const parent = document.getElementById('devices');
		parent.innerHTML = '<a class="error">Failed to update status!</a>';
		if (interval) clearInterval(interval);
		return setTimeout(() => init(stationId), 3000);
	}
}

export async function init(stationId, plotsConfig) {
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
		el.classList.add('device-container');
		const devStEl = elements[dev] = document.createElement('div');
		devStEl.classList.add('device-status');
		el.append(devStEl);
		parent.append(el);
		if (plotsConfig) {
			plots[dev] = {};
			for (const p in plotsConfig) {
				plots[dev][p] = createPlot(el, devStEl, plotsConfig[p], stats[dev].data);


			}

		}
	}
	update(stationId, elements, plots);
	interval = setInterval(() => update(stationId, elements, plots), 20000);
}
