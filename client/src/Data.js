import { useEffect, useState } from 'react';
import { useQuery } from 'react-query';

import { Selector, IntervalInput } from './Corrections';
import uPlot from 'uplot/dist/uPlot.esm.js';

import './css/Corrections.css';

const DAY_MS = 86400000;
const DEFAULT_INTERVAL = 7; // days

const RESOLUTION = {
	'1 minute': 60,
	'10 minutes': 600,
	'1 hour': 3600,
	'1 day': 86400
};

const COLOR = 'rgb(0,180,130)';
const SERIES = {
	voltage: {
		color: 'yellow',
		alias: 'v',
		precision: 2
	},
	temperature_ext: { // eslint-disable-line
		color: 'cyan',
		alias: 't_ext',
		precision: 1
	},
	temperature: {
		color: 'cyan',
		alias: 'temp',
		precision: 1
	},
	pressure: {
		color: 'magenta',
		alias: 'pres',
		precision: 1
	}
};

export function plotOptions(data, fields, withCorr=false) {
	const shown = fields.length <= 1 ? fields
		: fields.filter(f => !Object.keys(SERIES).includes(f));
	const maxLen = fields.map((f, i) =>
		Math.max.apply(Math, data[i+2]).toFixed(SERIES[f]?.precision ?? 0).length);
	const css = window.getComputedStyle(document.body);
	const style = {
		bg: css.getPropertyValue('--color-bg'),
		font: (px) => css.font.replace('16px', px+'px'),
		stroke: css.getPropertyValue('--color-text-dark'),
		grid: css.getPropertyValue('--color-border'),
	};
	
	return {
		tzDate: ts => uPlot.tzDate(new Date(ts * 1e3), 'UTC'),
		padding: [8, 12, 0, 2],
		scales: { x: {  }, _corr: { range: [1, 2] } },
		series: [
			{ value: '{YYYY}-{MM}-{DD} {HH}:{mm}', stroke: style.stroke }
		].concat(
			withCorr ? {
				label: '_corr', _hide: true, scale: '_corr',
				points: { show: true, fill: style.bg, stroke: 'red' }
			} : []
		).concat(fields.map((f, i) => ({
			label: (fields.length > 6 ? SERIES[f]?.alias : f) || f,
			show: shown.includes(f),
			scale: Object.keys(SERIES).includes(f) ? f : 'count',
			stroke: SERIES[f]?.color ?? COLOR,
			grid: { stroke: style.grid, width: 1 },
			points: { fill: style.bg, stroke: SERIES[f]?.color ?? COLOR },
			value: (u, v) => v === null ? '-'
				: v.toFixed(SERIES[f]?.precision ?? 0).padEnd(SERIES[f]?.precision ? maxLen[i] : 0, 0)
		}))),
		axes: (withCorr ? ['time', 'corr', 'count'] : ['time', 'count']).concat(Object.keys(SERIES)).map((f, i) => ( f === 'corr' ? { show: false, scale: '_corr' } : {
			...(f !== 'time' && {
				values: (u, vals) => vals.map(v => v.toFixed(SERIES[f]?.precision ?? 0)),
				size: 8 + 12 * maxLen[i-1],
				scale: Object.keys(SERIES).includes(f) ? f : 'count',
			}),
			show: ['time', 'count'].includes(f),
			font: style.font(14),
			ticks: { stroke: style.grid, width: 1, size: 2 },
			grid: { stroke: style.grid, width: 1 },
			stroke: style.stroke,
		})),
	};
}

function DataPlot() {

	return null;
}

function DataWrapper({ station, interval, resolution }) {
	const query = useQuery(['data', station, interval, resolution], async () => {
		const resp = await fetch((process.env.REACT_APP_API || '') + 'api/data/product?' + new URLSearchParams({
			from: Math.floor(interval[0].getTime() / 1000),
			to: Math.floor(interval[1].getTime() / 1000),
			period: RESOLUTION[resolution],
			station
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
			resolution: '1 hour'
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
		resolution: Object.keys(RESOLUTION)
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
			{settings.station
				? <DataWrapper station={settings.station} interval={debouncedDates} resolution={settings.resolution}/>
				: <div style={{ position: 'absolute', top: '45%', left: '50%', transform: 'translate(-50%, -50%)' }}>INSUFFICIENT PARAMS</div>}
		</div>
	);
}