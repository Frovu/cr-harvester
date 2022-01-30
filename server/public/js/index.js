
function show(station) {

}

async function load() {
	const sel = document.getElementById('select-station');
	sel.innerHTML = '';
	const resp = await fetch('api/stations').catch(e => { console.error(e); });
	if (resp && resp.status == 200) {
		const stations = await resp.json();
		for (const s in stations) {
			const el = document.createElement('p');
			el.innerHTML = s;
			el.addEventListener('click', () => {
				const parent = document.getElementById('select-station');
				for (const c of parent.children) {
					c.classList.remove('active');
				}
				el.classList.add('active');
				show(s);
			});
		}
		if (Object.keys(stations).length === 0)
			sel.innerHTML = '<p class="error">No stations are configured.<p>';
	} else {
		sel.innerHTML = '<p class="error">Failed to fetch stations list<p>';
		setTimeout(load, 3000);
	}
}

window.onload = load;
