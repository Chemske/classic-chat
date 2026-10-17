import test from 'node:test';
import assert from 'node:assert/strict';
import { createRelay, responseEvents, generatedFiles, validateAttachment } from '../server/server.mjs';
const token = 'stream-test-device-token-at-least-32-characters';
const messages = [{ role: 'user', content: 'Hallo' }];
const answer = text => ({ status: 'completed', output: [{ type: 'message', content: [{ type: 'output_text', text }] }] });
const event = x => `data: ${JSON.stringify(x)}\r\n\r\n`;
function upstream(events) { return new Response(events.map(event).join(''), { headers: { 'Content-Type': 'text/event-stream' } }); }
async function fixture(options, run) {
  const server = createRelay({ token, apiKey: 'private-key', model: 'configured-model', ...options });
  await new Promise(r => server.listen(0, '127.0.0.1', r));
  const post = (body = { messages, stream: true }) => fetch(`http://127.0.0.1:${server.address().port}/v1/chat`, {
    method: 'POST', headers: { Authorization: `Bearer ${token}`, 'Content-Type': 'application/json' }, body: JSON.stringify(body)
  });
  try { await run(post); } finally { server.closeAllConnections(); await new Promise(r => server.close(r)); }
}
const records = async r => (await r.text()).trim().split('\n').map(JSON.parse);
test('stream delivers text before provider completion and reconciles final citations', async () => {
  let finish, observed;
  const stream = new ReadableStream({ start(c) {
    c.enqueue(Buffer.from(event({ type: 'response.output_text.delta', delta: 'Hallo 🌍' })));
    finish = () => { c.enqueue(Buffer.from(event({ type: 'response.completed', response: answer('Hallo 🌍') }))); c.close(); };
  } });
  await fixture({ fetchImpl: async (url, init) => { observed = JSON.parse(init.body); return new Response(stream); } }, async post => {
    const r = await post(); assert.match(r.headers.get('content-type'), /ndjson/);
    const reader = r.body.getReader(); let early = '';
    while (!early.includes('delta')) early += new TextDecoder().decode((await reader.read()).value);
    assert.match(early, /Hallo 🌍/); assert.doesNotMatch(early, /"done"/);
    assert.equal(observed.stream, true); assert.equal(observed.store, false);
    assert.equal(observed.tools.at(-1).name, 'create_text_file');
    assert.match(observed.instructions, /BlackBerry 10.3.3/);
    finish(); let rest = ''; while (true) { const x = await reader.read(); if (x.done) break; rest += new TextDecoder().decode(x.value); }
    assert.match(rest, /"type":"done"/);
  });
});
test('SSE parser handles split UTF-8 and CRLF boundaries', async () => {
  const bytes = Buffer.from(event({ type: 'response.output_text.delta', delta: 'café 🌍' })); let i = 0;
  const body = new ReadableStream({ pull(c) { if (i === bytes.length) c.close(); else c.enqueue(bytes.subarray(i, ++i)); } });
  const result = []; for await (const e of responseEvents(body)) result.push(e);
  assert.equal(result[0].delta, 'café 🌍');
});
test('incomplete and failing streams never emit a done record or leak provider errors', async () => {
  for (const tail of [[], [{ type: 'error', message: 'private-key internal provider exception' }], [{ type: 'response.completed', response: { status: 'incomplete', output: [] } }]]) {
    await fixture({ fetchImpl: async () => upstream([{ type: 'response.output_text.delta', delta: 'Deel' }, ...tail]) }, async post => {
      const result = await records(await post()); assert.equal(result.at(-1).type, 'error');
      assert.ok(!result.some(x => x.type === 'done')); assert.doesNotMatch(JSON.stringify(result), /private-key|provider exception/);
    });
  }
});
test('generated file is bounded text with safe filename and no server storage', async () => {
  const data = { status: 'completed', output: [{ type: 'function_call', name: 'create_text_file', arguments: JSON.stringify({ name: 'notitie.txt', content: 'Hallo\n🌍' }) }] };
  await fixture({ fetchImpl: async () => upstream([{ type: 'response.completed', response: data }]) }, async post => {
    const result = await records(await post()); assert.equal(result.at(-1).type, 'done');
    assert.deepEqual(result.at(-1).files, [{ name: 'notitie.txt', content: 'Hallo\n🌍' }]);
    assert.match(result.at(-1).text, /Opslaan op SD/);
  });
  for (const name of ['../test.txt', '/test.txt', 'a..b.txt', 'run.exe', 'a/b.txt'])
    assert.throws(() => generatedFiles({ output: [{ ...data.output[0], arguments: JSON.stringify({ name, content: 'x' }) }] }));
  assert.throws(() => generatedFiles({ output: [{ ...data.output[0], arguments: JSON.stringify({ name: 'a.txt', content: '🌍'.repeat(5000) }) }] }));
});
test('text and image attachments map only to the current input; client tools stay forbidden', async () => {
  let observed;
  await fixture({ fetchImpl: async (url, init) => { observed = JSON.parse(init.body); return upstream([{ type: 'response.completed', response: answer('Gelezen') }]); } }, async post => {
    const attachment = { name: 'notitie.txt', type: 'text/plain', text: 'Testdocument' };
    assert.equal((await post({ messages, attachment, stream: true })).status, 200);
    assert.match(observed.input[0].content[1].text, /Testdocument/);
    const data = Buffer.from([255,216,255,1,2,3]).toString('base64');
    await (await post({ messages, attachment: { name: 'foto.jpg', type: 'image/jpeg', data }, stream: true })).text();
    assert.equal(observed.input[0].content[1].type, 'input_image');
    assert.equal(observed.input[0].content[1].image_url, 'data:image/jpeg;base64,' + data);
    assert.equal((await post({ messages, stream: true, tools: [] })).status, 400);
    assert.equal((await post({ messages, stream: 'true' })).status, 400);
  });
});
test('attachments reject URLs, paths, malformed base64, spoofed types and oversized text', () => {
  for (const x of [
    { name: '../a.txt', type: 'text/plain', text: 'x' },
    { name: 'a.txt', type: 'text/plain', text: 'x'.repeat(13000) },
    { name: 'a.exe', type: 'text/plain', text: 'x' },
    { name: 'a.png', type: 'image/png', data: 'YWJj' },
    { name: 'a.jpg', type: 'image/jpeg', data: 'https://private.host/photo' },
    { name: 'a.txt', type: 'text/plain', text: 'x', path: '/etc/passwd' },
  ]) assert.throws(() => validateAttachment(x));
  const data = Buffer.alloc(512 * 1024, 1); data[0]=255;data[1]=216;data[2]=255;
  assert.equal(validateAttachment({ name: 'large.jpg', type: 'image/jpeg', data: data.toString('base64') }).type, 'image/jpeg');
});
test('disconnect aborts the upstream and releases its concurrency slot', async () => {
  let aborted = false, calls = 0;
  await fixture({ maxConcurrent: 1, fetchImpl: async (url, { signal }) => {
    calls++;
    if (calls > 1) return upstream([{ type: 'response.completed', response: answer('Opnieuw') }]);
    return new Response(new ReadableStream({ start(c) {
      c.enqueue(Buffer.from(event({ type: 'response.output_text.delta', delta: 'begin' })));
      signal.addEventListener('abort', () => { aborted = true; c.error(new DOMException('stop', 'AbortError')); });
    } }));
  } }, async post => {
    const r = await post(); const reader = r.body.getReader(); await reader.read(); await reader.cancel();
    for (let i=0; i<30 && !aborted; i++) await new Promise(r => setTimeout(r, 10));
    assert.equal(aborted, true);
    const retry = await post(); assert.equal(retry.status, 200); assert.equal((await records(retry)).at(-1).text, 'Opnieuw');
  });
});
