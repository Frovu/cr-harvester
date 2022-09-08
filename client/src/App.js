import React from 'react';
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

function StatusPanes(props) {
	const { isLoading, error, data, isFetching } = useQuery('stats', async () => {
		const query = new URLSearchParams({ limit: props.rowLimit || DEFAULT_LIMIT }).toString();
		const resp = await fetch(process.env.REACT_APP_API + '/stations?' + query);
		return await resp.json();
	});

	if (isLoading) return <span>Loading...</span>

	if (error) return <span>Error: {error.message}</span>

	return (
		<>
			<div>{isFetching ? 'Updating...' : ''}</div>
			<div className="App">
				{Object.keys(data).map(id => (<>
					<DevicePane
						id = { id }
						ip={ data[id].ip }
						data={ data[id].data }
					/>
				</>))}
			</div>
		</>
	);
}
