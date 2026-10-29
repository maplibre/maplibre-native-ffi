// The wasm suite's page: cross-origin isolated, so the browser module's
// threads can share its memory, with the module served beside the tests.
//
// mise run //bindings/kotlin:test:wasm passes the module's directory through
// MLN_KOTLIN_WASM_MODULE_DIR, its render backend through
// MLN_KOTLIN_WASM_RENDER_BACKEND, and the browser through CHROME_BIN.
const moduleDir = process.env.MLN_KOTLIN_WASM_MODULE_DIR;
if (!moduleDir) {
  throw new Error(
    "MLN_KOTLIN_WASM_MODULE_DIR names no module directory; run mise run //bindings/kotlin:test:wasm",
  );
}

config.customHeaders = [
  { match: ".*", name: "Cross-Origin-Opener-Policy", value: "same-origin" },
  { match: ".*", name: "Cross-Origin-Embedder-Policy", value: "require-corp" },
];
// The suite bundles its fixture-bearing engine through the same import as a published runtime.
config.webpack = {
  ...config.webpack,
  resolve: {
    ...config.webpack?.resolve,
    alias: {
      ...config.webpack?.resolve?.alias,
      "@maplibre/native-ffi-runtime$": `${moduleDir}/maplibre-native-kotlin-test.mjs`,
    },
  },
};

// Each test bounds itself with TEST_TIMEOUT, 60 s, and each of its waits fails
// naming what it waited for, so Mocha's own limit, 2 s by default, sits above
// both and catches only a test that never settles. Karma ends a run whose page
// reports nothing for 30 s by default, which would end a test that waits out
// its own limit before it can fail, so that limit sits above Mocha's.
config.client = Object.assign({}, config.client);
config.client.mocha = Object.assign({}, config.client.mocha, {
  timeout: 90000,
});
config.browserNoActivityTimeout = 120000;

// Headless Chromium renders through SwiftShader where the host has no GPU it
// can use, with the flags that scripts/run-browser-test.mjs passes the native
// suite for the module's backend, which MLN_KOTLIN_WASM_RENDER_BACKEND names.
// The collection tests call globalThis.gc, which --expose-gc defines.
const renderBackend = process.env.MLN_KOTLIN_WASM_RENDER_BACKEND;
const backendFlags = {
  webgl: ["--enable-unsafe-swiftshader"],
  // Headless Chromium can use Metal on macOS. Forcing its Linux SwiftShader
  // Vulkan stack there prevents WebGPU device creation.
  webgpu:
    process.platform === "darwin"
      ? ["--enable-unsafe-webgpu"]
      : [
          "--enable-unsafe-webgpu",
          "--enable-features=Vulkan",
          "--enable-unsafe-swiftshader",
          "--use-angle=swiftshader",
          "--use-vulkan=swiftshader",
        ],
}[renderBackend];
if (!backendFlags) {
  throw new Error(
    `MLN_KOTLIN_WASM_RENDER_BACKEND names no known backend: ${renderBackend}`,
  );
}
config.browsers = ["ChromeHeadlessMaplibre"];
config.customLaunchers = Object.assign({}, config.customLaunchers, {
  ChromeHeadlessMaplibre: {
    base: "ChromeHeadless",
    flags: [...backendFlags, "--js-flags=--expose-gc"],
  },
});

// Serves each request under /maplibre-test-origin/ the text that its path
// encodes with encodeURIComponent, so a wasm test hands native loading an
// HTTP URL of the page's own origin that returns a body the test chose. See
// withLoopbackOrigin in Platform.wasmJs.kt.
const testOrigin = "/maplibre-test-origin/";
config.plugins = config.plugins || [];
config.plugins.push({
  "middleware:maplibreTestOrigin": [
    "value",
    (request, response, next) => {
      if (!request.url.startsWith(testOrigin)) return next();
      const body = decodeURIComponent(request.url.slice(testOrigin.length));
      response.setHeader("Content-Type", "application/json");
      response.end(body);
    },
  ],
});
config.beforeMiddleware = (config.beforeMiddleware || []).concat(
  "maplibreTestOrigin",
);
