const fs = require('fs');
const pathlib = require('path');
const zlib = require('zlib');

const DIR = 'logs';
const filename = () => new Date().toISOString().replace(/T.*/, '') + '.log';
const path = file => pathlib.join(DIR, file);
let currentFile, writeStream;

function switchFiles() {
	if (currentFile === filename()) return writeStream;
	if (!fs.existsSync(DIR)) fs.mkdirSync(DIR);
	currentFile = filename();
	writeStream = fs.createWriteStream(path(currentFile), {flags: 'a'});
	fs.symlink(path(currentFile), path('last.log'))
		.catch(e => global.log(`Failed to symlink log: ${e}`));
	// gzip all not gziped log files except current
	fs.readdirSync(DIR).forEach(file => {
		if (!file.endsWith('.log') || file === currentFile) return;
		const fileContents = fs.createReadStream(path(file));
		const writeStream = fs.createWriteStream(path(file + '.gz'));
		fileContents.pipe(zlib.createGzip()).pipe(writeStream).on('finish', (err) => {
			if (err) global.log(`Failed to gzip: ${err}`);
			else fs.unlinkSync(path(file));
		});
	});
}
switchFiles();

module.exports = (msg) => {
	console.log(msg);
	switchFiles().write(`[${new Date().toISOString().replace(/\..+/g, '')}] ${msg}\n`);
};
