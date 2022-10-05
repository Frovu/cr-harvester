import { useEffect, useState } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import './css/Corrections.css';

const dateStr = date => date?.toISOString().replace(/\..*/, '').replace('T', ' ');

export default function Editor({ settings, interval }) {
	return (
		<div className="Editor">
			<div style={{ position: 'absolute', top: '40%', left: '50%' }}>GRAPH</div>
			<div className="Footer">
				<div style={{ textAlign: 'right' }}>
					<span>{dateStr(interval[0])}</span><br/>
					<span>to {dateStr(interval[1])}</span>
				</div>
			</div>
		</div>
	);
}
