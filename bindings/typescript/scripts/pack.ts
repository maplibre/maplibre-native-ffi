// Bundles the facade and packs the npm packages for one or more presets:
//
//   node scripts/pack.ts [--version <version>] <out dir> <preset>...
//
// A user installs the facade, @maplibre/native-ffi, and one backend package,
// such as @maplibre/native-ffi-vulkan. A backend package holds no code. It
// pins one addon package per platform through optionalDependencies, and each
// addon package declares the platform it serves, so a package manager installs
// only the addon for the host. Each addon package carries the addon that the
// build task installed for its preset, with the licenses of the native library
// that the addon embeds.
//
// Every package takes the same version, 0.0.0 unless --version names one, and
// every pin is exact, so the facade and the addon always come from one build.

import { execFileSync } from "node:child_process";
import {
  cpSync,
  mkdirSync,
  readFileSync,
  rmSync,
  writeFileSync,
} from "node:fs";
import { join, resolve } from "node:path";
import { parseArgs } from "node:util";

const { values, positionals } = parseArgs({
  options: { version: { type: "string", default: "0.0.0" } },
  allowPositionals: true,
});
const [outArgument, ...presets] = positionals;
if (outArgument === undefined || presets.length === 0) {
  throw new Error("usage: pack.ts [--version <version>] <out dir> <preset>...");
}
const version = values.version;
const out = resolve(outArgument);
const binding = join(import.meta.dirname, "..");
const root = join(binding, "..", "..");
const source = JSON.parse(readFileSync(join(binding, "package.json"), "utf8"));
const shared = {
  version,
  license: source.license,
  repository: source.repository,
  homepage: "https://maplibre.org/maplibre-native-ffi/",
  bugs: { url: "https://github.com/maplibre/maplibre-native-ffi/issues" },
};

// The loader in src/internal/loader.ts names addon packages the same way,
// from process.platform and process.arch.
const platforms: Record<string, { os: string; libc?: string }> = {
  "linux-gnu": { os: "linux", libc: "glibc" },
  macos: { os: "darwin" },
};
const titles: Record<string, string> = {
  opengl: "OpenGL",
  metal: "Metal",
  vulkan: "Vulkan",
};
const backends: Record<string, string> = {
  egl: "opengl",
  metal: "metal",
  vulkan: "vulkan",
};

interface Addon {
  readonly preset: string;
  readonly backend: string;
  readonly name: string;
  readonly os: string;
  readonly cpu: string;
  readonly libc: string | undefined;
}

// A coverage preset builds the addon of its base preset with instrumentation,
// so it packs under the same name for the suite to install.
function addonOf(preset: string): Addon {
  const match =
    /^(linux-gnu|macos)-(x64|arm64)-(egl|metal|vulkan)(?:-coverage)?$/.exec(
      preset,
    );
  if (match === null) {
    throw new Error(
      `${preset} has no addon package; only the Linux glibc and macOS presets do`,
    );
  }
  const { os, libc } = platforms[match[1]!]!;
  const backend = backends[match[3]!]!;
  const cpu = match[2]!;
  const suffix = libc === undefined ? "" : "-gnu";
  return {
    preset,
    backend,
    name: `${source.name}-${backend}-${os}-${cpu}${suffix}`,
    os,
    cpu,
    libc,
  };
}

const addons = presets.map(addonOf);

function pack(directory: string, manifest: object): void {
  writeFileSync(
    join(directory, "package.json"),
    `${JSON.stringify(manifest, null, 2)}\n`,
  );
  cpSync(join(root, "LICENSE"), join(directory, "LICENSE"));
  execFileSync("pnpm", ["pack", "--pack-destination", out], {
    cwd: directory,
    stdio: "inherit",
  });
}

// Node strips types only outside node_modules, so the published facade is the
// bundle rather than the sources.
const dist = join(binding, "build", "dist");
rmSync(dist, { recursive: true, force: true });
execFileSync(
  "pnpm",
  [
    "exec",
    "vp",
    "pack",
    "src/index.ts",
    "--no-config",
    "--dts",
    "--out-dir",
    dist,
  ],
  { cwd: binding, stdio: "inherit" },
);

const staging = join(binding, "build", "packages");
rmSync(staging, { recursive: true, force: true });
mkdirSync(out, { recursive: true });

for (const addon of addons) {
  const directory = join(staging, addon.name.split("/")[1]!);
  const install = join(binding, "native", "build", addon.preset, "install");
  mkdirSync(directory, { recursive: true });
  cpSync(
    join(install, "maplibre_native_ffi.node"),
    join(directory, "maplibre_native_ffi.node"),
  );
  cpSync(join(install, "licenses"), join(directory, "licenses"), {
    recursive: true,
  });
  pack(directory, {
    name: addon.name,
    ...shared,
    description: `The ${addon.preset} Node-API addon of ${source.name}.`,
    os: [addon.os],
    cpu: [addon.cpu],
    ...(addon.libc === undefined ? {} : { libc: [addon.libc] }),
    files: ["maplibre_native_ffi.node", "licenses"],
  });
}

for (const backend of new Set(addons.map((addon) => addon.backend))) {
  const directory = join(staging, backend);
  mkdirSync(directory, { recursive: true });
  pack(directory, {
    name: `${source.name}-${backend}`,
    ...shared,
    description: `The ${titles[backend]} backend of ${source.name}: the Node-API addon for each platform.`,
    optionalDependencies: Object.fromEntries(
      addons
        .filter((addon) => addon.backend === backend)
        .map((addon) => [addon.name, version]),
    ),
    files: [],
  });
}

const facade = join(staging, "facade");
cpSync(dist, join(facade, "dist"), { recursive: true });
cpSync(join(binding, "README.md"), join(facade, "README.md"));
// The facade loads whichever backend package the application installed. The
// peer pins keep that package at the facade's version, and let a strict
// layout such as pnpm's link it where the facade can resolve it.
const backendPeers = Object.fromEntries(
  Object.keys(titles).map((backend) => [`${source.name}-${backend}`, version]),
);
pack(facade, {
  name: source.name,
  ...shared,
  description: source.description,
  type: "module",
  exports: {
    ".": { types: "./dist/index.d.mts", default: "./dist/index.mjs" },
  },
  engines: source.engines,
  peerDependencies: backendPeers,
  peerDependenciesMeta: Object.fromEntries(
    Object.keys(backendPeers).map((name) => [name, { optional: true }]),
  ),
  files: ["dist"],
});
