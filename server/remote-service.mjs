// Local relay + temporary public tunnel. Never prints credentials or chat content.
import { spawn } from 'node:child_process';
import { readFileSync, writeFileSync, renameSync, existsSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
const root = path.dirname(fileURLToPath(import.meta.url));
const config = JSON.parse(readFileSync(path.join(root, 'remote-runtime.json'), 'utf8'));
const privateEnv = config.envFile || path.join(root, '.env');
if (!existsSync(privateEnv) || !existsSync(config.cloudflared)) throw new Error('Lokale configuratie of tunnelprogramma ontbreekt.');
const children = [];
let stopping = false;
let tunnelLines = '';
const stateFile = path.join(config.stateDirectory || root, 'remote-state.json');
const internetFile = config.internetAddressFile || path.join(root, '../Internetadres.txt');
function record(url) {
  const state = { url, startedAt: new Date().toISOString(), temporary: true, running: Boolean(url) };
  writeFileSync(stateFile + '.tmp', JSON.stringify(state, null, 2) + '\n');
  renameSync(stateFile + '.tmp', stateFile);
  const host = process.platform === 'darwin' ? 'de Mac' : 'de VPS';
  writeFileSync(internetFile, url ? `${url}\n\nTijdelijk adres; ${host} moet aanstaan en internet hebben. Toegang tot chatten vereist je toestel-token.\n` : 'De internetverbinding is gestopt. Start Classic Chat internet om een nieuw adres te krijgen.\n');
}
function stop(code = 0) {
  if (stopping) return;
  stopping = true;
  record(null);
  for (const child of children) if (child.exitCode === null) child.kill('SIGTERM');
  const finish = () => { if (children.every(child => child.exitCode !== null || child.signalCode !== null)) process.exit(code); };
  for (const child of children) child.on('exit', finish);
  finish();
  setTimeout(() => { for (const child of children) if (child.exitCode === null) child.kill('SIGKILL'); process.exit(code); }, 5000);
}
function launch(executable, args, output = 'inherit') {
  const child = spawn(executable, args, { cwd: root, env: process.env, stdio: output });
  children.push(child);
  child.on('error', () => { console.error('Een Classic Chat-dienst kon niet starten.'); stop(1); });
  child.on('exit', code => { if (!stopping) { console.error('Een Classic Chat-dienst is gestopt.'); stop(code || 1); } });
  return child;
}
process.on('SIGTERM', () => stop());
process.on('SIGINT', () => stop());
record(null);
const relay = launch(process.execPath, [`--env-file=${privateEnv}`, path.join(root, 'server.mjs')]);
let healthy = false;
for (let attempt = 0; attempt < 30 && !stopping; attempt++) {
  try { const result = await fetch('http://127.0.0.1:8787/health', { signal: AbortSignal.timeout(1000) }); healthy = result.ok; } catch {}
  if (healthy) break;
  await new Promise(resolve => setTimeout(resolve, 200));
}
if (!healthy || stopping) stop(1);
else {
  if (process.platform === 'darwin') launch('/usr/bin/caffeinate', ['-i', '-w', String(process.pid)], 'ignore');
  const tunnel = launch(config.cloudflared, ['tunnel', '--url', 'http://127.0.0.1:8787', '--no-autoupdate', '--protocol', 'http2', '--metrics', '127.0.0.1:20491', '--grace-period', '3s'], ['ignore', 'pipe', 'pipe']);
  function output(chunk) {
    tunnelLines += chunk.toString();
    if (tunnelLines.length > 65536) tunnelLines = tunnelLines.slice(-32768);
    let lineEnd;
    while ((lineEnd = tunnelLines.indexOf('\n')) >= 0) {
      const line = tunnelLines.slice(0, lineEnd); tunnelLines = tunnelLines.slice(lineEnd + 1);
      const match = line.match(/https:\/\/[a-z0-9-]+\.trycloudflare\.com\b/);
      if (match) { record(match[0]); console.log('Tijdelijk internetadres: ' + match[0]); }
      if (line.includes('Registered tunnel connection')) console.log('Internetverbinding actief.');
      if (/\bERR\b/.test(line)) console.error('Tunnel meldt een verbindingsfout; wacht op automatisch herstel.');
    }
  }
  tunnel.stdout.on('data', output); tunnel.stderr.on('data', output);
}
