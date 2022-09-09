import { useState } from 'react';
import { QueryClient, QueryClientProvider } from 'react-query';

import StatusTab from './StatusTab';

import './css/App.css';

const queryClient = new QueryClient();

function Menu({ onChange, activeTab }) {
	return (
		<div className="Menu">
			{['Status', 'Corrections', 'Subscriptions'].map(tab => (
				<div className="MenuButton">
					<label>
						<input type="radio" name="Menu" value={tab} checked={activeTab===tab} onChange={onChange}/>
						{tab}
					</label>
				</div>
			))}
		</div>
	);
}

export default function App() {
	const [activeTab, setActiveTab] = useState('Status');
	return (
		<>
			<Menu activeTab={activeTab} onChange={e => setActiveTab(e.target.value)}/>
			<QueryClientProvider client={queryClient}>
				<div className="App">
					<StatusTab />
				</div>
			</QueryClientProvider>
		</>
	);
};
