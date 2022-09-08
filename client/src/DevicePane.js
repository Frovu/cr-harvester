import React from 'react';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import './DevicePane.css';

function StatusPlot(props) {
	const options = {
		width: 128,
		height: 128,
		legend: { show: false },
		cursor: { show: false },
		series: [
			{
			},
			{
				points: { show: false },
				stroke: props.color
			}
		],
		axes: [
			{
				// grid: { stroke: 'gray', width: 1 },
				// ticks: { stroke: 'gray', width: 1 },
				size: 0
			},
			{
				grid: { stroke: 'gray', width: 1 },
				stroke: 'darkgrey',
				gap: -8,
				size: 28, // TODO: dynamic size
				space: 48,
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

const COLORS = {
	voltage: 'darkgray',
	temperature_ext: 'cyan', // eslint-disable-line
	temperature: 'cyan',
	pressure: 'magenta',
	default: 'yellow'
};

export default function DevicePane(props) {
	const online = 'ONLINE';
	const fields = props.data.fields;
	const toDraw = fields.map((f, i) => (
		f.includes('time') || ['info', 'flash_failures'].includes(f) ||
		(f === 'temperature' && fields.includes('temperature_ext')) ? null : i
	)).filter(i => i != null);
	const time = props.data.columns[fields.indexOf('time')];
	return (
		<div className="DevicePane">
			<div className="DeviceStatus">
				<h3>{props.id}</h3>
				[ {online ? 'ONLINE' : 'LOST'} ]<br/>
				IP: {props.data.ip || 'N/A'}
			</div>
			<div className="StatusPlots">
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
