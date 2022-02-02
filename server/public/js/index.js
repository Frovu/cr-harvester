import * as mailing from './mailing.js';
import * as stats from './stats.js';

async function show(station) {
	const stations = document.getElementById('select-station');
	for (const c of stations.children) {
		c.classList.remove('active');
		if (c.innerHTML === station.name)
			c.classList.add('active');
	}
	const statusEl = document.getElementById('status');
	const descEl = document.getElementById('description');
	descEl.innerHTML = station.description || 'Empty';
	statusEl.innerHTML = '<span class="error">-----</span>';
	mailing.init(station);
	stats.init(station.id, station.plotsConfig);
	window.localStorage.setItem('station', station.id);
	const devdiv = document.getElementById('devices-div');
	const stadiv = document.getElementById('status-div');
	devdiv.hidden = null;
	stadiv.hidden = null;
}

async function load() {
	const sel = document.getElementById('select-station');
	sel.innerHTML = '';
	const resp = await fetch('api/stations').catch(e => { console.error(e); });
	if (resp && resp.status == 200) {
		const stations = await resp.json();
		for (const s in stations) {
			const el = document.createElement('p');
			el.classList.add('station-option');
			stations[s].id = s;
			el.innerHTML = stations[s].name;
			el.addEventListener('click', () => {
				show(stations[s]);
			});
			sel.append(el);
		}
		if (Object.keys(stations).length === 0) {
			sel.innerHTML = '<p class="error">No stations are configured.<p>';
		} else {
			const saved = window.localStorage.getItem('station');
			if (saved && stations[saved]) show(stations[saved]);
		}
	} else {
		sel.innerHTML = '<p class="error">Failed to fetch stations list<p>';
		setTimeout(load, 3000);
	}
}

window.onload = load;
