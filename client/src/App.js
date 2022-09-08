import React, { useEffect, useState } from 'react';
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

function StatusInfo(props) {
	const [ago, setAgo] = useState('just now');
	useEffect(() => {
		if (!props.data) return;
		const timeout = setInterval(() => {
			const sec = Math.floor((Date.now() - props.dataUpdatedAt) / 1000);
			setAgo(`${sec} second${sec===1?'':'s'} ago`);
		}, 1000);
		return () => {
			console.log('clear')
			setInterval(timeout);
		};
	}, [props.data, props.dataUpdatedAt]);

	let text = '';
	if (props.isLoading)
		text = 'Loading...';
	else if (props.isFetching)
		text = 'Fetching...';
	else if (props.error)
		text = `Error: ${props.error.message}`;
	else
		text = `Updated ${ago}.`;

	return <div className="Info">{text}</div>
}

function StatusPanes(props) {
	const query = useQuery('stats', async () => {
		const para = new URLSearchParams({ limit: props.rowLimit || DEFAULT_LIMIT }).toString();
		const resp = await fetch(process.env.REACT_APP_API + '/stations?' + para);
		return await resp.json();
	});
	const data = query.data;
	return (
		<>
			<StatusInfo { ...query } />
			<div className="App">
				{data ? Object.keys(data).map(id => (
					<DevicePane
						id = { id }
						ip = { data[id].ip }
						data = { data[id].data }
					/>
				)) : ''}
			</div>
		</>
	);
}
