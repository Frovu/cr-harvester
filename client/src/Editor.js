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

function EditorGraph({ size, data, fields, setU, setSelection, zoomTrig }) {
	const shown = fields.length <= 1 ? fields
		: fields.filter(f => !Object.keys(SERIES).includes(f));
	const css = window.getComputedStyle(document.body);
	const style = {
		bg: css.getPropertyValue('--color-bg'),
		font: (px) => css.font.replace('16px', px+'px'),
		stroke: css.getPropertyValue('--color-text-dark'),
		grid: css.getPropertyValue('--color-border'),
	};
	const maxLen = fields.map((f, i) =>
		Math.max.apply(Math, data[i+1]).toFixed(SERIES[f]?.precision ?? 0).length);
	let mouseSelection = false;
	const options = {
		...size,
		padding: [8, 12, 0, 2],
		cursor: {
			lock: true,
			drag: { dist: 12 },
			points: { size: 6, fill: (self, i) => self.series[i]._stroke },
			bind: {
				mousedown: (u, targ, handler) => {
					return e => {
						if (e.button === 0) {
							handler(e);
							if (e.ctrlKey || e.shiftKey) {
								mouseSelection = true;
							}
						}
					};
				},
				mouseup: (u, targ, handler) => {
					return e => {
						if (e.button === 0) {
							if (mouseSelection) {
								const _setScale = u.cursor.drag.setScale, _lock = u.cursor._lock;
								u.cursor.drag.setScale = false;
								handler(e);
								u.cursor.drag.setScale = _setScale;
								u.cursor._lock = _lock;
							}
							else
								handler(e);
						}
					};
				}
			}
		},
		hooks: {
			setSelect: [(u) => {
				if (!mouseSelection)
					return setSelection(null);
				setSelection({
					min: u.posToIdx(u.select.left),
					max: u.posToIdx(u.select.left+u.select.width)
				});
				mouseSelection = false;
			}],
			setScale: [(u, key) => {
				if (key === 'x') {
					setSelection(null);
					zoomTrig({});
				}
			}],
			setSize: [(u) => {
				u.select.height = u.over.offsetHeight;
				u.setSelect(u.select, false);
			}],
			init: [u => {
				console.log('PLOT RENDER');
				[...u.root.querySelectorAll('.u-legend .u-series')].forEach((el, i) => {
					if (u.series[i]._hide) {
						el.style.display = 'none';
					}
				});
			}],
		},
		scales: { x: {  }, _corr: { range: [1, 2] } },
		series: [
			{ value: '{YYYY}-{MM}-{DD} {HH}:{mm}', stroke: style.stroke },
			{
				label: '_corr', _hide: true, scale: '_corr',
				points: { show: true, fill: style.bg, stroke: 'red' }
			}
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
		axes: ['time', 'corr', 'count'].concat(Object.keys(SERIES)).map((f, i) => ( f === 'corr' ? { show: false, scale: '_corr' } : {
			...(f !== 'time' && {
				values: (u, vals) => vals.map(v => v.toFixed(SERIES[f]?.precision ?? 0)),
				size: 8 + 9 * maxLen[i-1],
				scale: Object.keys(SERIES).includes(f) ? f : 'count',
			}),
			show: ['time', 'count'].includes(f),
			font: style.font(14),
			ticks: { stroke: style.grid, width: 1, size: 2 },
			grid: { stroke: style.grid, width: 1 },
			stroke: style.stroke,
		})),
	};
	return <div style={{ position: 'absolute' }}><UPlotReact {...{ options, data, onCreate: setU }}/></div>;
}

function correctSpikes(rows, columns, action, threshold=.3) {
	const interpolate = action === 'interpolate';
	const result = new Map();
	const len = rows.length;
	for (let i = 2, prev, cur = rows[0], next = rows[1]; i < len; ++i) {
		prev = cur; cur = next; next = rows[i];
		for (let j = 0; j < columns.length; ++j) {
			const col = columns[j];
			const vLeft = Math.abs(prev[col] / cur[col] - 1);
			const vRight = Math.abs(next[col] / cur[col] - 1);
			if (vLeft > threshold && vRight > threshold) {
				result.set(cur[0], !interpolate
					? columns.map(c => null)
					: columns.map(c => prev[c] == null || next[c] == null ? null : (prev[c] + next[c]) / 2));
				break;
			}
		}
	}
	return result;
}

function doAction(corr, data, selection, targetFields, cursor, act) {
	const time = data.fields.indexOf('time');
	const newCorr = new Map([...(corr || [])]);
	if (act === 'interpolate') {
		if (selection) return corr; // TODO: implement
		if (cursor < 1 || cursor >= data.rows.length - 1)
			return corr;
		const row = targetFields.map(f => {
			const i = data.fields.indexOf(f);
			const prev = data.rows[cursor-1][i], next = data.rows[cursor+1][i];
			return prev == null || next == null ? null : (prev + next) / 2;
		});
		return newCorr.set(data.rows[cursor][0], row);
	}
	const target = selection ?? { min: cursor, max: cursor };
	const doAct = act === 'remove'
		? (i) => newCorr.set(data.rows[i][time], targetFields.map(f => null))
		: (i) => newCorr.delete(data.rows[i][time]);
	for (let i = target.min; i <= target.max; ++i)
		doAct(i);
	return newCorr;
}

export default function Editor({ data, fields, targetFields, action }) {
	const [u, setU] = useState();
	const [corrections, setCorrections] = useState(null);
	const [threshold, setThreshold] = useState(.3);

	const plotData = useMemo(() => {
		const idx = ['time', '_corr'].concat(fields).map(f => data.fields.indexOf(f));
		const targetIdx = targetFields.map(f => fields.indexOf(f) + 2); // 2 for time and corr
		const columns = Array(fields.length + 2).fill().map(_ => Array(data.rows.length));
		for (let i = 0; i < data.rows.length; ++i) {
			for (let j = 0; j < idx.length; ++j)
				columns[j][i] = data.rows[i][idx[j]] || null;
			const corr = corrections?.get(columns[0][i]);
			if (corr) {
				for (let t = 0; t < targetIdx.length; ++t)
					columns[targetIdx[t]][i] = corr[t];
				columns[1][i] = 1;
			}
		}
		return columns;
	}, [data, corrections, fields, targetFields]);
	useEffect(() => {
		if (!u) return;
		u.setData(plotData, false);
		u.redraw(false, false);
	}, [u, plotData]);
	useEffect(() => {
		if (u) u.setScale('x', { min: u.data[0][0], max: u.data[0][u.data[0].length-1] });
	}, [u, data]);

	const [selection, setSelection] = useState(null);
	useEffect(() => {
		if (!u) return;
		if (selection) {
			const left = u.valToPos(u.data[0][selection.min], 'x');
			u.setSelect({
				width: u.valToPos(u.data[0][selection.max], 'x') - left,
				height: u.over.offsetHeight, top: 0, left
			}, false);
		} else {
			u.setSelect({ width: 0, height: 0 }, false);
		}
	}, [u, selection]);

	const handleCorrection = (key) => {
		if (!u) return;
		if (key === 'D') {
			const target = selection
				? [selection.min, selection.max]
				: [u.valToIdx(u.scales.x.min), u.valToIdx(u.scales.x.max)];
			const columns = targetFields.map(f => data.fields.indexOf(f));
			const corr = correctSpikes(data.rows.slice(...target), columns, action, threshold);
			setCorrections(oldCorr => new Map([...(oldCorr || []), ...corr]));
		} else if (['E', 'Delete', 'R', 'Insert'].includes(key)) {
			if (selection || u.cursor.idx)
				setCorrections(corr =>
					doAction(corr, data, selection, targetFields, u.cursor.idx,
						['R', 'Insert'].includes(key) ? 'restore' : action));
		} else if (key === 'Z') {
			if (selection) {
				setSelection(null);
				u.setScale('x', {
					min: u.data[0][selection.min],
					max: u.data[0][selection.max]
				}, false);
			}
		} else if (key === 'C') {
			// TODO: commit
		} else if (key === 'X') {
			setCorrections(null);
		} else {
			console.log(key);
		}
	};
	const handleRef = useRef(handleCorrection);
	handleRef.current = handleCorrection;
	useEffect(() => {
		if (!u) return;
		const handler = (e) => {
			const moveCur = { ArrowLeft: -1, ArrowRight: 1 }[e.key];
			if (moveCur) {
				const min = u.valToIdx(u.scales.x.min), max = u.valToIdx(u.scales.x.max);
				const cur = u.cursor.idx || (moveCur < 0 ? max : min);
				const move = moveCur * (e.ctrlKey ? Math.ceil((max - min) / 64) : 1)
					* (e.altKey ? Math.ceil((max - min) / 16) : 1);
				setSelection(sel => {
					if (!e.shiftKey) return null;
					const vals = (!sel || !(cur !== sel.min ^ cur !== sel.max))
						? [cur, cur + move]
						: [cur + move, cur !== sel.min ? sel.min : sel.max];
					return vals[0] === vals[1] ? null : {
						min: Math.min(...vals),
						max: Math.max(...vals)
					};
				});
				const idx = Math.min(Math.max(cur + move, min), max);
				u.setCursor({ left: u.valToPos(u.data[0][idx], 'x'), top: u.cursor.top || 0 });
			} else if (e.key === 'Home') {
				u.setCursor({ left: u.valToPos(u.scales.x.min, 'x'), top: u.cursor.top || 0 });
			} else if (e.key === 'End') {
				u.setCursor({ left: u.valToPos(u.scales.x.max, 'x'), top: u.cursor.top || 0 });
			} else if (e.key === 'Escape') {
				u.setScale('x', { min: u.data[0][0], max: u.data[0][u.data[0].length-1] }, false);
				setSelection(null);
			} else {
				handleRef.current(e.code.replace('Key', ''));
			}
		};
		window.addEventListener('keydown', handler);
		return () => window.removeEventListener('keydown', handler);
	}, [u]);

	const [, zoomTrig] = useState();
	const graphRef = useRef();
	const [graphSize, setGraphSize] = useState({});
	useLayoutEffect(() => {
		if (!graphRef.current) return;
		const updateSize = () => setGraphSize({
			width: graphRef.current.offsetWidth,
			height: graphRef.current.offsetHeight - 32
		});
		updateSize();
		const observer = new ResizeObserver(updateSize);
		observer.observe(graphRef.current);
		return () => observer.disconnect();
	}, []);
	useEffect(() => {
		if (u) u.setSize(graphSize);
	}, [u, graphSize]);
	const graph = useMemo(() => (
		<EditorGraph {...{ size: graphSize, data: plotData, fields, setU, selection, zoomTrig, setSelection }}/>
	), [fields]); // eslint-disable-line
	const interv = [0, plotData[0].length - 1].map(i => new Date(plotData[0][i]*1000)?.toISOString().replace(/T.*/, ''));
	return (<>
		<div className="Graph" ref={graphRef}>
			{graph}
		</div>
		<div className="Footer">
			<div style={{ flexShrink: 0, textAlign: 'right', position: 'relative' }}>
				<p style={{ margin: 0 }}>{interv[0]}</p>to {interv[1]}
				{u && (u.scales.x.min !== plotData[0][0] || u.scales.x.max !== plotData[0][plotData[0].length - 1])
					&& <div style={{
						backgroundColor: 'rgba(0,0,0,.7)', fontSize: '16px', gap: '1em',
						position: 'absolute', top: 0, width: '100%', height: '100%',
						display: 'flex', alignItems: 'center', justifyContent: 'center'
					}}>(in zoom)</div>}
			</div>
			<div style={{ flexShrink: 0, textAlign: 'left' }} >
				{corrections?.size > 0 && <span style={{ color: 'red' }}>[{corrections.size}]</span>}
				<br/>
				{selection && <>({selection.max-selection.min})</>}
			</div>
			<div style={{ flex: 1 }}></div>
			{action === 'interpolate' && <div style={{ color: 'var(--color-text)', paddingBottom: '4px' }}>
				<span>Threshold: </span>
				<input type="number" style={{ width: '8ch' }} min=".05" max="10" step=".01"
					defaultValue={threshold} onChange={e => setThreshold(e.target.value)}/>
			</div>}
			<Keybinds handle={handleRef}/>
		</div>
	</>);
}

function Keybinds({ handle }) {
	const keys = {
		E: 'Action',
		R: 'Restore',
		Z: 'Zoom',
		Y: 'Despike',
		X: 'Discard',
		C: 'Commit',
	};
	return (
		<div className="Keybinds">
			{Object.keys(keys).map(k => (
				<div key={k} className="Keybind" onClick={() => handle.current(k)}><div className="Key">{k}</div>{keys[k]}</div>
			))}
		</div>
	);
}
