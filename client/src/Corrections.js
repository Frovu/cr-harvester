import React, { useEffect, useState, useMemo, useCallback, useContext } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import './css/Corrections.css';
import Editor from './Editor';

const DAY_MS = 86400000;
const DEFAULT_INTERVAL = 7; // days

const epoch = date => date && Math.floor(date.getTime() / 1000);
const dateValue = date => date && date.toISOString().slice(0, 10);
const ceilEpoch = date => date && Math.ceil(date / DAY_MS) * DAY_MS;
const TIME_MODES = ['recent', 'dates', 'interval'];

function IntervalInput({ callback, defaults }) {
	const [state, setState] = useState({ start: defaults[0], end: defaults[1] });
	const [mode, setMode] = useState(() => (
		ceilEpoch(Date.now()) === ceilEpoch(defaults[1]) ? 'recent' : 'interval'
	));
	const days = Math.ceil((state.end - state.start) / DAY_MS);
	const changeValue = (what) => (e) => {
		const value = e.target[what === 'days' ? 'valueAsNumber' : 'valueAsDate'];
		if (value && !isNaN(value)) {
			const newDays = what === 'days' ? value : days;
			const newState = { ...state, ...(what !== 'days' && { [what]: value }) };
			if (mode === 'recent')
				newState.end = new Date(ceilEpoch(Date.now()));
			if (mode !== 'dates')
				newState.start = new Date(newState.end - newDays * DAY_MS);
			setState(newState);
			callback([newState.start, newState.end]);
		}
	};
	const inputs = {
		start:
			<input type="date" defaultValue={dateValue(state.start)} onChange={changeValue('start')}/>,
		end:
			<input type="date" defaultValue={dateValue(state.end)} onChange={changeValue('end')}/>,
		days:
			<input type="number" style={{ width: '6ch' }}
				min="1" max="999" defaultValue={days} onChange={changeValue('days')}/>
	};
	return (<>
		<Selector text="Time:" selected={mode} options={TIME_MODES} callback={setMode}/>
		<div>
			{ mode === 'dates' &&
				<>{inputs.start} <span> to </span> {inputs.end} </> }
			{ mode === 'interval' &&
				<>{inputs.days} <span> days before </span> {inputs.end} </> }
			{ mode === 'recent' &&
				<> <span>last </span> {inputs.days} <span> days</span> </> }
		</div>
	</>);
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

const EditorContext = React.createContext();

async function mutateCorrections(action, body) {
	const res = await fetch(process.env.REACT_APP_API + '/corrections', {
		method: action,
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(body)
	});
	if (res.status === 200)
		return null;
	if (res.status === 404)
		return `unknown device: ${body.device}`;
	if (res.status === 401)
		return 'wrong secret key';
	if (res.status === 400)
		return 'bad request';
	return 'HTTP ' + res.status;
}

function CorrectionsWrapper({ data }) {
	const queryClient = useQueryClient();
	const { device, secret, targetFields: fields } = useContext(EditorContext);
	const [report, setReport] = useState();
	const mutationCallbacks = {
		onError: (error) => setReport(`Error: ${error}`),
		onSuccess: () => queryClient.invalidateQueries(['editor'])
	};
	const correctMutation = useMutation(async (corrections) => {
		const error = mutateCorrections('POST', { device, secret, fields, corrections });
		if (error) setReport(`Error: ${error}`);
	}, mutationCallbacks);
	const eraseMutation = useMutation(async (from, to) => {
		const error = mutateCorrections('POST', { device, secret, fields, from, to });
		if (error) setReport(`Error: ${error}`);
	}, mutationCallbacks);

	const commitCorrection = useCallback((corrections) => {

	}, [correctMutation]);
	const commitErase = useCallback((corrections) => {

	}, [eraseMutation]);

	return (<>

		<Editor {...{ data, commitCorrection, commitErase }}/>
	</>);
}

function EditorWrapper() {
	const { device, interval } = useContext(EditorContext);
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
	return <CorrectionsWrapper data={query.data}/>;
}

export default function Corrections({ devices, secret }) {
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
	), [settings.mode, settings.device, settings.field, devices]);

	const [debouncedDates, setDebouncedDates] = useState(settings.dates);
	useEffect(() => {
		const timeout = setTimeout(() => setDebouncedDates(settings.dates), 500);
		return () => clearTimeout(timeout);
	}, [settings.dates]);

	const context = {
		device: settings.device,
		action: settings.action,
		interval: debouncedDates,
		fields, targetFields, secret,
	};
	return (
		<div className="Corrections">
			<div className="Settings">
				{selectors}
				<IntervalInput callback={settingsCallback('dates')} defaults={settings.dates} throttle={500}/>
			</div>
			<EditorContext.Provider value={context}>
				{targetFields && targetFields.length && settings.action
					? <EditorWrapper/>
					: <div style={{ position: 'absolute', top: '45%', left: '50%', transform: 'translate(-50%, -50%)' }}>INSUFFICIENT PARAMS</div>}
			</EditorContext.Provider>
		</div>
	);
}
