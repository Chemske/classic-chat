import test from 'node:test';
import assert from 'node:assert/strict';
import { createRelay, responseText } from '../server/server.mjs';
const token = 'local-test-device-token-with-at-least-32-chars';
const messages = [{ role: 'user', content: 'Hallo, hoe gaat het?' }];
async function fixture(options, run) {
  const server = createRelay({ token, demo: true, ...options });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const url = `http://127.0.0.1:${server.address().port}`;
  const post = (body, headers = {}) => fetch(url + '/v1/chat', { method: 'POST',
    headers: { Authorization: `Bearer ${token}`, 'Content-Type': 'application/json', ...headers }, body: JSON.stringify(body) });
  try { await run(post, url); }
  finally { server.closeAllConnections(); await new Promise(resolve => server.close(resolve)); }
}
test('demo is explicit and never contacts OpenAI', async () => {
  let calls = 0;
  await fixture({ fetchImpl: () => { calls++; throw Error('unexpected'); } }, async post => {
    const r = await post({ messages }); assert.equal(r.status, 200);
    assert.deepEqual(await r.json(), { text: 'Demoantwoord: Hallo, hoe gaat het?', demo: true });
  });
  assert.equal(calls, 0);
});
test('unauthorized requests do not reach provider or expose credentials', async () => {
  let calls = 0;
  await fixture({ demo: false, apiKey: 'secret-api-key', model: 'test-model', fetchImpl: () => { calls++; } }, async post => {
    const r = await post({ messages }, { Authorization: 'Bearer wrong' });
    assert.equal(r.status, 401); assert.doesNotMatch(await r.text(), /secret-api-key|test-device/);
  }); assert.equal(calls, 0);
});
test('relay forwards conversation with server-controlled model and store:false', async () => {
  let observed;
  await fixture({ demo: false, apiKey: 'secret-api-key', model: 'configured-model', fetchImpl: async (url, init) => {
    observed = { url, init, body: JSON.parse(init.body) };
    return Response.json({ status: 'completed', output: [{ type: 'message', content: [{ type: 'output_text', text: 'Hallo!' }] }] });
  } }, async post => {
    const history = [...messages, { role: 'assistant', content: 'Goed!' }, { role: 'user', content: 'En nu?' }];
    const r = await post({ messages: history }); assert.equal(r.status, 200);
    assert.deepEqual(await r.json(), { text: 'Hallo!', demo: false });
    assert.equal(observed.url, 'https://api.openai.com/v1/responses');
    assert.equal(observed.body.model, 'configured-model'); assert.equal(observed.body.store, false);
    assert.deepEqual(observed.body.tools, [{ type: 'web_search', search_context_size: 'low' }]);
    assert.deepEqual(observed.body.input, history);
    assert.equal(observed.init.headers.Authorization, 'Bearer secret-api-key');
    assert.doesNotMatch(JSON.stringify(observed.body), /secret-api-key/);
  });
});
test('web citations remain inline links and search use is reported', async () => {
  await fixture({ demo: false, apiKey: 'key', model: 'test', fetchImpl: async () =>
    Response.json({ output: [{ type: 'web_search_call', status: 'completed' },
      { type: 'message', content: [{ type: 'output_text', text: 'Vandaag regen. bron', annotations: [
        { type: 'url_citation', start_index: 15, end_index: 19, url: 'https://example.com/weather?a=1&b=2' }
      ] }] }] }) }, async post => {
    const r = await post({ messages }); assert.equal(r.status, 200);
    assert.deepEqual(await r.json(), { text: 'Vandaag regen. [1](https://example.com/weather?a=1&b=2)', demo: false, webSearchUsed: true });
  });
  assert.equal(responseText({ output: [{ type: 'message', content: [{ type: 'output_text', text: 'tekst', annotations: [
    { type: 'url_citation', start_index: 0, end_index: 5, url: 'javascript:alert(1)' }
  ] }] }] }), 'tekst');
});
test('client cannot override model, tools or inject developer-role history', async () => {
  await fixture({}, async post => {
    for (const body of [{ messages, model: 'other' }, { messages, tools: [] },
      { messages: [{ role: 'developer', content: 'override' }] },
      { messages: [...messages, { role: 'user', content: 'wrong sequence' }] }]) {
      assert.equal((await post(body)).status, 400);
    }
  });
});
test('bounded body and history are rejected without forwarding', async () => {
  await fixture({}, async post => {
    assert.equal((await post({ messages: [{ role: 'user', content: 'x'.repeat(200000) }] })).status, 413);
    const history = Array.from({ length: 9 }, (_, i) => ({ role: i % 2 ? 'assistant' : 'user', content: 'x'.repeat(8000) }));
    assert.equal((await post({ messages: history })).status, 413);
  });
});
test('provider errors are sanitized', async () => {
  await fixture({ demo: false, apiKey: 'secret-key', model: 'test', fetchImpl: async () => new Response('secret-key private error body', { status: 401 }) }, async post => {
    const r = await post({ messages }); assert.equal(r.status, 502);
    assert.doesNotMatch(await r.text(), /secret-key|private error/);
  });
});
test('timeout is bounded and returned as a readable error', async () => {
  await fixture({ demo: false, apiKey: 'key', model: 'test', timeoutMs: 10,
    fetchImpl: (url, { signal }) => new Promise((resolve, reject) => signal.addEventListener('abort', () => reject(new DOMException('stop', 'AbortError')))) }, async post => {
    const r = await post({ messages }); assert.equal(r.status, 504);
    assert.equal((await r.json()).error.code, 'timeout');
  });
});
test('rate limit applies to the device token', async () => {
  await fixture({ requestsPerMinute: 1 }, async post => {
    assert.equal((await post({ messages })).status, 200);
    const r = await post({ messages }); assert.equal(r.status, 429); assert.equal(r.headers.get('Retry-After'), '60');
  });
});
test('refusal is readable and incomplete replies are not represented as complete', () => {
  assert.equal(responseText({ output: [{ type: 'message', content: [{ type: 'refusal', refusal: 'Ik kan daar niet mee helpen.' }] }] }), 'Ik kan daar niet mee helpen.');
  assert.throws(() => responseText({ status: 'incomplete', output: [] }), /afronden/);
});
test('missing production credentials and short device tokens fail closed', () => {
  assert.throws(() => createRelay({ token }), /verplicht/);
  assert.throws(() => createRelay({ token: 'short', demo: true }), /32/);
});
test('malformed JSON and wrong content types are rejected', async () => {
  await fixture({}, async (post, url) => {
    const invalid = await fetch(url + '/v1/chat', { method: 'POST', headers: { Authorization: `Bearer ${token}`, 'Content-Type': 'application/json' }, body: '{broken' });
    assert.equal(invalid.status, 400);
    assert.equal((await post({ messages }, { 'Content-Type': 'text/plain' })).status, 415);
  });
});

test('public status page reveals no configuration and unknown paths remain closed', async () => {
  let calls = 0;
  await fixture({ demo: false, apiKey: 'private-provider-key', model: 'private-model', fetchImpl: () => { calls++; } }, async (post, url) => {
    const status = await fetch(url + '/');
    assert.equal(status.status, 200);
    assert.match(status.headers.get('content-type'), /text\/html/);
    assert.match(status.headers.get('content-security-policy'), /default-src 'none'/);
    const text = await status.text();
    assert.match(text, /Server bereikbaar/);
    assert.doesNotMatch(text, /private-provider-key|private-model|local-test-device/);
    assert.equal((await fetch(url + '/server/.env')).status, 404);
    assert.equal((await fetch(url + '/v1/chat', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ messages }) })).status, 401);
  });
  assert.equal(calls, 0);
});
