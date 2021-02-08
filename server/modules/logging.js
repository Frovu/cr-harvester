const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const DIR = 'logs';
const filename = () => {return new Date().toISOString().replace(/T.*/, '') + '.log';};
let currentFile, writeStream;

function switchFiles() {
	if(!fs.existsSync(DIR)) fs.mkdirSync(DIR);
	if(currentFile === filename()) return writeStream;
	currentFile = filename();
	writeStream = fs.createWriteStream(path.join(DIR, currentFile), {flags:'a'});
	// gzip all not gziped log files except current
	fs.readdirSync(DIR).forEach(file => {
		if(!file.endsWith('.log') || file === currentFile) return;
		const fileContents = fs.createReadStream(path.join(DIR, file));
		const writeStream = fs.createWriteStream(path.join(DIR, file + '.gz'));
		fileContents.pipe(zlib.createGzip()).pipe(writeStream).on('finish', (err) => {
			if (err) global.log(`Failed to gzip: ${err}`);
			else fs.rmSync(path.join(DIR, file));
		});
	});
}
switchFiles();

module.exports = (msg) => {
	console.log(msg);
	switchFiles().write(`[${new Date().toISOString().replace(/\..+/g, '')}] ${msg}\n`);
};
