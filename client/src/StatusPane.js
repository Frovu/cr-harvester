import React from 'react';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import './css/StatusTab.css';

function StatusPlot(props) {
	const max = Math.max.apply(Math, props.data[1]);
	const options = {
		width: 128,
		height: 128,
		legend: { show: false },
		cursor: { show: false },
		padding: [12, 0, 0, 0],
		series: [
			{ },
			{
				points: { show: false },
				stroke: props.color
			}
		],
		axes: [
			{
				size: 0
			},
			{
				stroke: 'grey',
				gap: -7,
				size: 3 + 7 * max.toFixed().length,
				splits: u => [
					u.series[1].min + (u.series[1].max - u.series[1].min) / 6,
					u.series[1].max - (u.series[1].max - u.series[1].min) / 5
				],
				values: (u, vals) => vals.map(v => v.toFixed())
			}
		],
	};
	return (
		<div className="StatusPlot">
			<div className="StatusPlotTitle">{props.title}</div>
			<UPlotReact options={options} data={props.data} />
		</div>
	);
}

function intervalToString(seconds) {
	const days = Math.floor(seconds / 86400);
	const hours = Math.floor(seconds / 3600);
	const minutes = Math.floor(seconds / 60);
	const n = days || hours || minutes || Math.floor(seconds);
	return `${n} ${days ? 'day' : hours ? 'hour' : minutes ? 'minute' : 'second'}${n!==1?'s':''}`;
}

const COLORS = {
	voltage: 'yellow',
	temperature_ext: 'cyan', // eslint-disable-line
	temperature: 'cyan',
	pressure: 'magenta',
	default: 'rgb(215,90,60)'
};

export default function DevicePane(props) {
	if (!props.data.rows.length)
		return;
	const period = 60; // FIXME
	const fields = props.data.fields;
	const toDraw = fields.map((f, i) => (
		f.includes('time') || ['info', 'flash_failures'].includes(f) ||
		(f === 'temperature' && fields.includes('temperature_ext')) ? null : i
	)).filter(i => i != null);
	const time = props.data.columns[fields.indexOf('time')];
	const uptimes = props.data.columns[fields.indexOf('uptime')];
	const uptime = uptimes[uptimes.length-1] * period;
	const online = Date.now()/1000 - time[time.length-1] < 3 * period;
	const colAvailable = Math.floor((document.body.offsetWidth - 240) / 136);
	const columns = toDraw.length <= colAvailable ? toDraw.length : Math.min(Math.ceil(toDraw.length / 2), colAvailable);
	return (
		<div className="DevicePane">
			<div className="DeviceStatus">
				<p>
					<u><b>{props.id}</b></u><br/>
				</p>
				<p style={{ color: online ? 'rgb(0,255,0)' : 'red' }}>
					[{online ? 'ONLINE' : 'LOST'}]
				</p>
				<p style={{ fontSize: '12px' }}>
					{new Date(time[time.length-1]*1000).toISOString().replace(/\..*/,'').replace('T',' ')}<br/>
					UPT: {uptime ? intervalToString(uptime) : 'N/A'}<br/>
					IP: {props.data.ip || 'N/A'}
				</p>
			</div>
			<div className="StatusPlots" style={{ gridTemplateColumns: `repeat(${columns}, 136px)`, opacity: online ? 1 : .5 }}>
				{toDraw.map(i => [i, fields[i]]).map(([i, f]) => (
					<StatusPlot
						key={f} title={f.split('_')[0]}
						data={[ time, props.data.columns[i] ]}
						color={COLORS[f]||COLORS.default}/>
				))}
			</div>
		</div>
	);
};;
