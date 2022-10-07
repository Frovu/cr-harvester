import { useEffect, useLayoutEffect, useState, useRef } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import './css/Corrections.css';

const dateStr = date => date?.toISOString().replace(/T.*/, '');
const epoch = date => Math.floor(date.getTime() / 1000);

function Graph({ data, size }) {
	const options = {
		...size,
		padding: [8, 4, 0, 4],
		series: [
			{ },
			{
				stroke: 'red'
			}
		],
		axes: [
			{
				size: 0
			},
			{
				stroke: 'grey',
			}
		],
	};
	return <div style={{ position: 'absolute' }}><UPlotReact options={options} data={data.columns}/></div>;
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
			{query.data && (query.data.rows.length ? <Graph data={query.data} size={graphSize}/> : 'NO DATA')}
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
