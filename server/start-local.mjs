// Starts the two local services; does not persist or print secrets.
import { spawn } from 'node:child_process';
import { readFileSync, existsSync } from 'node:fs';
import { networkInterfaces } from 'node:os';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
const root = path.dirname(fileURLToPath(import.meta.url));
const envFile = path.join(root, '.env');
if (!existsSync(envFile)) throw new Error('De private serverconfiguratie ontbreekt.');
const { bind } = JSON.parse(readFileSync(path.join(root, 'local-network.json'), 'utf8'));
const addresses = Object.values(networkInterfaces()).flat().filter(Boolean).map(item => item.address);
if (!addresses.includes(bind)) throw new Error('Het wifi-adres van de Mac is veranderd. Werk eerst het HTTPS-certificaat en lokale adres bij.');
const children = [];
let stopping = false;
function stop(code = 0) {
  if (stopping) return;
  stopping = true;
  for (const child of children) if (child.exitCode === null) child.kill('SIGTERM');
  setTimeout(() => process.exit(code), 1000);
}
function launch(args, env) {
  const child = spawn(process.execPath, args, { cwd: root, env, stdio: 'inherit' });
  children.push(child);
  child.on('error', () => { console.error('Een lokale dienst kon niet starten.'); stop(1); });
  child.on('exit', code => { if (!stopping) stop(code || 1); });
}
launch([`--env-file=${envFile}`, path.join(root, 'server.mjs')], process.env);
launch([path.join(root, 'local-https.mjs')], { ...process.env, CLASSIC_CHAT_BIND: bind, PORT: '8787' });
process.on('SIGINT', () => stop());
process.on('SIGTERM', () => stop());
console.log('Classic Chat start op deze Mac. Laat dit venster open. Stoppen: Ctrl+C.');
