import React, { useEffect, useState, useRef } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import './css/Subscriptions.css';

function emailMutation(action, station, secret, email) {
	return async () => {
		const res = await fetch(process.env.REACT_APP_API + '/subscriptions', {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({ action, station, secret, email })
		});
		const body = await res.json().catch(() => {});
		if (res.status !== 200)
			return { error: body?.error ? `${res.status}: ${body.error}`: `HTTP: ${res.status}` };
		return {};
	};
}

function AddEmail({ station, secret, callback }) {
	const queryClient = useQueryClient();
	const [email, setEmail] = useState();
	const mutation = useMutation(emailMutation('sub', station, secret, email), {
		onError: (error) => callback({ error }),
		onSuccess: (data) => {
			queryClient.invalidateQueries(['subs']);
			callback(data);
		}
	});
	return (
		<div className="AddEmail">
			<input type="textbox" placeholder="example@gmail.com" onChange={e => setEmail(e.target.value)}/>
			<button onClick={mutation.mutate}>Add</button>
		</div>
	);
}

function Email({ email, station, secret, callback }) {
	const queryClient = useQueryClient();
	const mutation = useMutation(emailMutation('unsub', station, secret, email), {
		onError: (error) => callback({ error }),
		onSuccess: (data) => {
			queryClient.invalidateQueries(['subs']);
			callback(data);
		}
	});
	return (
		<div className="Email">
			<div className="EmailText">{email}</div>
			<span className="EmailCross" onClick={mutation.mutate}>&times;</span>
		</div>
	);
}

export default function Subscriptions({ stations }) {
	const [report, setReport] = useState();
	const [secret, setSecret] = useState();
	useEffect(() => {
		setTimeout(() => setReport(null), 3000);
	}, [report]);
	const query = useQuery(['subs'], () =>
		fetch(process.env.REACT_APP_API + '/subscriptions').then(res => res.json()));
	if (query.isLoading) return;
	if (query.error)
		return <div className="Subscriptions" style={{ color: 'red' }}>{query.error?.message}</div>;
	return (
		<div className="Subscriptions">
			<input className="Secret" name="secret" type="password" placeholder="Secret" onChange={e => setSecret(e.target.value)}/>
			{Object.keys(stations).map(s => (
				<div key={s}>
					<h4>{stations[s].name}</h4>
					<div className="EmailList">
						{query.data[s].map(email =>
							<Email station={s} secret={secret} email={email} key={email} callback={setReport}/>
						)}
					</div>
					<AddEmail station={s} secret={secret} callback={setReport}/>
				</div>
			))}
			<div style={{
				visibility: report ? 'visible' : 'hidden',
				color: report?.error ? 'red' : 'inherit'
			}}>
				{report ? (report?.error?.toString() ?? 'Success') : 'dummy'}
			</div>
		</div>
	);

}
