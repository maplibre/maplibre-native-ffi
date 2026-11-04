const path = require("node:path");

const { getDefaultConfig, mergeConfig } = require("@react-native/metro-config");

const root = path.resolve(__dirname, "../..");

// The binding is a workspace package, and pnpm keeps every package in the
// repository's store, so Metro watches both. It skips the binding's builds and
// test fixtures.
const binding = path.join(root, "bindings/typescript");

/** @type {import('@react-native/metro-config').MetroConfig} */
module.exports = mergeConfig(getDefaultConfig(__dirname), {
  watchFolders: [path.join(root, "node_modules"), binding],
  resolver: {
    // Babel's helpers and React Native resolve from the app, even for code in
    // the binding.
    nodeModulesPaths: [path.join(__dirname, "node_modules")],
    blockList: [
      new RegExp(`^${escape(path.join(binding, "native"))}/.*`),
      new RegExp(`^${escape(path.join(binding, "test"))}/.*`),
      new RegExp(`^${escape(path.join(binding, "build"))}/.*`),
      /\/(android|ios)\/build\/.*/,
    ],
  },
});

function escape(value) {
  return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}
