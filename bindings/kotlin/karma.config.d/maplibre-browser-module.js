// Karma configuration the browser binding's tests need, appended by Kotlin to the generated
// karma.conf.js.
//
// Three things make this more than "run the bundle in a browser":
//
//   * The module uses pthreads, so the page needs SharedArrayBuffer, which needs cross-origin
//     isolation. Karma serves the page, so the COOP/COEP headers come from here.
//   * The module is prelinked by the same emsdk that built the C API and is fetched at runtime
//     rather than bundled, so Karma has to serve it, its wasm, and its ABI manifest.
//   * Containers have no user namespaces and a small /dev/shm, so the launcher carries the same
//     flags scripts/run-browser-test.mjs passes for the C API suite.

(function configureMaplibreBrowserTests() {
  const fs = require("fs");
  const os = require("os");
  const path = require("path");

  // Written by the wasmJsBrowserTest task, which collects the module out of the browser build.
  const moduleDir = process.env.MLN_FFI_BROWSER_MODULE_DIR;
  if (!moduleDir) {
    throw new Error(
      "MLN_FFI_BROWSER_MODULE_DIR is unset, so Karma cannot serve the MapLibre Native browser " +
        "module. Run the wasmJs tests through :bindings:kotlin:wasmJsBrowserTest.",
    );
  }

  // Chromium comes from the environment where CI provides one, and otherwise from whatever is on
  // PATH or in a local Playwright cache. Same order as scripts/run-browser-test.mjs, so one host
  // setting names one browser for every browser suite in the repository.
  function findBrowser() {
    const explicit = process.env.CHROME_BIN;
    if (explicit) return explicit;
    const names = [
      "/usr/bin/google-chrome",
      "/usr/bin/google-chrome-stable",
      "/usr/bin/chromium",
      "/usr/bin/chromium-browser",
      "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
    ];
    for (const name of names) if (fs.existsSync(name)) return name;

    const cache = path.join(
      os.homedir(),
      process.platform === "darwin"
        ? "Library/Caches/ms-playwright"
        : ".cache/ms-playwright",
    );
    if (fs.existsSync(cache)) {
      for (const entry of fs.readdirSync(cache).sort().reverse()) {
        if (!entry.startsWith("chromium-")) continue;
        for (const relative of [
          // Playwright renamed this directory to chrome-linux64; older caches still
          // use chrome-linux, so both are tried rather than pinning either.
          "chrome-linux64/chrome",
          "chrome-linux/chrome",
          "chrome-mac/Chromium.app/Contents/MacOS/Chromium",
        ]) {
          const candidate = path.join(cache, entry, relative);
          if (fs.existsSync(candidate)) return candidate;
        }
      }
    }
    throw new Error(
      "no Chromium found; set MLN_FFI_TEST_BROWSER to a Chrome or Chromium binary",
    );
  }

  process.env.CHROME_BIN = findBrowser();

  // Cross-origin isolation, so the page may use SharedArrayBuffer. Every response carries them,
  // because the module's workers are documents of their own and lose the isolation without them.
  config.customHeaders = (config.customHeaders || []).concat([
    { match: ".*", name: "Cross-Origin-Opener-Policy", value: "same-origin" },
    {
      match: ".*",
      name: "Cross-Origin-Embedder-Policy",
      value: "require-corp",
    },
  ]);

  // A module served as anything but JavaScript is refused by the dynamic import that loads it, and
  // a wasm served as anything but application/wasm falls off the streaming compile path.
  config.mime = Object.assign({}, config.mime, {
    "text/javascript": ["mjs"],
    "application/wasm": ["wasm"],
    "application/json": ["json"],
  });

  // Served rather than included: the page loads the module itself, through the URL the binding is
  // given, so that the test exercises the same loader a host uses.
  config.files = (config.files || []).concat([
    {
      pattern: path.join(moduleDir, "*"),
      included: false,
      served: true,
      watched: false,
      nocache: true,
    },
  ]);
  // The prefix the tests name. Kept short and stable so the URL in BrowserTestSupport.kt reads as a
  // deployment path rather than as a Karma detail.
  config.proxies = Object.assign({}, config.proxies, {
    "/maplibre/": "/absolute" + moduleDir + "/",
  });

  // CI containers run as root without user namespaces available, and with a /dev/shm too small for
  // Chromium's default shared memory. Software WebGL2 covers a runner with no GPU.
  config.customLaunchers = Object.assign({}, config.customLaunchers, {
    MaplibreChromeHeadless: {
      base: "ChromeHeadless",
      flags: [
        "--no-sandbox",
        "--disable-dev-shm-usage",
        "--enable-unsafe-swiftshader",
      ],
    },
  });
  config.browsers = ["MaplibreChromeHeadless"];

  // Loading the module means fetching eight megabytes of wasm and starting a pthread pool, and a
  // suite whose first test does that has nothing to report until it finishes. The defaults are
  // thirty seconds, which the first test alone can exceed on a cold cache.
  config.captureTimeout = 300000;
  config.browserDisconnectTimeout = 60000;
  config.browserNoActivityTimeout = 300000;
  config.client = Object.assign({}, config.client, {
    mocha: Object.assign({}, (config.client || {}).mocha, { timeout: 120000 }),
  });
})();
