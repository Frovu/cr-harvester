import { useEffect, useState } from 'react';
import { useQueryClient, useQuery, useMutation } from 'react-query';

import UPlotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import './css/Corrections.css';

const dateStr = date => date?.toISOString().replace(/T.*/, '');

function Graph({  }) {

}

export default function Editor({ fields, interval }) {

	const query = useQuery('editor', async () => {
		const para = new URLSearchParams({ fields: fields }).toString();
		const resp = await fetch(process.env.REACT_APP_API + '/status?' + para);
	});
	return (
		<div className="Editor">
			<div style={{ position: 'absolute', top: '45%', left: '50%', transform: 'translate(-50%, -50%)' }}>GRAPH</div>
			<div className="Footer">
				<div style={{ textAlign: 'right' }}>
					<span>{dateStr(interval[0])}</span><br/>
					<span>to {dateStr(interval[1])}</span>
				</div>
				<div>{fields.join()}</div>
			</div>
		</div>
	);
}
