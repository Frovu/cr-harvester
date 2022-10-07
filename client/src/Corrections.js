import { useEffect, useState } from 'react';
import { useQuery } from 'react-query';

import './css/Corrections.css';
import Editor from './Editor';

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
	const [state, setState] = useState(defaults);
	const submit = m => {
		state.start = m === 'dates'
			? state.start
			: new Date(state.end - 86400000 * state.days);
		callback([state.start, state.end]);
		setState({ ...state });
	};
	const changeMode = newMode => {
		setMode(newMode);
		if (newMode === 'recent')
			state.end = new Date(Math.ceil(Date.now() / 86400000) * 86400000);
		submit(newMode);
	};
	const eventHandler = (what) => (e) => {
		if (e.key === 'Enter')
			return submit(mode);
		const value = e.target[what === 'days' ? 'valueAsNumber' : 'valueAsDate'];
		if (value && !isNaN(value)) {
			state[what] = value;
			submit(mode);
		}
	};
	return (
		<>
			<Selector text="Time:" selected={mode} options={modes} callback={changeMode}/>
			<div>
				{ mode === 'dates' &&
					<>
						<input type="date" defaultValue={dateValue(state.start)} onChange={eventHandler('start')}/>
						<span> to </span>
						<input type="date" defaultValue={dateValue(state.end)} onChange={eventHandler('end')}/>
					</> }
				{ mode === 'interval' &&
					<>
						<input type="number" style={{ width: '6ch' }} min="1" max="999"
							defaultValue={state.days} onKeyDown={eventHandler('days')} onChange={eventHandler('days')}/>
						<span> days before </span>
						<input type="date" defaultValue={dateValue(state.end)} onChange={eventHandler('end')}/>
					</> }
				{ mode === 'recent' &&
					<>
						<span>last </span>
						<input type="number" style={{ width: '6ch' }} min="1" max="999"
							defaultValue={state.days} onKeyDown={eventHandler('days')} onChange={eventHandler('days')}/>
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

const epoch = date => date && Math.floor(date.getTime() / 1000);

function EditorWrapper({ device, fields, interval }) {
	const query = useQuery(['editor', device, interval], async () => {
		const resp = await fetch(process.env.REACT_APP_API + '/data?' + new URLSearchParams({
			from: epoch(interval[0]),
			to: epoch(interval[1]),
			dev: device
		}).toString());
		if (resp.status === 404)
			throw new Error('DEVICE NOT FOUND');
		if (resp.status === 400)
			throw new Error('BAD REQUEST');
		const data = await resp.json();
		const len = data.rows.length, colLen = data.fields.length, rows = data.rows;
		const cols = Array(colLen).fill().map(_ => Array(len));
		for (let i = 0; i < len; ++i)
			for (let j = 0; j < colLen; ++j)
				cols[j][i] = rows[i][j];
		data.columns = cols;
		return data;
	});

	if (query.isLoading)
		return <div className="Graph">LOADING...</div>;
	if (query.error)
		return <div className="Graph">ERROR<br/>{query.error.message}</div>;
	return <Editor data={query.data} fields={fields}/>;
}

export default function Corrections({ devices }) {
	const dateDefaults = DEFAULTS();
	const [settings, setSettings] = useState(() => {
		const state = Object.assign({}, JSON.parse(window.localStorage.getItem('corrSettings')));
		state.dates = state.dates ? state.dates.map(d => new Date(d)) : [dateDefaults.start, dateDefaults.end];
		return state;
	});
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

	const fields = settings.mode === 'single'
		? (settings.field && [settings.field])
		: devices[settings.device]?.counters
			.concat(settings.mode === 'all' ? devices[settings.device]?.fields : []);

	return (
		<div className="Corrections">
			<div className="Settings">
				{selectors}
				<IntervalInput callback={settingsCallback('dates')} defaults={settings.dates ?? dateDefaults}/>
			</div>
			{fields && fields.length
				? <EditorWrapper device={settings.device} fields={fields} interval={settings.dates ?? dateDefaults}/>
				: <div style={{ position: 'absolute', top: '45%', left: '50%', transform: 'translate(-50%, -50%)' }}>SELECT CHANNEL</div>}
		</div>
	);
}
