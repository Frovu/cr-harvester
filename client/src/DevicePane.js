import React from 'react';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import './DevicePane.css';

function StatusPlot(props) {
	const data = [
		[...new Array(32)].map((_, i) => new Date().getTime() - 100000 + i*100),
		[...new Array(32)].map((_, i) => 50 + Math.random() * (10 + Math.random()) * Math.random() * 100)
	];
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
			<UPlotReact options={options} data={data} />
		</div>
	);
}

export default function DevicePane(props) {
	const data = props.data;
	console.log(props.data);
	const online = 'ONLINE';
	return (
		<div className="DevicePane">
			<div className="DeviceStatus">
				<h3>{props.id}</h3>
				[ {online ? 'ONLINE' : 'LOST'} ]<br/>
				IP: {data.ip || 'N/A'}
			</div>
			<div className="StatusPlots">
				{data.fields.map((f, i) => (
					<StatusPlot color="red" title={i}/>
				))}
			</div>
		</div>
	);
};;
