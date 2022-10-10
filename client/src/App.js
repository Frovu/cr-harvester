import { useState } from 'react';
import { QueryClient, QueryClientProvider, useQuery } from 'react-query';

import StatusTab from './StatusTab';
import Subscriptions from './Subscriptions';
import Corrections from './Corrections';

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
	const [activeTab, setActiveTab] = useState('Corrections'); // FIXME:
	const query = useQuery('stations',
		() => fetch(process.env.REACT_APP_API + '/stations').then(res => res.json()));
	if (query.isLoading)
		return <div className="App">Loading...</div>;
	if (query.error)
		return <div className="App" style={{ color: 'red' }}>Failed to load stations config.<br/>{query.error.message}</div>;
	return (
		<div className="App">
			<div className="Header">
				<Menu activeTab={activeTab} onChange={e => setActiveTab(e.target.value)}/>
				{activeTab !== 'Status' && <div className="Secret">
					<span>Secret key: </span>
					<input style={{ maxWidth: '8em' }} type="password"/>
				</div>}
			</div>
			<div className="AppBody">
				{activeTab === 'Status' ? <StatusTab /> :
					activeTab === 'Corrections' ? <Corrections devices={query.data.devices}/> :
						<Subscriptions stations={query.data.stations}/>}
			</div>
		</div>
	);
};

export default function AppWrapper() {
	return <QueryClientProvider client={queryClient}><App/></QueryClientProvider>;
}
