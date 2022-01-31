
function subscribe() {

}

function unsubscribe() {

}

export function showForm(type) {
	const form = document.getElementById('sub-form');
	const mail = document.getElementById('email');
	const secr = document.getElementById('secret');
	const opts = document.getElementById('sub-options');
	form.hidden = null;
	opts.hidden = type === 'sub' ? null : 'true';
	mail.value = '';
	secr.value = '';
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
