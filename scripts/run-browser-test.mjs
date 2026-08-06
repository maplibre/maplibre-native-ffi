// Runs an emcc HTML page or JavaScript module in isolated headless Chromium.
//
// Usage: node scripts/run-browser-test.mjs <page.html|module.js|module.mjs>
//        [--timeout-seconds N] [--render-backend NAME] [--browser-arg FLAG]...
//        [--module-arg ARG]... [--page-canvas]
//
// A `.js` module is hosted in a worker; a `.mjs` module is an ES module the
// page imports and instantiates itself. Backend-specific browser flags live
// here for the C, Rust, and Kotlin suites.

import { spawn } from "node:child_process";
import {
  createReadStream,
  existsSync,
  readdirSync,
  statSync,
  writeFileSync,
} from "node:fs";
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

// Style documents served by the Rust resource fixtures.
const FIXTURE_STYLE_LAYER_IDS = {
  "/__fixture/http-style.json": "http-fixture",
  "/__fixture/rewritten-style.json": "rewritten",
  "/__fixture/original-after-clear.json": "original-after-clear",
};

// Enable software graphics in headless Chromium.
const BACKEND_BROWSER_ARGS = {
  webgpu: [
    "--enable-unsafe-webgpu",
    "--enable-features=Vulkan",
    "--enable-unsafe-swiftshader",
    "--use-angle=swiftshader",
    "--use-vulkan=swiftshader",
  ],
  opengl: ["--enable-unsafe-swiftshader"],
};

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
      // Playwright has shipped both layouts; newer builds carry the arch.
      for (const relative of [
        "chrome-linux64/chrome",
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

const [targetPath, ...rest] = process.argv.slice(2);
if (!targetPath)
  fail(
    "usage: run-browser-test.mjs <page.html|module.js|module.mjs> [--timeout-seconds N]",
  );
if (!existsSync(targetPath)) fail(`test target does not exist: ${targetPath}`);

let timeoutSeconds = 600;
let pageCanvas = false;
const extraBrowserArgs = [];
const moduleArgs = [];
for (let i = 0; i < rest.length; i += 1) {
  if (rest[i] === "--timeout-seconds") timeoutSeconds = Number(rest[i + 1]);
  if (rest[i] === "--browser-arg") extraBrowserArgs.push(rest[i + 1]);
  if (rest[i] === "--module-arg") moduleArgs.push(rest[i + 1]);
  if (rest[i] === "--page-canvas") pageCanvas = true;
  if (rest[i] === "--render-backend") {
    const backend = rest[i + 1];
    if (!(backend in BACKEND_BROWSER_ARGS))
      fail(`no browser flags are known for the ${backend} backend`);
    extraBrowserArgs.push(...BACKEND_BROWSER_ARGS[backend]);
  }
}

const root = path.dirname(path.resolve(targetPath));

// The HTML parser ends an inline script at the first `</script`, whatever the
// JavaScript around it means, so a value carrying one has to reach the script
// escaped. Test filters and libtest arguments arrive here from a command line.
const embed = (value) => JSON.stringify(value).replaceAll("</", "<\\/");
const escapeText = (value) =>
  value
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;");

// The page half of the result relay. Every generated page reports one payload,
// and a page that fails before reaching the module reports the failure itself
// rather than leaving the run to time out.
const REPORTER_SCRIPT = `const report = (payload) =>
  void fetch("/__result", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
window.addEventListener("error", (event) =>
  report({ status: 70, output: "uncaught error: " + event.message }));
window.addEventListener("unhandledrejection", (event) =>
  report({ status: 70, output: "unhandled rejection: " + event.reason }));`;

const pageName = selectPage(path.basename(targetPath));

function selectPage(name) {
  switch (path.extname(name)) {
    case ".html":
      return name;
    case ".mjs":
      return generateEsModulePage(name);
    default:
      return generateModulePage(name);
  }
}

// Writes the page and worker a cargo test binary needs, which emcc does not
// produce for a `.js` output.
//
// The page hosts the module in a worker of its own so the browser's main thread
// stays out of it entirely. Whether the module then proxies its entry point onto
// a further pthread is the module's own business; see bindings/rust/mise.toml.
function generateModulePage(moduleName) {
  const stem = moduleName.replace(/\.js$/, "");
  const workerName = `${stem}.runner-worker.js`;
  const generatedPageName = `${stem}.runner.html`;

  writeFileSync(
    path.join(root, workerName),
    `self.onmessage = (event) => {
  const lines = [];
  const record = (text) => {
    lines.push(text);
    self.postMessage({ line: text });
  };
  self.Module = {
    // Emscripten resolves the pthread worker script from the running script's
    // URL, which in here is this worker rather than the module, so the pool
    // would load this file forever without being told otherwise.
    mainScriptUrlOrBlob: new URL(${JSON.stringify(moduleName)}, self.location.href).href,
    arguments: event.data.args,
    // Only a module linked with ENV among its exported runtime methods can be
    // handed the fixture origin. One that was not is a module with no test
    // reading it, so this reports nothing and lets the run proceed.
    preRun: [() => {
      if (typeof ENV !== "undefined") Object.assign(ENV, event.data.env);
    }],
    print: record,
    printErr: record,
    onExit: (status) => self.postMessage({ status, output: lines.join("\\n") }),
    onAbort: (reason) => {
      record("aborted: " + reason);
      self.postMessage({ status: 70, output: lines.join("\\n") });
    },
  };
  importScripts(${JSON.stringify(moduleName)});
};
`,
  );

  writeFileSync(
    path.join(root, generatedPageName),
    `<!DOCTYPE html>
<html lang="en">
  <head><meta charset="utf-8" /><title>${escapeText(stem)}</title></head>
  <body>
    <script>
${REPORTER_SCRIPT}

const worker = new Worker(${embed(workerName)});
worker.onmessage = (event) => {
  if (event.data.line !== undefined) {
    console.log(event.data.line);
    return;
  }
  report(event.data);
};
worker.onerror = (event) =>
  report({ status: 70, output: "worker error: " + event.message });
worker.postMessage({
  args: ${embed(moduleArgs)},
  env: { MLN_FFI_TEST_FIXTURE_ORIGIN: location.origin },
});
    </script>
  </body>
</html>
`,
  );
  return generatedPageName;
}

// Writes the page an ES module linked with -sMODULARIZE -sEXPORT_ES6 needs.
//
// This one instantiates the module on the page's own main thread rather than in
// a worker, because only a document holds a canvas the module can be given, and
// Emscripten transfers one only as it creates the thread that will draw on it.
// A module that keeps its main thread out of its own work -- one linked with
// -sPROXY_TO_PTHREAD -- leaves the page's event loop free anyway.
//
// The status reaches the page through the module's own exit, because nothing
// else crosses back from the thread the suite runs on.
function generateEsModulePage(moduleName) {
  const stem = moduleName.replace(/\.mjs$/, "");
  const generatedPageName = `${stem}.runner.html`;
  // A canvas the module can display on. It is handed over before the factory
  // is called because Emscripten transfers a canvas only as it creates the
  // thread that will draw on it, and instantiation is what creates that thread.
  const canvasMarkup = pageCanvas
    ? `<canvas id="maplibre" width="512" height="512"></canvas>`
    : "";
  const canvasOption = pageCanvas
    ? `options.mlnPageCanvas =
  document.getElementById("maplibre").transferControlToOffscreen();`
    : "";

  writeFileSync(
    path.join(root, generatedPageName),
    `<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <title>${escapeText(stem)}</title>
    <style>
      body { margin: 0 }
      canvas { display: block; width: 512px; height: 512px }
    </style>
  </head>
  <body>
    ${canvasMarkup}
    <script type="module">
${REPORTER_SCRIPT}

const lines = [];
const record = (text) => {
  lines.push(text);
  console.log(text);
};

const options = {
  arguments: ${embed(moduleArgs)},
  print: record,
  printErr: record,
  onExit: (status) => report({ status, output: lines.join("\\n") }),
  onAbort: (reason) => {
    record("aborted: " + reason);
    report({ status: 70, output: lines.join("\\n") });
  },
};
${canvasOption}

const create = (await import(${embed("./" + moduleName)})).default;
await create(options);
    </script>
  </body>
</html>
`,
  );
  return generatedPageName;
}

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

  const layerId = FIXTURE_STYLE_LAYER_IDS[url.pathname];
  if (layerId !== undefined) {
    response.writeHead(200, { "Content-Type": "application/json" }).end(
      JSON.stringify({
        version: 8,
        sources: {},
        layers: [{ id: layerId, type: "background" }],
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
