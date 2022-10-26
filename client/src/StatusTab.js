import React, { useEffect, useState, useRef } from 'react';
import { useQuery } from 'react-query';
import StatusPane from './StatusPane';

const DEFAULT_LIMIT = 60;

function useInterval(callback, delay) {
	const savedCallback = useRef();

	useEffect(() => {
		savedCallback.current = callback;
	}, [callback]);

	useEffect(() => {
		const id = setInterval(() => {
			savedCallback.current();
		}, delay);
		return () => clearInterval(id);
	}, [delay]);
}

function StatusInfo(props) {
	const [ago, setAgo] = useState('just now');
	useInterval(() => {
		const sec = Math.floor((Date.now() - props.dataUpdatedAt) / 1000);
		setAgo(sec > 0 ? `${sec} second${sec===1?'':'s'} ago` : 'just now');
	}, 1000);

	let text = '';
	if (props.isLoading)
		text = 'Loading...';
	else if (props.isFetching)
		text = 'Fetching...';
	else if (props.error)
		text = `Error: ${props.error.message}`;
	else
		text = `Updated ${ago}.`;

	return <div className="Info">{text}</div>;
}

export default function StatusTab(props) {
	const query = useQuery('stats', async () => {
		const para = new URLSearchParams({ limit: props.rowLimit || DEFAULT_LIMIT }).toString();
		const resp = await fetch((process.env.REACT_APP_API || '') + 'api/status?' + para);
		const data = await resp.json();
		for (const dev in data) {
			const len = data[dev].rows.length, colLen = data[dev].fields.length, rows = data[dev].rows;
			const cols = Array(colLen).fill().map(_ => Array(len));
			for (let i = 0; i < len; ++i)
				for (let j = 0; j < colLen; ++j)
					cols[j][i] = rows[i][j];
			data[dev].columns = cols;
		}
		console.log('got data', data);
		return data;
	});

	return (
		<>
			<StatusInfo { ...query } />
			{query.data ? Object.keys(query.data).map(id => (
				<StatusPane
					key={id}
					id={id}
					data={query.data[id]}
				/>
			)) : ''}
		</>
	);
}
// <StatusPane id={'stub'} data={{fields: [ "server_time", "time", "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "c10", "c11", "pressure" ], rows: [1], columns: [[1662560940], [1662560880]].concat(Array(14).fill().map(a=>[1]))}}/>
