async function readJson(response) {
  try {
    return await response.json();
  } catch (e) {
    return {};
  }
}

async function getJson(url) {
  const response = await fetch(url);
  const body = await readJson(response);
  if (!response.ok) {
    throw new Error(body.error || `HTTP ${response.status}`);
  }
  return body;
}

async function postJson(url, data = {}) {
  const response = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  });
  const body = await readJson(response);
  if (!response.ok) {
    const error = new Error(body.error || `HTTP ${response.status}`);
    error.status = response.status;
    error.body = body;
    throw error;
  }
  return body;
}

function escapeHtml(value) {
  return String(value)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

document.addEventListener('DOMContentLoaded', () => {
  const path = location.pathname;
  const section = path.startsWith('/settings') || path.startsWith('/saved_networks') ? '/settings' : '/';
  document.querySelectorAll('.nav-btn').forEach((link) => {
    link.classList.toggle('active', link.getAttribute('href') === section);
  });
});
