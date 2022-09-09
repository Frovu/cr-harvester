import React, { useEffect, useState, useRef } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import './css/Subscriptions.css';

export default function Subscriptions(props) {
	const queryClient = useQueryClient();
	const query = useQuery('subs', () =>
		fetch(process.env.REACT_APP_API + '/subscriptions').then(res => res.json()));
	if (query.isLoading) return;
	if (query.error)
		return <div className="Subscriptions" style={{ color: 'red' }}>{query.error?.message}</div>;
	return (
		<div className="Subscriptions">
			{Object.keys(props.stations).map(s => (
				<div key={s}>
					<p>{props.stations[s].name}</p>
				</div>
			))}
			<input className="Secret" name="secret" type="password" placeholder="Secret"/>
		</div>
	);

}
