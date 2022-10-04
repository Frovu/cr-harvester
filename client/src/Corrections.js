import { useEffect, useState } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import './css/Subscriptions.css';

function Selector({ text, options, defaultOption, callback }) {

}

function Editor() {

}

export default function Corrections({ devices }) {
	const [settings, setSettings] = useState(null);

	useEffect(() => setSettings(JSON.parse(window.localStorage.getItem('corrSettings'))), []);
	useEffect(() => window.localStorage.setItem('corrSettings', settings), [settings]);

	return (
		<div className="Corrections">
			<div className="Settings">
				<Selector text="Device" options={Object.keys(devices)}/>
			</div>
		</div>
	);
}
