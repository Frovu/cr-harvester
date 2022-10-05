import { useEffect, useState } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import './css/Corrections.css';

const DEFAULT_INTERVAL = 7; // days
const DEFAULTS = () => ({
	start: new Date(Math.ceil(Date.now() / 86400000 - DEFAULT_INTERVAL) * 86400000),
	end: new Date(Math.ceil(Date.now() / 86400000) * 86400000),
	days: DEFAULT_INTERVAL
});

const dateValue = date => date.toISOString().slice(0, 10);

function IntervalInput({ callback, defaults }) { // returns seconds from epoch
	const modes = ['recent', 'dates', 'interval'];
	const [mode, setMode] = useState(modes[0]);
	const state = {
		start: useState(defaults.start),
		end: useState(defaults.end),
		days: useState(defaults.days)
	};
	const submit = () => {
		const end = mode !== 'recent'
			? state.end[0]
			: new Date(Math.ceil(Date.now() / 86400000) * 86400000);
		const start = mode === 'dates'
			? state.start[0]
			: new Date(end - 86400000 * state.days[0]);
		state.start[1](start);
		state.end[1](end);
		callback([start, end]);
	};
	const eventHandler = (what) => (e) => {
		if (e.key === 'Enter')
			return submit();
		const value = e.target[what === 'days' ? 'valueAsNumber' : 'valueAsDate'];
		if (value && !isNaN(value)) {
			state[what][1](value);
			state[what][0] = value;
			submit();
		}
	};
	return (
		<>
			<Selector text="Time:" selected={mode} options={modes} callback={setMode}/>
			<div>
				{ mode === 'dates' &&
					<>
						<input type="date" defaultValue={dateValue(state.start[0])} onChange={eventHandler('start')}/>
						<span> to </span>
						<input type="date" defaultValue={dateValue(state.end[0])} onChange={eventHandler('end')}/>
					</> }
				{ mode === 'interval' &&
					<>
						<input type="number" style={{ width: '6ch' }} min="1" max="999"
							defaultValue={state.days[0]} onKeyDown={eventHandler('days')} onChange={eventHandler('days')}/>
						<span> days before </span>
						<input type="date" defaultValue={dateValue(state.end[0])} onChange={eventHandler('end')}/>
					</> }
				{ mode === 'recent' &&
					<>
						<span>last </span>
						<input type="number" style={{ width: '6ch' }} min="1" max="999"
							defaultValue={state.days[0]} onKeyDown={eventHandler('days')} onChange={eventHandler('days')}/>
						<span> days</span>
					</> }
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
	const dateDefaults = DEFAULTS();
	const [dates, setDates] = useState([dateDefaults.start, dateDefaults.end]);
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
				<IntervalInput callback={setDates} defaults={dateDefaults}/>
			</div>
			<div>
				{dates && dates[0].toISOString().replace(/\..*/, '')}<br/>
				{dates && dates[1].toISOString().replace(/\..*/, '')}
			</div>
		</div>
	);
}
