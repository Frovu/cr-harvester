import React, { useEffect, useState, useRef } from 'react';
import './App.css';
import DevicePane from './DevicePane';
import { QueryClient, QueryClientProvider, useQuery } from 'react-query';

const queryClient = new QueryClient();
const DEFAULT_LIMIT = 60;

export default function App() {
	return (
		<QueryClientProvider client={ queryClient }>
			<StatusPanes />
		</QueryClientProvider>
	);
};

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

function StatusPanes(props) {
	const query = useQuery('stats', async () => {
		const para = new URLSearchParams({ limit: props.rowLimit || DEFAULT_LIMIT }).toString();
		const resp = await fetch(process.env.REACT_APP_API + '/stations?' + para);
		const data = await resp.json();
		console.log('got data', data);
		return data;
	});
	const data = query.data;

	return (
		<>
			<StatusInfo { ...query } />
			<div className="App">
				{data ? Object.keys(data).map(id => (
					<DevicePane
						id = { id }
						data = { data[id] }
					/>
				)) : ''}
			</div>
		</>
	);
}
