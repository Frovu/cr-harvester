
function show(name, station) {
	const parent = document.getElementById('select-station');
	for (const c of parent.children) {
		c.classList.remove('active');
		if (c.innerHTML == name)
	}
	const statusEl = document.getElementById('status');
	const descEl = document.getElementById('description');
	descEl.innerHTML = station.description || 'Empty';
	statusEl.innerHTML = '<span class="ok">Online</span>';
	window.localStorage.setItem('station', station);

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
		if (Object.keys(stations).length === 0)
			sel.innerHTML = '<p class="error">No stations are configured.<p>';
	} else {
		sel.innerHTML = '<p class="error">Failed to fetch stations list<p>';
		setTimeout(load, 3000);
	}
}

window.onload = load;
