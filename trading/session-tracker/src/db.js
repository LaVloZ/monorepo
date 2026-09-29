const cfg = () => window.APP_CONFIG;
const root = () => cfg().couchUrl || "/db";
const base = () => `${root()}/${cfg().dbName}`;

async function req(url, options = {}, retried = false) {
  const headers = { "Content-Type": "application/json", ...(options.headers || {}) };
  if (cfg().authHeader) headers.Authorization = cfg().authHeader;
  const res = await fetch(url, {
    credentials: "same-origin",
    ...options,
    headers,
  });
  if (res.status === 401 && !retried && url !== `${root()}/_session`) {
    await login();
    return req(url, options, true);
  }
  return res;
}

export async function login() {
  const res = await req(`${root()}/_session`, {
    method: "POST",
    body: JSON.stringify({ name: cfg().couchUser, password: cfg().couchPassword }),
  });
  if (!res.ok) throw new Error(`login CouchDB: ${res.status}`);
}

export async function createDoc(doc) {
  const res = await req(base(), { method: "POST", body: JSON.stringify(doc) });
  if (!res.ok) throw new Error(`createDoc: ${res.status}`);
  return res.json();
}

export async function getDoc(id) {
  const res = await req(`${base()}/${id}`);
  if (res.status === 404) return null;
  if (!res.ok) throw new Error(`getDoc: ${res.status}`);
  return res.json();
}

export async function updateDoc(doc) {
  const res = await req(`${base()}/${doc._id}`, {
    method: "PUT",
    body: JSON.stringify(doc),
  });
  if (!res.ok) throw new Error(`updateDoc: ${res.status}`);
  return res.json();
}

export async function deleteDoc(id, rev) {
  const res = await req(`${base()}/${id}?rev=${rev}`, { method: "DELETE" });
  if (!res.ok) throw new Error(`deleteDoc: ${res.status}`);
}

export async function find(selector, opts = {}) {
  const res = await req(`${base()}/_find`, {
    method: "POST",
    body: JSON.stringify({ selector, limit: 10000, ...opts }),
  });
  if (!res.ok) throw new Error(`find: ${res.status}`);
  const data = await res.json();
  return data.docs;
}
