// LAN-only HTTPS front door for the relay on this Mac. Never disables client TLS validation.
import https from 'node:https';
import http from 'node:http';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
const root = path.dirname(fileURLToPath(import.meta.url));
const bindHost = process.env.CLASSIC_CHAT_BIND;
if (!bindHost || !/^192\.168\.\d{1,3}\.\d{1,3}$/.test(bindHost)) {
  throw new Error('CLASSIC_CHAT_BIND moet het lokale 192.168.x.x-adres van de Mac zijn.');
}
const relayPort = Number(process.env.PORT || 8787);
const tlsPort = 8788;
const server = https.createServer({
  key: readFileSync(path.join(root, '.private/server.key')),
  cert: readFileSync(path.join(root, '.private/server.crt')),
  minVersion: 'TLSv1.2'
}, (req, res) => {
  const upstream = http.request({ host: '127.0.0.1', port: relayPort,
    method: req.method, path: req.url, headers: { ...req.headers, host: `127.0.0.1:${relayPort}` },
    timeout: 100000 }, reply => {
      res.writeHead(reply.statusCode, reply.headers); reply.pipe(res);
    });
  upstream.on('timeout', () => upstream.destroy());
  upstream.on('error', () => {
    if (!res.headersSent) { res.writeHead(502, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' }); }
    if (!res.destroyed) res.end(JSON.stringify({ error: { code: 'local_relay_unavailable', message: 'De tussenserver op de Mac is niet bereikbaar.' } }));
  });
  req.on('aborted', () => upstream.destroy());
  res.on('close', () => { if (!res.writableEnded) upstream.destroy(); });
  req.pipe(upstream);
});
server.headersTimeout = 10000; server.requestTimeout = 20000;
server.listen(tlsPort, bindHost, () => console.log(`Classic Chat HTTPS beschikbaar op https://${bindHost}:${tlsPort}.`));
