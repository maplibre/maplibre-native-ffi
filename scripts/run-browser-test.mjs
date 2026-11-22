// Runs a test page in headless Chromium.
//
// Usage: node scripts/run-browser-test.mjs <page.html>
//        [--timeout-seconds N] [--render-backend NAME] [--browser-arg FLAG]...
//
// Backend-specific browser flags live here for every suite.
//
// The browser is slower than a native run, so the page gets the timeout scale
// that the test suites stretch their waits by: MLN_TEST_TIMEOUT_SCALE, or 3.

import { spawn } from "node:child_process";
import { createReadStream, existsSync, statSync } from "node:fs";
import { mkdtemp, rm } from "node:fs/promises";
import { createServer } from "node:http";
import { tmpdir } from "node:os";
import path from "node:path";
import process from "node:process";

import { findChromium } from "./find-chromium.mjs";

const CONTENT_TYPES = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".mjs": "text/javascript; charset=utf-8",
  ".wasm": "application/wasm",
  ".data": "application/octet-stream",
  ".map": "application/json",
};

// Headless Chromium can use Metal on macOS. Forcing its Linux SwiftShader
// Vulkan stack there prevents WebGPU device creation.
const WEBGPU_BROWSER_ARGS =
  process.platform === "darwin"
    ? ["--enable-unsafe-webgpu"]
    : [
        "--enable-unsafe-webgpu",
        "--enable-features=Vulkan",
        "--enable-unsafe-swiftshader",
        "--use-angle=swiftshader",
        "--use-vulkan=swiftshader",
      ];

// Enable software graphics where the host cannot use its native graphics API.
const BACKEND_BROWSER_ARGS = {
  webgpu: WEBGPU_BROWSER_ARGS,
  opengl: ["--enable-unsafe-swiftshader"],
};

function fail(message) {
  console.error(`error: ${message}`);
  process.exit(2);
}

const [targetPath, ...rest] = process.argv.slice(2);
if (!targetPath)
  fail("usage: run-browser-test.mjs <page.html> [--timeout-seconds N]");
if (!existsSync(targetPath)) fail(`test target does not exist: ${targetPath}`);

let timeoutSeconds = 600;
const extraBrowserArgs = [];
for (let i = 0; i < rest.length; i += 1) {
  if (rest[i] === "--timeout-seconds") timeoutSeconds = Number(rest[i + 1]);
  if (rest[i] === "--browser-arg") extraBrowserArgs.push(rest[i + 1]);
  if (rest[i] === "--render-backend") {
    const backend = rest[i + 1];
    if (!(backend in BACKEND_BROWSER_ARGS))
      fail(`no browser flags are known for the ${backend} backend`);
    extraBrowserArgs.push(...BACKEND_BROWSER_ARGS[backend]);
  }
}

const timeoutScale = process.env.MLN_TEST_TIMEOUT_SCALE ?? "3";
const root = path.dirname(path.resolve(targetPath));
const pageName = path.basename(targetPath);

let settle;
const finished = new Promise((resolve) => {
  settle = resolve;
});

// The HTTP origins that a page's suite opens and closes, each under
// /__origins/<id>/ on the page's own origin, so the module's fetches need no
// cross-origin access. An origin serves fixed bodies by path and records each
// request's path and headers. POST /__origins takes { routes } and answers
// { id }; GET /__origins/<id>/requests answers the records; DELETE closes it.
const origins = new Map();
let nextOrigin = 1;

function readJson(request) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    request.on("data", (chunk) => chunks.push(chunk));
    request.on("end", () => {
      try {
        resolve(JSON.parse(Buffer.concat(chunks).toString("utf8")));
      } catch (error) {
        reject(error);
      }
    });
  });
}

function serveOrigin(url, request, response) {
  const json = (status, value) =>
    response
      .writeHead(status, { "Content-Type": "application/json" })
      .end(value === undefined ? "" : JSON.stringify(value));
  if (url.pathname === "/__origins" && request.method === "POST") {
    readJson(request).then(
      ({ routes }) => {
        const id = nextOrigin++;
        origins.set(id, { routes, requests: [] });
        json(200, { id });
      },
      () => json(400),
    );
    return;
  }
  const match = /^\/__origins\/(\d+)(\/.*)?$/.exec(url.pathname);
  const origin = match && origins.get(Number(match[1]));
  if (!origin) {
    json(404);
    return;
  }
  const rest = match[2] ?? "/";
  if (rest === "/requests" && request.method === "GET") {
    json(200, origin.requests);
  } else if (rest === "/" && request.method === "DELETE") {
    origins.delete(Number(match[1]));
    json(204);
  } else {
    origin.requests.push({ path: rest + url.search, headers: request.headers });
    const body = origin.routes[rest + url.search];
    if (body === undefined) json(404);
    else
      response.writeHead(200, { "Content-Type": "application/json" }).end(body);
  }
}

const server = createServer((request, response) => {
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

  if (url.pathname.startsWith("/__origins")) {
    serveOrigin(url, request, response);
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

let browser;
try {
  browser = findChromium();
} catch (error) {
  fail(error.message);
}
const profile = await mkdtemp(path.join(tmpdir(), "mln-browser-test-"));

await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
const { port } = server.address();
const pageQuery = new URLSearchParams({ scale: timeoutScale });
const pageUrl = `http://127.0.0.1:${port}/${pageName}?${pageQuery}`;
console.log(`running ${pageUrl} in ${browser}`);

const child = spawn(
  browser,
  [
    "--headless=new",
    // Temporary test profiles must not access the host's credential store.
    "--password-store=basic",
    "--use-mock-keychain",
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
// process.exit drops what Node hasn't yet written to a full pipe, so the exit
// waits for the write.
if (result.status !== 0 && result.output)
  await new Promise((resolve) =>
    process.stdout.write(`${result.output}\n`, resolve),
  );
process.exit(result.status === 0 ? 0 : 1);
