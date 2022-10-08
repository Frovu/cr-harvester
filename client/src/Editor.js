import { useEffect, useLayoutEffect, useState, useRef, useMemo } from 'react';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import './css/Corrections.css';

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

function EditorGraph({ size, data, fields, setU }) {
	const [zoom, setZoom] = useState({});
	const [shown, setShown] = useState(fields.length <= 1 ? fields
		: fields.filter(f => !Object.keys(SERIES).includes(f)));
	const css = window.getComputedStyle(document.body);
	const style = {
		bg: css.getPropertyValue('--color-bg'),
		font: (px) => css.font.replace('16px', px+'px'),
		stroke: css.getPropertyValue('--color-text-dark'),
		grid: css.getPropertyValue('--color-border'),
	};
	const maxLen = fields.map((f, i) =>
		Math.max.apply(Math, data[i+1]).toFixed(SERIES[f]?.precision ?? 0).length);

	const options = {
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
	return <div style={{ position: 'absolute' }}><UPlotReact {...{ options, data, onCreate: setU }}/></div>;
}

export default function Editor({ data: rawData, fields }) {
	const data = useMemo(() => (
		['time'].concat(fields).map(f => rawData.columns[rawData.fields.indexOf(f)])
	), [rawData, fields]);

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

	const [u, setU] = useState();
	const [selection, setSelection] = useState(null);
	useEffect(() => {
		if (!u) return;
		if (selection) {
			const left = u.valToPos(u.data[0][selection.min], 'x');
			u.setSelect({
				width: u.valToPos(u.data[0][selection.max], 'x') - left,
				height: u.over.offsetHeight, top: 0, left
			});
		} else {
			u.setSelect({ width: 0, height: 0 });
		}
	}, [u, selection]);
	useEffect(() => {
		if (!u) return;
		const handler = (e) => {
			const moveCur = { ArrowLeft: -1, ArrowRight: 1 }[e.key];
			if (moveCur) {
				const min = u.valToIdx(u.scales.x.min), max = u.valToIdx(u.scales.x.max);
				const cur = u.cursor.idx || (moveCur < 0 ? max : min);
				const move = Math.floor(moveCur * (e.ctrlKey ? (max-min) / 100 : 1) * (e.altKey ? (max-min) / 10 : 1));
				const idx = Math.min(Math.max(cur + move, min), max);
				setSelection(sel => {
					if (!e.shiftKey) return null;
					if (!sel || !(cur !== sel.min ^ cur !== sel.max)) {
						return {
							min: Math.min(cur, cur + move),
							max: Math.max(cur, cur + move)
						};
					} else {
						const newSel = { ...sel };
						newSel[cur === sel.min ? 'min' : 'max'] += move;
						return(newSel);
					}
				});
				u.setCursor({ left: u.valToPos(u.data[0][idx], 'x'), top: u.cursor.top || 0 });
			} else if (e.key === 'Home') {
				u.setCursor({ left: u.valToPos(u.scales.x.min, 'x'), top: u.cursor.top || 0 });
			} else if (e.key === 'End') {
				u.setCursor({ left: u.valToPos(u.scales.x.max, 'x'), top: u.cursor.top || 0 });
			} else if (e.key === 'Escape') {
				u.setScale('x', { min: u.data[0][0], max: u.data[0][u.data[0].length-1] }, true);
				setSelection(null);
			} else {
				console.log(e.key);
			}
		};
		window.addEventListener('keydown', handler);
		return () => window.removeEventListener('keydown', handler);
	}, [u]);

	const graph = useMemo(() => (
		<EditorGraph {...{ size: graphSize, data, fields, setU }}/>
	), [graphSize, data, fields]);

	const interv = [0, data[0].length - 1].map(i => new Date(data[0][i]*1000)?.toISOString().replace(/T.*/, ''));
	console.log(u.scales.x.min !== data[0][0] || u.scales.x.max !== data[0][data[0].length - 1])
	return (<>
		<div className="Graph" ref={graphRef}>
			{graph}
		</div>
		<div className="Footer">
			<div style={{ textAlign: 'right', position: 'relative' }}>
				{interv[0]}<br/>to {interv[1]}
				{u && (u.scales.x.min !== data[0][0] || u.scales.x.max !== data[0][data[0].length - 1])
					&& <div style={{
						backgroundColor: 'rgba(0,0,0,.7)', fontSize: '16px', gap: '1em',
						position: 'absolute', top: 0, width: '100%', height: '100%',
						display: 'flex', alignItems: 'center', justifyContent: 'center'
					}}>(in zoom)</div>}
			</div>
			{selection && <>({selection.min}, {selection.max}) [{selection.max-selection.min}]</>}
		</div>
	</>);
}
