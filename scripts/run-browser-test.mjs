// Runs a browser test page in headless Chromium and exits with its status.
//
// Two things make this more than "open a file:// URL":
//
//   * The build uses pthreads, so the page needs SharedArrayBuffer, which needs
//     cross-origin isolation. That means COOP/COEP response headers, and those
//     need a real origin, so the page is served rather than opened from disk.
//   * A page cannot set a process exit status, so the shell posts Unity's status
//     back here (see src/c_api/tests/browser_shell.html) and this process exits
//     with it.
//
// Usage: node scripts/run-browser-test.mjs <page.html> [--timeout-seconds N]
//        [--browser-arg FLAG]...
//
// Backends need different things from the browser, so the flags they need come
// from the build rather than being hardcoded here.

import { spawn } from "node:child_process";
import { createReadStream, existsSync, readdirSync, statSync } from "node:fs";
import { mkdtemp, rm } from "node:fs/promises";
import { createServer } from "node:http";
import { tmpdir } from "node:os";
import path from "node:path";
import process from "node:process";

const CONTENT_TYPES = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".mjs": "text/javascript; charset=utf-8",
  ".wasm": "application/wasm",
  ".data": "application/octet-stream",
  ".map": "application/json",
};

// Style document the browser HTTP regression test fetches. Serving it from the
// runner's real origin exercises emscripten_fetch end to end.
const HTTP_FIXTURE_PATH = "/__fixture/http-style.json";
const HTTP_FIXTURE_LAYER_ID = "http-fixture";

function fail(message) {
  console.error(`error: ${message}`);
  process.exit(2);
}

// Chromium comes from the environment where CI provides one, and otherwise from
// whatever is on PATH or in a local Playwright cache.
function findBrowser() {
  const explicit = process.env.MLN_FFI_TEST_BROWSER ?? process.env.CHROME_PATH;
  if (explicit) {
    if (!existsSync(explicit))
      fail(`MLN_FFI_TEST_BROWSER is not a file: ${explicit}`);
    return explicit;
  }
  const names = [
    "/usr/bin/google-chrome",
    "/usr/bin/google-chrome-stable",
    "/usr/bin/chromium",
    "/usr/bin/chromium-browser",
    "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
  ];
  for (const name of names) if (existsSync(name)) return name;

  const cache = path.join(
    process.env.HOME ?? "",
    process.platform === "darwin"
      ? "Library/Caches/ms-playwright"
      : ".cache/ms-playwright",
  );
  if (existsSync(cache)) {
    for (const entry of readdirSync(cache).sort().reverse()) {
      if (!entry.startsWith("chromium-")) continue;
      for (const relative of [
        "chrome-linux/chrome",
        "chrome-mac/Chromium.app/Contents/MacOS/Chromium",
      ]) {
        const candidate = path.join(cache, entry, relative);
        if (existsSync(candidate)) return candidate;
      }
    }
  }
  fail(
    "no Chromium found; set MLN_FFI_TEST_BROWSER to a Chrome or Chromium binary",
  );
}

const [pagePath, ...rest] = process.argv.slice(2);
if (!pagePath)
  fail("usage: run-browser-test.mjs <page.html> [--timeout-seconds N]");
if (!existsSync(pagePath)) fail(`page does not exist: ${pagePath}`);

let timeoutSeconds = 600;
const extraBrowserArgs = [];
for (let i = 0; i < rest.length; i += 1) {
  if (rest[i] === "--timeout-seconds") timeoutSeconds = Number(rest[i + 1]);
  if (rest[i] === "--browser-arg") extraBrowserArgs.push(rest[i + 1]);
}

const root = path.dirname(path.resolve(pagePath));
const pageName = path.basename(pagePath);

let settle;
const finished = new Promise((resolve) => {
  settle = resolve;
});

const server = createServer((request, response) => {
  // Cross-origin isolation, so the page may use SharedArrayBuffer.
  response.setHeader("Cross-Origin-Opener-Policy", "same-origin");
  response.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
  response.setHeader("Cache-Control", "no-store");

  const url = new URL(request.url ?? "/", "http://localhost");
  if (url.pathname === "/__result" && request.method === "POST") {
    const chunks = [];
    request.on("data", (chunk) => chunks.push(chunk));
    request.on("end", () => {
      response.writeHead(204).end();
      try {
        settle(JSON.parse(Buffer.concat(chunks).toString("utf8")));
      } catch (error) {
        settle({ status: 2, output: `malformed result: ${String(error)}` });
      }
    });
    return;
  }

  if (url.pathname === HTTP_FIXTURE_PATH) {
    response.writeHead(200, { "Content-Type": "application/json" }).end(
      JSON.stringify({
        version: 8,
        sources: {},
        layers: [{ id: HTTP_FIXTURE_LAYER_ID, type: "background" }],
      }),
    );
    return;
  }

  // Serving one directory, so a path that escapes it is a bug or an attack.
  const requested = path.join(root, path.normalize(url.pathname));
  if (
    !requested.startsWith(root) ||
    !existsSync(requested) ||
    statSync(requested).isDirectory()
  ) {
    response.writeHead(404).end("not found");
    return;
  }
  response.writeHead(200, {
    "Content-Type":
      CONTENT_TYPES[path.extname(requested)] ?? "application/octet-stream",
  });
  createReadStream(requested).pipe(response);
});

const browser = findBrowser();
const profile = await mkdtemp(path.join(tmpdir(), "mln-browser-test-"));

await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
const { port } = server.address();
const pageUrl = `http://127.0.0.1:${port}/${pageName}`;
console.log(`running ${pageUrl} in ${browser}`);

const child = spawn(
  browser,
  [
    "--headless=new",
    // Chromium's own logging carries the page console, so a run that hangs
    // still shows how far the suite got instead of only reporting the timeout.
    "--enable-logging=stderr",
    "--v=0",
    // CI containers run as root without user namespaces available.
    "--no-sandbox",
    "--disable-dev-shm-usage",
    `--user-data-dir=${profile}`,
    ...extraBrowserArgs,
    pageUrl,
  ],
  { stdio: ["ignore", "pipe", "pipe"] },
);
child.stdout.on("data", (chunk) => process.stderr.write(`[browser] ${chunk}`));
child.stderr.on("data", (chunk) => process.stderr.write(`[browser] ${chunk}`));

let browserExited = false;

const timer = setTimeout(() => {
  settle({
    status: 2,
    output: `timed out after ${timeoutSeconds}s with no result`,
  });
}, timeoutSeconds * 1000);

child.on("exit", (code, signal) => {
  browserExited = true;
  settle({
    status: 2,
    output: `browser exited before reporting a result (code=${code} signal=${signal})`,
  });
});

const result = await finished;
clearTimeout(timer);
server.close();

// Wait for the browser to actually be gone before removing its profile, or the
// removal races the files it is still flushing. A child killed by a signal
// leaves exitCode null, so the flag is what says whether 'exit' already fired.
if (!browserExited) {
  const exited = new Promise((resolve) => child.once("exit", resolve));
  child.kill("SIGKILL");
  await exited;
}
await rm(profile, { recursive: true, force: true, maxRetries: 5 });

// Chromium's logging already carried the console through as the suite ran, so
// the collected output only gets reprinted when it is the failure report.
if (result.status !== 0 && result.output) console.log(result.output);
process.exit(result.status === 0 ? 0 : 1);
