// TypeDoc declares typescript as a peer, so pnpm would resolve it from this
// workspace's TypeScript 7. Move that peer to a regular dependency so the
// typedoc>typescript override in pnpm-workspace.yaml can isolate TypeDoc on
// the TypeScript 6 API. Drop this file with that override once TypeDoc
// supports TS 7, expected around the TS 7.1 release:
// https://github.com/TypeStrong/typedoc/issues/3098
module.exports = {
  hooks: {
    readPackage(pkg) {
      if (pkg.name === "typedoc" && pkg.peerDependencies?.typescript) {
        pkg.dependencies = {
          ...pkg.dependencies,
          typescript: pkg.peerDependencies.typescript,
        };
        delete pkg.peerDependencies.typescript;
      }
      return pkg;
    },
  },
};
