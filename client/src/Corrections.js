import { useEffect, useState } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import './css/Corrections.css';

function IntervalInput({ callback }) {
	const modes = ['recent', 'dates', 'interval'];
	const [mode, setMode] = useState();
	return (
		<>
			<Selector text="Time:" selected={mode} options={modes} callback={setMode}/>
			<div>
				{ mode === 'dates' &&
					<><input type="date"/><span> to </span><input type="date"/></> }
				{ mode === 'interval' &&
					<><input type="text" style={{ width: '3ch' }} pattern="\d{1,3}" placeholder="n"/>
						<span> days before </span><input type="date"/></> }
				{ mode === 'recent' &&
					<>last <input type="text" style={{ width: '3ch' }} pattern="\d{1,3}" placeholder="n"/> days</> }
			</div>
		</>
	);
}

function Selector({ text, options, selected, callback }) {
	return (
		<div className="Selector">
			<span>{text}</span>
			<select value={selected || 'default'} onChange={e => callback(e.target.value)}>
				{!selected ? <option key='default' value='default' disabled>-- none --</option> : ''}
				{options.map(o => <option key={o} value={o}>{o}</option>)}
			</select>
		</div>
	);
}

function Editor() {

}

export default function Corrections({ devices }) {
	const [settings, setSettings] = useState(() =>
		JSON.parse(window.localStorage.getItem('corrSettings')) || {});
	const settingsCallback = key => value => setSettings(state => ({ ...state, [key]: value }));
	useEffect(() => window.localStorage.setItem('corrSettings', JSON.stringify(settings)), [settings]);

	const options = {
		device: Object.keys(devices),
		mode: ['single', 'counters', 'all'],
		field: settings.mode !== 'single' ? null :
			devices[settings.device]?.counters.concat(devices[settings.device]?.fields)
	};
	if (options.field && !options.field.includes(settings.field))
		settings.field = null;
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
				<IntervalInput/>
			</div>
		</div>
	);
}
