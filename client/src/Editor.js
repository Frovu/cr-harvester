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

function EditorGraph({ size, data, fields, setU, setSelection, zoom, setZoom, shown, setShown }) {
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
					const scale = u.scales[key];
					setZoom({ min: scale.min, max: scale.max });
				}
			}],
			setSeries: [(u, i, opts) => {
				setShown(st => opts.show ? st.concat(fields[i-2]) : st.filter(f => f !== fields[i-2]));
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
		scales: { x: { ...zoom }, _corr: { range: [1, 2] } },
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

function correctSpikes(rows, columns, interpolate=false, threshold=.3) {
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

export default function Editor({ data, fields, targetFields, action }) {
	const [u, setU] = useState();
	const [corrections, setCorrections] = useState(null);

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

	const handleCorrection = useMemo(() => (e) => {
		if (e.key === 'j') {
			const target = selection
				? [selection.min, selection.max]
				: [u.valToIdx(u.scales.x.min), u.valToIdx(u.scales.x.max)];
			const columns = targetFields.map(f => data.fields.indexOf(f)); // FIXME
			const corr = correctSpikes(data.rows.slice(...target), columns, action === 'interpolate');
			setCorrections(oldCorr => new Map([...(oldCorr || []), ...corr]));
		} else if (e.key === 'e') {
			const idx = u.cursor.idx;
			if (idx && idx > 0 && idx < data.rows.length - 1) {
				const corr = action === 'remove' ? targetFields.map(f => null)
					: targetFields.map(f => {
						const i = data.fields.indexOf(f);
						const prev = data.rows[idx-1][i], next = data.rows[idx+1][i];
						return prev == null || next == null ? null : (prev + next) / 2;
					});
				setCorrections(oldCorr => new Map([...(oldCorr || []), [data.rows[idx][0], corr]]));
			}
		} else if (e.key === 'u') {
			setCorrections(null);
		} else {
			console.log(e.key);
		}

	}, [u, action, data, selection, targetFields]);
	const handleRef = useRef(handleCorrection);
	useEffect(() => { handleRef.current = handleCorrection; }, [handleCorrection]);
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
				handleRef.current(e);
			}
		};
		window.addEventListener('keydown', handler);
		return () => window.removeEventListener('keydown', handler);
	}, [u]);

	const graphRef = useRef();
	const [graphSize, setGraphSize] = useState({});
	const [zoom, setZoom] = useState({});
	const [shown, setShown] = useState(() => fields.length <= 1 ? fields
		: fields.filter(f => !Object.keys(SERIES).includes(f)));
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
		<EditorGraph {...{ size: graphSize, data: plotData, fields, setU, selection, setSelection, zoom, setZoom, shown, setShown }}/>
	), [fields]); // eslint-disable-line
	const interv = [0, plotData[0].length - 1].map(i => new Date(plotData[0][i]*1000)?.toISOString().replace(/T.*/, ''));
	return (<>
		<div className="Graph" ref={graphRef}>
			{graph}
		</div>
		<div className="Footer">
			<div style={{ flexShrink: 0, textAlign: 'right', position: 'relative' }}>
				<p style={{ margin: 0 }}>{interv[0]}</p>to {interv[1]}
				{u && (zoom.min !== plotData[0][0] || zoom.max !== plotData[0][plotData[0].length - 1])
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
			<Keybinds/>
		</div>
	</>);
}

function Keybinds() {
	const keys = {
		'←/→': 'Move cursor',
		'Ctrl/Alt+←/→': 'Move faster',
		'Shift+←/→': 'Select',
		E: 'Action',
		J: 'Despike',
		U: 'Discard',
		C: 'Commit',
	};
	return (
		<div className="Keybinds">
			{Object.keys(keys).map(k => (
				<div key={k} className="Keybind"><div className="Key">{k}</div>{keys[k]}</div>
			))}
		</div>
	);
}
