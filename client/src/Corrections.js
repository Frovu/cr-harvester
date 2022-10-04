import { useEffect, useState } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import './css/Subscriptions.css';

function Selector({ text, options, selected, callback }) {
	return (
		<div className="Selector">
			<span>{text}</span>
			<select value={selected} onChange={e => callback(e.target.value)}>
				{!selected ? <option key='default' disabled selected>-- none --</option> : ''}
				{options.map(o => <option key={o} value={o}>{o}</option>)}
			</select>
		</div>
	);
}

function Editor() {

}

export default function Corrections({ devices }) {
	const [settings, setSettings] = useState({});
	const settingsCallback = key => value => setSettings(state => ({ ...state, [key]: value }));

	useEffect(() => setSettings(JSON.parse(window.localStorage.getItem('corrSettings'))), []);
	useEffect(() => window.localStorage.setItem('corrSettings', JSON.stringify(settings)), [settings]);

	const options = {
		device: Object.keys(devices),
		mode: ['single', 'counters', 'all'],
		field: settings.mode !== 'single' ? null :
			devices[settings.device]?.counters.concat(devices[settings.device]?.fields)
	};
	const selectors = Object.keys(options).filter(k => options[k]).map(key =>
		<Selector
			key={key}
			text={key.charAt(0).toUpperCase()+key.slice(1)+':'}
			options={options[key]}
			selected={settings[key]}
			callback={settingsCallback(key)}
		/>
	);

	return (
		<div className="Corrections">
			<div className="Settings">
				{selectors}
			</div>
		</div>
	);
}
