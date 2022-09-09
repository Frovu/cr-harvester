import { useState } from 'react';
import { QueryClient, QueryClientProvider, useQuery } from 'react-query';

import StatusTab from './StatusTab';
import Subscriptions from './Subscriptions';

import './css/App.css';

const queryClient = new QueryClient();

function Menu({ onChange, activeTab }) {
	return (
		<div className="Menu">
			{['Status', 'Corrections', 'Subscriptions'].map(tab => (
				<div className="MenuButton" key={tab}>
					<input type="radio" name="Menu" id={tab} value={tab} checked={activeTab===tab} onChange={onChange}/>
					<label htmlFor={tab}>{tab}</label>
				</div>
			))}
		</div>
	);
}

function App() {
	const [activeTab, setActiveTab] = useState('Status');
	const query = useQuery('stations',
		() => fetch(process.env.REACT_APP_API + '/stations').then(res => res.json()));
	if (query.isLoading)
		return <div className="App">Loading..</div>;
	if (query.error)
		return <div className="App" style={{ color: 'red' }}>Failed to load stations config.<br/>{query.error.message}</div>;
	return (
		<>
			<Menu activeTab={activeTab} onChange={e => setActiveTab(e.target.value)}/>
			<div className="App">
				{activeTab === 'Status' ? <StatusTab /> : <Subscriptions stations={query.data}/>}
			</div>
		</>
	);
};

export default function AppWrapper() {
	return <QueryClientProvider client={queryClient}><App/></QueryClientProvider>;
}
