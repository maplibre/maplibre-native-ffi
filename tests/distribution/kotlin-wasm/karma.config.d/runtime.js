config.customHeaders = [
  { match: ".*", name: "Cross-Origin-Opener-Policy", value: "same-origin" },
  { match: ".*", name: "Cross-Origin-Embedder-Policy", value: "require-corp" },
];
config.client = {
  ...config.client,
  mocha: { ...config.client?.mocha, timeout: 90000 },
  maplibreBackend: process.env.MLN_BROWSER_BACKEND,
};
config.browserNoActivityTimeout = 120000;
