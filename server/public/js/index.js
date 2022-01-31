
function show(name, station) {
	const stations = document.getElementById('select-station');
	for (const c of stations.children) {
		c.classList.remove('active');
		if (c.innerHTML === name)
			c.classList.add('active');
	}
	const statusEl = document.getElementById('status');
	const descEl = document.getElementById('description');
	const mailEl = document.getElementById('mailing');
	descEl.innerHTML = station.description || 'Empty';
	statusEl.innerHTML = '<span class="ok">Online</span>';
	const mailing = {};
	for (const email of station.mailing.failures)
		mailing[email] = 'FAILURES';
	for (const email of station.mailing.events) {
		if (mailing[email])
			mailing[email] += ', EVENTS';
		else
			mailing[email] = 'EVENTS';
	}
	mailEl.innerHTML = Object.keys(mailing).map(m => `${m}<i> - ${mailing[m]}</i>`).join('<br>\n');
	window.localStorage.setItem('station', name);
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
			el.innerHTML = s;
			el.addEventListener('click', () => {
				show(s, stations[s]);
			});
			sel.append(el);
		}
		if (Object.keys(stations).length === 0) {
			sel.innerHTML = '<p class="error">No stations are configured.<p>';
		} else {
			const saved = window.localStorage.getItem('station');
			if (saved && stations[saved]) show(saved, stations[saved]);
		}
	} else {
		sel.innerHTML = '<p class="error">Failed to fetch stations list<p>';
		setTimeout(load, 3000);
	}
}

window.onload = load;
