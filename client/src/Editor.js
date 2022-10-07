import { useEffect, useLayoutEffect, useState, useRef, useMemo } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import './css/Corrections.css';

const dateStr = date => date?.toISOString().replace(/T.*/, '');
const epoch = date => date && Math.floor(date.getTime() / 1000);

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

const editorSelection = {};

function editorKeydown(u, e) {
	if (!u) return;
	const moveCur = { ArrowLeft: -1, ArrowRight: 1 }[e.key];
	if (moveCur) {
		const min = u.valToIdx(u.scales.x.min), max = u.valToIdx(u.scales.x.max);
		const move = Math.floor(moveCur * (e.ctrlKey ? (max-min) / 100 : 1) * (e.altKey ? (max-min) / 10 : 1));
		const idx = Math.min(Math.max((u.cursor.idx || min) + move, min), max);
		if (e.shiftKey) {
			if (u.cursor.idx === editorSelection.min && u.cursor.idx !== editorSelection.max)
				editorSelection.min += move;
			else if (u.cursor.idx === editorSelection.max && u.cursor.idx !== editorSelection.min)
				editorSelection.max += move;
			else {
				editorSelection.min = Math.min(u.cursor.idx, u.cursor.idx + move);
				editorSelection.max = Math.max(u.cursor.idx, u.cursor.idx + move);
			}
			console.log(editorSelection);
			const left = u.valToPos(u.data[0][editorSelection.min], 'x');
			u.setSelect({
				width: u.valToPos(u.data[0][editorSelection.max], 'x') - left,
				height: u.over.offsetHeight, top: 0, left
			});
		} else {
			u.setSelect({ width: 0, height: 0 });
		}
		u.setCursor({ left: u.valToPos(u.data[0][idx], 'x'), top: u.cursor.top || 0 });
	} else if (e.key === 'Home') {
		u.setCursor({ left: u.valToPos(u.scales.x.min, 'x'), top: u.cursor.top || 0 });
	} else if (e.key === 'End') {
		u.setCursor({ left: u.valToPos(u.scales.x.max, 'x'), top: u.cursor.top || 0 });
	} else if (e.key === 'Escape') {
		u.setScale('x', { min: u.data[0][0], max: u.data[0][u.data[0].length-1] }, true);
	} else {
		console.log(e.key);
	}
}

function Graph({ data, size, fields }) {
	const uRef = useRef();
	useEffect(() => {
		const handler = (e) => editorKeydown(uRef.current, e);
		window.addEventListener('keydown', handler);
		return () => window.removeEventListener('keydown', handler);
	}, []);
	const [zoom, setZoom] = useState({});
	const [shown, setShown] = useState(fields.length <= 1 ? fields
		: fields.filter(f => !Object.keys(SERIES).includes(f)));
	const plotData = ['time'].concat(fields).map(f => data.columns[data.fields.indexOf(f)]);
	const memoOptions = useMemo(() => {
		const css = window.getComputedStyle(document.body);
		const style = {
			bg: css.getPropertyValue('--color-bg'),
			font: (px) => css.font.replace('16px', px+'px'),
			stroke: css.getPropertyValue('--color-text-dark'),
			grid: css.getPropertyValue('--color-border'),
		};
		const maxLen = fields.map((f, i) =>
			Math.max.apply(Math, plotData[i+1]).toFixed(SERIES[f]?.precision ?? 0).length);

		return {
			...size,
			padding: [8, 12, 0, 2],
			cursor: {
				drag: { dist: 12 },
				points: { size: 6, fill: (self, i) => self.series[i]._stroke }
			},
			hooks: {
				setScale: [(u, key) => {
					if (key === 'x') {
						const scale = u.scales[key];
						if (zoom.min !== scale.min || zoom.max !== scale.max)
							setZoom({ min: scale.min, max: scale.max });
					}
				}],
				setSeries: [(u, i, opts) => {
					setShown(opts.show ? shown.concat(fields[i-1]) : shown.filter(f => f !== fields[i-1]));
				}],
				ready: [u => {
					console.log('PLOT RENDER');
					uRef.current = u;
					let clickX, clickY;
					u.over.addEventListener('mousedown', e => {
						clickX = e.clientX;
						clickY = e.clientY;
					});
					u.over.addEventListener('mouseup', e => {
						if (e.clientX === clickX && e.clientY === clickY) {
							const dataIdx = u.cursor.idx;
							if (dataIdx != null)
								console.log(123);
						}
					});
				}],
				destroy: [u => {
					// window.removeEventListener('keydown', keydownHandler(u));
				}]
			},
			scales: { x: { ...zoom } },
			series: [
				{ value: '{YYYY}-{MM}-{DD} {HH}:{mm}', stroke: style.stroke },
			].concat(fields.map((f, i) => ({
				label: (fields.length > 6 ? SERIES[f]?.alias : f) || f,
				show: shown.includes(f),
				scale: Object.keys(SERIES).includes(f) ? f : 'count',
				stroke: SERIES[f]?.color ?? COLOR,
				grid: { stroke: style.grid, width: 1 },
				points: { fill: style.bg, stroke: SERIES[f]?.color ?? COLOR },
				value: (u, v) => v === null ? '-'
					: v.toFixed(SERIES[f]?.precision ?? 0).padEnd(SERIES[f]?.precision ? maxLen[i] : 0, 0)
				// [SERIES[f]?.precision ? 'padEnd' : 'padStart']?.(maxLen[i], '0'),
			}))),
			axes: ['time'].concat(fields).map((f, i) => ({
				...(f !== 'time' && {
					values: (u, vals) => vals.map(v => v.toFixed(SERIES[f]?.precision ?? 0)),
					size: 8 + 9 * maxLen[i-1],
					scale: Object.keys(SERIES).includes(f) ? f : 'count',
				}),
				show: i <= 1,
				font: style.font(14),
				ticks: { stroke: style.grid, width: 1, size: 2 },
				grid: { stroke: style.grid, width: 1 },
				stroke: style.stroke,
			})),
		};
	}, [data, size, fields]); // eslint-disable-line
	return <div style={{ position: 'absolute' }}><UPlotReact options={memoOptions} data={plotData} onCreate={u=>{uRef.current=u;}}/></div>;
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

	useEffect(() => {

	}, []);

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
		</div>
	</>);
}
