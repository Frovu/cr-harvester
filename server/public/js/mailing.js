
let msgTimeout;

function showMessage(html, error=true) {
	const parent = document.getElementById('sub-message');
	parent.classList[error ? 'add' : 'remove']('error');
	parent.innerHTML = html;
	clearTimeout(msgTimeout);
	msgTimeout = setTimeout(() => { parent.innerHTML = '<br>'; }, 2000);
}

async function subscribe(unsub=false) {
	const secret = document.getElementById('secret');
	const mailEl = document.getElementById('email');
	const email = mailEl.value;
	let options;
	for (const op of ['failures', 'events']) {
		const el = document.getElementById('sub-' + op);
		if (el.checked)
			options = options ? options.concat(op) : [op];
	}
	if (!email || !email.includes('@'))
		return showMessage('Doesn\'t look like an email');
	const res = await fetch('stations/subscribe', {
		method: 'POST',
		body: JSON.stringify({
			secret: secret.value,
			email: email,
			options: options
		}),
		headers: { 'Content-Type': 'application/json' }
	}).catch(e => console.error(e));
	if (res && res.status === 400) {
		const resp = await res.json();
		console.log(resp)
	} else if (res && res.status === 200) {
		const subEl = document.getElementById('subscribed');
		showMessage((unsub ? 'Removed: ' : 'Subscribed: ')+email, false);
		if (unsub)
			subEl.innerHTML = subEl.innerHTML.split('<br>').filter(l => !l.includes(email)).join('<br>');
		else
			subEl.innerHTML = email + ' - ' + options.map(o => o.toUpperCase()).join(', ');
	} else {
		showMessage('Failed to fetch server');
	}
}

export function showForm(type) {
	const form = document.getElementById('sub-form');
	const mail = document.getElementById('email');
	const secr = document.getElementById('secret');
	const opts = document.getElementById('sub-options');
	const subm = document.getElementById('submit');
	form.hidden = null;
	opts.hidden = type === 'sub' ? null : 'true';
	mail.value = '';
	secr.value = '';
	subm.innerHTML = type === 'sub' ? 'Subscribe' : 'Unsubscribe';
	subm.onclick = () => type === 'sub' ? subscribe() : subscribe(false);
}

export function hideForm() {
	const form = document.getElementById('sub-form');
	form.hidden = 'true';
}

export function init(station) {
	hideForm();
	const subEl = document.getElementById('subscribed');
	const mailing = {};
	for (const email of station.mailing.failures)
		mailing[email] = 'FAILURES';
	for (const email of station.mailing.events) {
		if (mailing[email])
			mailing[email] += ', EVENTS';
		else
			mailing[email] = 'EVENTS';
	}
	subEl.innerHTML = Object.keys(mailing).map(m => `${m}<i> - ${mailing[m]}</i>`).join('<br>\n');

	const btns = [document.getElementById('sub'), document.getElementById('unsub')];
	for (const btn of btns) {
		btn.classList.remove('active');
		btn.addEventListener('click', () => {
			btn.classList.toggle('active');
			btns.forEach(b => b.id !== btn.id && b.classList.remove('active'));
			if (btn.classList.contains('active'))
				showForm(btn.id);
			else
				hideForm();
		});
	}
}
