import { useEffect, useState } from 'react';
import { useQuery } from 'react-query';

import { Selector, IntervalInput } from './Corrections';

import './css/Corrections.css';

const DAY_MS = 86400000;
const DEFAULT_INTERVAL = 7; // days

const RESOLUTION = {
	'1 minute': 60,
	'10 minutes': 600,
	'1 hour': 3600,
	'1 day': 86400
};

function plotOptions() {

}

function DataPlot() {

	return null;
}

function DataWrapper({ station, set, interval, resolution }) {
	const query = useQuery(['data', station, interval, resolution], async () => {
		const resp = await fetch((process.env.REACT_APP_API || '') + 'api/data/product?' + new URLSearchParams({
			from: Math.floor(interval[0].getTime() / 1000),
			to: Math.floor(interval[1].getTime() / 1000),
			period: RESOLUTION[resolution],
			station,
			set
		}).toString());
		if (resp.status === 404)
			throw new Error('STATION NOT FOUND');
		if (resp.status === 400)
			throw new Error('BAD REQUEST');
		const data = await resp.json();
		console.log('got data:', data);
		return data;
	});

	if (query.isLoading)
		return <div className="Graph">LOADING...</div>;
	if (query.error)
		return <div className="Graph">ERROR<br/>{query.error.message}</div>;
	if (!query.data?.rows?.length)
		return <div className="Graph">NO DATA</div>;
	return <DataPlot data={query.data}/>;
}

export default function DataTab({ stations }) {
	const [settings, setSettings] = useState(() => {
		const state = JSON.parse(window.localStorage.getItem('harvesterDataSettings')) || {
			resolution: '1 hour',
			set: 'raw'
		};
		state.dates = state.dates && state.dates.map(d => new Date(d));
		if (!state.dates || state.dates[0] >= state.dates[1])
			state.dates = [
				new Date(Math.ceil(Date.now() / DAY_MS) * DAY_MS - DEFAULT_INTERVAL * DAY_MS),
				new Date(Math.ceil(Date.now() / DAY_MS) * DAY_MS)
			];
		return state;
	});
	const settingsCallback = key => value => setSettings(state => ({ ...state, [key]: value }));
	useEffect(() => window.localStorage.setItem('harvesterCorrSettings', JSON.stringify(settings)), [settings]);
	const options = {
		station: Object.keys(stations),
		set: ['raw'].concat(Object.keys(stations[settings.station]?.datasets ?? {})),
		resolution: Object.keys(RESOLUTION)
	};
	if (!options.set.includes(settings.set))
		settings.set = null;

	const selectors = Object.keys(options).filter(k => options[k]).map(key =>
		<Selector
			key={key}
			text={key.charAt(0).toUpperCase()+key.slice(1)+':'}
			options={options[key]}
			selected={settings[key]}
			callback={settingsCallback(key)}
		/>
	);

	const [debouncedDates, setDebouncedDates] = useState(settings.dates);
	useEffect(() => {
		const timeout = setTimeout(() => setDebouncedDates(settings.dates), 500);
		return () => clearTimeout(timeout);
	}, [settings.dates]);

	return (
		<div className="Corrections">
			<div className="Settings">
				{selectors}
				<IntervalInput callback={settingsCallback('dates')} defaults={settings.dates} throttle={500}/>
			</div>
			{settings.station && settings.set
				? <DataWrapper station={stations[settings.station]} interval={debouncedDates} set={settings.set} resolution={settings.resolution}/>
				: <div style={{ position: 'absolute', top: '45%', left: '50%', transform: 'translate(-50%, -50%)' }}>INSUFFICIENT PARAMS</div>}
		</div>
	);
}