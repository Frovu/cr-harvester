import React from 'react';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import './DevicePane.css';

class StatusPlot extends React.Component {
	constructor(props) {
		super(props);
		this.state = {
			title: props.title,
			options: {
				// title: props.title,
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
						// grid: { stroke: 'grey', width: 1 },
						size: 0
					},
					{
						grid: { stroke: 'grey', width: 1 },
						stroke: 'darkgrey',
						gap: -8,
						size: 28, // TODO: dynamic size
						space: 48,
						values: (u, vals) => vals.map(v => v.toFixed())
					}
				],
			},
			data: [
				[...new Array(32)].map((_, i) => new Date().getTime() - 100000 + i*100),
				[...new Array(32)].map((_, i) => 50 + Math.random() * (10 + Math.random()) * Math.random() * 100)
			]
		};
	}

	render() {
		return (
			<div className="StatusPlot">
				<div className="StatusPlotTitle">{this.state.title}</div>
				<UPlotReact
					options={ this.state.options }
					data={ this.state.data }
				/>
			</div>
		);
	}
}

class DevicePane extends React.Component {
	constructor(props) {
		super(props);
		this.state = {
			id: props.id,
			ip: props.ip,
			data: props.data
		};
	}

	render() {
		return (
			<>
				<StatusPlot title="title0" color="cyan"/>
				<StatusPlot title="title1" color="magenta"/>
				<StatusPlot title="title2" color="yellow"/>
			</>
		);
	}
};

export default DevicePane;
