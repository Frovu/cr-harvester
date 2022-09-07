import React from 'react';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';

class StatusPlot extends React.Component {
	constructor(props) {
		super(props);
		this.state = {
			options: {
				title: "Chart",
				width: 400,
				height: 300,
				series: [
					{
						label: "Date"
					},
					{
						label: "",
						points: { show: false },
						stroke: "blue",
						fill: "blue"
					}
				],
				scales: { x: { time: false } },
			},
			data: [
				[...new Array(100000)].map((_, i) => i),
				[...new Array(100000)].map((_, i) => i % 1000)
			]
		};
	}
	render() {
		return (
			<UPlotReact
				key="uplot-div"
				options={this.state.options}
				data={this.state.data}
			/>
		);
	}

}

class DevicePane extends React.Component {
	constructor(props) {
		super(props);
		this.state = {
			active: false,
		};
	}

	render() {
		return (
			<StatusPlot />
		);
	}
};


export default DevicePane;
