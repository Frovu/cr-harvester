import { useEffect, useState } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import './css/Corrections.css';

const dateStr = date => date?.toISOString().replace(/T.*/, '');
const epoch = date => Math.floor(date.getTime() / 1000);

function Graph({ data,  }) {
	const options = {
		width: 128,
		height: 128,
		legend: { show: false },
		cursor: { show: false },
		padding: [12, 0, 0, 0],
		series: [
			{ },
			{
				points: { show: false },
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
	return <UPlotReact options={options} data={data.columns}/>;
}

export default function Editor({ device, fields, interval }) {
	const query = useQuery('editor', async () => {
		const resp = await fetch(process.env.REACT_APP_API + '/data?' + new URLSearchParams({
			fields: fields,
			from: epoch(interval[0]),
			to: epoch(interval[1]),
			dev: device
		}).toString());
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
		<div className="Graph">
			{query.error ? <>ERROR<br/>{query.error.message}</> : query.isLoading && 'LOADING..'}
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
