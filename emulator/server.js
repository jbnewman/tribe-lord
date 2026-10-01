// Runs the Tribe Lord web emulator. No dependencies, just Node.js 18 or newer.
// Start: node emulator/server.js    Stop: Ctrl+C
// The first run downloads the mGBA web emulator into emulator/mgba/.
const http = require('http');
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');
const crypto = require('crypto');

const PORT = 8080;
const EMULATOR_DIR = __dirname;
const MGBA_DIR = path.join(EMULATOR_DIR, 'mgba');
const ROM_PATH = path.join(EMULATOR_DIR, '..', 'tribe-lord.gba');
const MGBA_PACKAGE_URL = 'https://registry.npmjs.org/@thenick775/mgba-wasm/latest';

const MIME_TYPES = {
    '.html': 'text/html; charset=utf-8',
    '.js': 'text/javascript; charset=utf-8',
    '.css': 'text/css; charset=utf-8',
    '.wasm': 'application/wasm',
    '.gba': 'application/octet-stream'
};

// Downloads the mGBA package from npm and unpacks its dist/ files (an npm package is a gzipped tar file)
async function downloadMgba() {
    console.log('Downloading mGBA web emulator...');
    const info = await (await fetch(MGBA_PACKAGE_URL)).json();
    const tarball = Buffer.from(await (await fetch(info.dist.tarball)).arrayBuffer());

    const expectedHash = info.dist.integrity.replace('sha512-', '');
    const actualHash = crypto.createHash('sha512').update(tarball).digest('base64');
    if (actualHash !== expectedHash) {
        throw new Error('Downloaded mGBA package failed its integrity check.');
    }

    const tar = zlib.gunzipSync(tarball);
    const readText = (start, length) => tar.toString('utf8', start, start + length).replace(/\0.*$/s, '');
    fs.mkdirSync(MGBA_DIR, { recursive: true });

    // Each file in a tar is a 512 byte header followed by its contents padded to 512 bytes
    let offset = 0;
    while (offset + 512 <= tar.length && tar[offset] !== 0) {
        const name = readText(offset, 100);
        const prefix = readText(offset + 345, 155);
        const fullName = prefix ? prefix + '/' + name : name;
        const size = parseInt(readText(offset + 124, 12).trim() || '0', 8);
        const isFile = tar[offset + 156] === 0 || tar[offset + 156] === 48;
        const fileName = fullName.slice('package/dist/'.length);
        offset += 512;

        if (isFile && fullName.startsWith('package/dist/') && fileName && !fileName.includes('/')) {
            fs.writeFileSync(path.join(MGBA_DIR, fileName), tar.subarray(offset, offset + size));
        }
        offset += Math.ceil(size / 512) * 512;
    }

    console.log(`Installed mGBA web emulator ${info.version}`);
}

function sendFile(req, res, filePath) {
    fs.stat(filePath, (err, stats) => {
        if (err || !stats.isFile()) {
            res.writeHead(404, { 'Content-Type': 'text/plain' });
            return res.end('404 Not Found');
        }

        res.writeHead(200, {
            'Content-Type': MIME_TYPES[path.extname(filePath).toLowerCase()] || 'application/octet-stream',
            'Content-Length': stats.size,
            'Last-Modified': stats.mtime.toUTCString(),
            'Cache-Control': 'no-store'
        });

        if (req.method === 'HEAD') {
            return res.end();
        }
        fs.createReadStream(filePath).pipe(res);
    });
}

const server = http.createServer((req, res) => {
    // mGBA uses threads, which browsers only allow on "cross-origin isolated" pages
    res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
    res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');

    let urlPath;
    try {
        urlPath = decodeURIComponent(new URL(req.url, 'http://localhost').pathname);
    } catch {
        res.writeHead(400);
        return res.end('Bad request');
    }

    if (urlPath === '/tribe-lord.gba') {
        return sendFile(req, res, ROM_PATH);
    }
    if (urlPath.endsWith('/')) {
        urlPath += 'index.html';
    }

    // Block requests like /../secret.txt from escaping the emulator folder
    const filePath = path.join(EMULATOR_DIR, urlPath);
    if (!filePath.startsWith(EMULATOR_DIR + path.sep)) {
        res.writeHead(403);
        return res.end('Forbidden');
    }

    sendFile(req, res, filePath);
});

async function start() {
    if (!fs.existsSync(path.join(MGBA_DIR, 'mgba.wasm'))) {
        await downloadMgba();
    }

    // Only listen on this computer, not the whole network
    server.listen(PORT, '127.0.0.1', () => {
        console.log(`Tribe Lord emulator running at http://localhost:${PORT}  (Ctrl+C to stop)`);
    });
}

start().catch(err => {
    console.error(err.message);
    process.exit(1);
});
