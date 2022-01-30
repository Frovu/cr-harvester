
let stations;

async function load() {
	const sel = document.getElementById('select-station');
	sel.innerHTML = '';
	const resp = await fetch('api/stations').catch(e => { console.error(e); });
	if (resp && resp.status == 200) {
		stations = await resp.json();
		sel.append('');

	} else {
		sel.innerHTML = '<p class="error">Failed to fetch stations list<p>';
		setTimeout(load, 3000);
	}
}

window.onload = load;
