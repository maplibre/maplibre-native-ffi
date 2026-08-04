// Leaves the browser binding's dynamic import alone.
//
// The binding instantiates its Emscripten module with `import(url)`, where the URL is whatever the
// host passes at runtime. Webpack reads a dynamic import as a build-time dependency, so it replaces
// the expression with its own resolver and the module is then looked for among the bundle's
// modules, where it has never been. Disabling the parsing leaves the call as the platform's own
// dynamic import, which resolves against the page.
config.module = config.module || {};
config.module.parser = Object.assign({}, config.module.parser, {
  javascript: Object.assign({}, (config.module.parser || {}).javascript, {
    import: false,
  }),
});
