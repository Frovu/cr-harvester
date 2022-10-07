import { useEffect, useLayoutEffect, useState, useRef } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import './css/Corrections.css';

const dateStr = date => date?.toISOString().replace(/T.*/, '');
const epoch = date => Math.floor(date.getTime() / 1000);

const COLORS = {
	voltage: 'yellow',
	temperature_ext: 'cyan', // eslint-disable-line
	temperature: 'cyan',
	pressure: 'magenta',
	default: 'rgb(0,180,130)'
};

const PRECISION = {
	voltage: 2,
	temperature_ext: 1, // eslint-disable-line
	temperature: 1,
	pressure: 1
};

function Graph({ data, size, fields }) {
	const plotData = ['time'].concat(fields).map(f => data.columns[data.fields.indexOf(f)]);
	const css = window.getComputedStyle(document.body);
	const style = {
		bg: css.getPropertyValue('--color-bg'),
		font: (px) => css.font.replace('16px', px+'px'),
		stroke: css.getPropertyValue('--color-text-dark'),
		grid: css.getPropertyValue('--color-border'),
	};
	const options = {
		...size,
		padding: [2, 12, 0, 2],
		cursor: {
			drag: { dist: 12 },
			points: { size: 6, fill: (self, i) => self.series[i]._stroke }
		},
		hooks: {
			init: [
				u => {
					[...u.root.querySelectorAll('.u-legend .u-series')].forEach((el, i) => {
						if (u.series[i]._hide) {
							el.style.display = 'none';
						}
					});
				}
			],
			ready: [u => {
				let clickX, clickY;
				u.over.addEventListener('mousedown', e => {
					clickX = e.clientX;
					clickY = e.clientY;
				});
				u.over.addEventListener('mouseup', e => {
					if (e.clientX === clickX && e.clientY === clickY) {
						const dataIdx = u.cursor.idx;
						if (dataIdx != null)
							console.log(123)
					}
				});
			}]
		},
		series: [
			{ value: '{YYYY}-{MM}-{DD} {HH}:{mm}', stroke: style.stroke },
		].concat(fields.map(f => ({
			label: f,
			show: fields.length <= 1 || !Object.keys(PRECISION).includes(f),
			scale: Object.keys(PRECISION).includes(f) ? f : 'count',
			stroke: COLORS[f] ?? COLORS.default,
			grid: { stroke: style.grid, width: 1 },
			points: { fill: style.bg, stroke: COLORS[f] ?? COLORS.default },
			value: (u, v) => v === null ? '-' : v.toFixed(PRECISION[f] || 0),
		}))),
		axes: ['time'].concat(fields).map((f, i) => ({
			...(f !== 'time' && {
				values: (u, vals) => vals.map(v => v.toFixed(PRECISION[f] || 0)),
				size: 8 + 9 * Math.max.apply(Math,plotData[i]).toFixed(PRECISION[f] || 0).length,
				scale: Object.keys(PRECISION).includes(f) ? f : 'count',
			}),
			show: i <= 1,
			font: style.font(14),
			ticks: { stroke: style.grid, width: 1, size: 2 },
			grid: { stroke: style.grid, width: 1 },
			stroke: style.stroke,
		})),
	};
	return <div style={{ position: 'absolute' }}><UPlotReact options={options} data={plotData}/></div>;
}

export default function Editor({ device, fields, interval }) {
	const graphRef = useRef();
	const [graphSize, setGraphSize] = useState({});
	useLayoutEffect(() => {
		if (!graphRef.current) return;
		const updateSize = () => setGraphSize({
			width: graphRef.current.offsetWidth,
			height: graphRef.current.offsetHeight - 32
		});
		updateSize();
		window.addEventListener('resize', updateSize);
		return () => window.removeEventListener('resize', updateSize);
	}, []);

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

	return (<>
		<div className="Graph" ref={graphRef}>
			{query.error ? <>ERROR<br/>{query.error.message}</> : query.isLoading && 'LOADING...'}
			{query.data && (query.data.rows.length ? <Graph data={query.data} size={graphSize} fields={fields}/> : 'NO DATA')}
		</div>
		<div className="Footer">
			<div style={{ textAlign: 'right' }}>
				<span>{dateStr(interval[0])}</span><br/>
				<span>to {dateStr(interval[1])}</span>
			</div>
			<div>{fields.join()}</div>
		</div>
	</>);
}
