import { useEffect, useState, useMemo } from 'react';
import { useQuery } from 'react-query';

import './css/Corrections.css';
import Editor from './Editor';

const DAY_MS = 86400000;
const DEFAULT_INTERVAL = 7; // days

const dateValue = date => date && date.toISOString().slice(0, 10);
const ceilEpoch = date => date && Math.ceil(date / DAY_MS) * DAY_MS;
const TIME_MODES = ['recent', 'dates', 'interval'];

function IntervalInput({ callback, defaults }) { // returns seconds from epoch
	const [mode, setMode] = useState(() => (
		ceilEpoch(Date.now()) === ceilEpoch(defaults[1]) ? 'recent' : 'interval'
	));
	const [state, setState] = useState({ start: defaults[0], end: defaults[1] });

	const submit = () => callback([state.start, state.end]);

	const changeMode = newMode => {
		setMode(newMode);
		if (newMode === 'recent')
			setState(st => ({ ...st, end: new Date(ceilEpoch(Date.now())) }));
		if (newMode === 'dates')
			setState(st => ({ ...st, end: new Date(ceilEpoch(Date.now())) }));
	};
	const changeValue = (what) => (e) => {
		const value = e.target[what === 'days' ? 'valueAsNumber' : 'valueAsDate'];
		if (value && !isNaN(value)) {
			setState(st => ({ ...st,
				...(what === 'days'
					? { start: new Date(st.end - value * DAY_MS) }
					: { [what]: value }) }));
		}
	};
	const inputs = {
		start:
			<input type="date" defaultValue={dateValue(state.start)} onChange={changeValue('start')}/>,
		end:
			<input type="date" defaultValue={dateValue(state.end)} onChange={changeValue('end')}/>,
		days:
			<input type="number" style={{ width: '6ch' }}
				min="1" max="999" defaultValue={Math.ceil((state.end - state.start) / DAY_MS)}
				onKeyDown={e => e.key === 'Enter' && submit()} onChange={changeValue('days')}/>
	};
	return (
		<>
			<Selector text="Time:" selected={mode} options={TIME_MODES} callback={changeMode}/>
			<div>
				{ mode === 'dates' &&
					<>
						{inputs.start}
						<span> to </span>
						{inputs.end}
					</> }
				{ mode === 'interval' &&
					<>
						{inputs.days}
						<span> days before </span>
						{inputs.end}
					</> }
				{ mode === 'recent' &&
					<>
						<span>last </span>
						{inputs.days}
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

function EditorWrapper({ device, fields, targetFields, interval, action }) {
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
		return data;
	});

	if (query.isLoading)
		return <div className="Graph">LOADING...</div>;
	if (query.error)
		return <div className="Graph">ERROR<br/>{query.error.message}</div>;
	if (!query.data?.rows?.length)
		return <div className="Graph">NO DATA</div>;
	return <Editor {...{ data: query.data, fields, targetFields, action }}data={query.data} fields={fields} targetFields={targetFields}/>;
}

export default function Corrections({ devices }) {
	const [settings, setSettings] = useState(() => {
		const state = JSON.parse(window.localStorage.getItem('corrSettings'));
		state.dates = state.dates && state.dates.map(d => new Date(d));
		if (!state.dates || state.dates[0] >= state.dates[1])
			state.dates = [
				new Date(ceilEpoch(Date.now()) - DEFAULT_INTERVAL * DAY_MS),
				new Date(ceilEpoch(Date.now()))
			];
		return state;
	});
	const settingsCallback = key => value => setSettings(state => ({ ...state, [key]: value }));
	useEffect(() => window.localStorage.setItem('corrSettings', JSON.stringify(settings)), [settings]);
	const options = {
		device: Object.keys(devices),
		mode: ['single', 'counters', 'all'],
		field: settings.mode !== 'single' ? null :
			devices[settings.device]?.counters.concat(devices[settings.device]?.fields),
		action: ['remove', 'interpolate']
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
	const fields = useMemo(() => (
		devices[settings.device]?.counters.concat(devices[settings.device]?.fields)
	), [settings.device, devices]);
	const targetFields = useMemo(() => (
		settings.mode === 'single'
			? (settings.field && [settings.field])
			: devices[settings.device]?.counters
				.concat(settings.mode === 'all' ? devices[settings.device]?.fields : [])
	), [settings, devices]);

	return (
		<div className="Corrections">
			<div className="Settings">
				{selectors}
				<IntervalInput callback={settingsCallback('dates')} defaults={settings.dates}/>
			</div>
			{targetFields && targetFields.length && settings.action
				? <EditorWrapper {...{
					device: settings.device, fields, targetFields, interval: settings.dates, action: settings.action
				}}/>
				: <div style={{ position: 'absolute', top: '45%', left: '50%', transform: 'translate(-50%, -50%)' }}>INSUFFICIENT PARAMS</div>}
		</div>
	);
}
