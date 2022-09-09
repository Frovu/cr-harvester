import './css/App.css';
import { QueryClient, QueryClientProvider } from 'react-query';
import StatusTab from './StatusTab';

const queryClient = new QueryClient();

export default function App() {
	return (
		<QueryClientProvider client={queryClient}>
			<div className="App">
				<StatusTab />
			</div>
		</QueryClientProvider>
	);
};
