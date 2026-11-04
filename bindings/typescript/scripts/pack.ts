// Bundles the facade and packs the npm packages for one or more presets:
//
//   node scripts/pack.ts [--version <version>] [--xcframework <directory>] [--android <directory>] <out dir> <preset>...
//
// A desktop user installs the facade, @maplibre/native-ffi, and one backend package,
// such as @maplibre/native-ffi-vulkan. A backend package holds no code. It
// pins one addon package per platform through optionalDependencies, and each
// desktop addon package declares the platform it serves, so a package manager
// installs only the addon for the host. A browser addon package carries the
// module of an Emscripten preset, which no host platform selects, so it
// installs wherever its backend package does. Each addon package carries what
// the build task installed for its preset, with the licenses of the native
// library that it embeds.
//
// A React Native app installs @maplibre/native-ffi-react-native, which depends
// on the facade. It carries the binding's native sources, which the app's build
// compiles, with the XCFramework that --xcframework names and the Android EGL
// libraries in the native archives under the directory that --android names.
//
// Every package takes the same version, 0.0.0 unless --version names one, and
// every pin is exact, so the facade and the addon always come from one build.

import { execFileSync } from "node:child_process";
import {
  cpSync,
  mkdirSync,
  readFileSync,
  readdirSync,
  rmSync,
  writeFileSync,
} from "node:fs";
import { join, resolve } from "node:path";
import { parseArgs } from "node:util";

const { values, positionals } = parseArgs({
  options: {
    version: { type: "string", default: "0.0.0" },
    xcframework: { type: "string" },
    android: { type: "string" },
  },
  allowPositionals: true,
});
const [outArgument, ...presets] = positionals;
if (outArgument === undefined || presets.length === 0) {
  throw new Error(
    "usage: pack.ts [--version <version>] [--xcframework <directory>] [--android <directory>] <out dir> <preset>...",
  );
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
  "linux-musl": { os: "linux", libc: "musl" },
  macos: { os: "darwin" },
  freebsd: { os: "freebsd" },
  windows: { os: "win32" },
  mingw: { os: "win32" },
};
const titles: Record<string, string> = {
  opengl: "OpenGL",
  metal: "Metal",
  vulkan: "Vulkan",
  webgpu: "WebGPU",
  "opengl-mingw": "OpenGL (MinGW)",
  "vulkan-mingw": "Vulkan (MinGW)",
};
// WebGL is the OpenGL backend with the browser's context provider.
// The Android ABI of each architecture that the presets name.
const androidAbis: Record<string, string> = {
  arm: "armeabi-v7a",
  arm64: "arm64-v8a",
  x86: "x86",
  x64: "x86_64",
};
const backends: Record<string, string> = {
  egl: "opengl",
  wgl: "opengl",
  metal: "metal",
  vulkan: "vulkan",
  webgl: "opengl",
  webgpu: "webgpu",
};

interface Addon {
  readonly preset: string;
  readonly backend: string;
  readonly name: string;
  /** The platform that a desktop addon serves; a browser module has none. */
  readonly platform:
    | { readonly os: string; readonly cpu: string; readonly libc?: string }
    | undefined;
  readonly files: readonly string[];
}

function addonOf(preset: string): Addon {
  const browser = /^emscripten-wasm32-(webgl|webgpu)$/.exec(preset);
  if (browser !== null) {
    const backend = backends[browser[1]!]!;
    return {
      preset,
      backend,
      name: `${source.name}-${backend}-wasm32`,
      platform: undefined,
      files: ["maplibre_native_ffi.mjs", "maplibre_native_ffi.wasm"],
    };
  }
  const match =
    /^(linux-gnu|linux-musl|macos|freebsd|windows|mingw)-(x64|arm64|riscv64|loongarch64)-(egl|wgl|metal|vulkan)$/.exec(
      preset,
    );
  if (match === null) {
    throw new Error(
      `${preset} has no addon package; only the Linux, macOS, FreeBSD, Windows, and Emscripten presets do`,
    );
  }
  const { os, libc } = platforms[match[1]!]!;
  const mingw = match[1] === "mingw";
  const backend = `${backends[match[3]!]!}${mingw ? "-mingw" : ""}`;
  const cpu = match[2] === "loongarch64" ? "loong64" : match[2]!;
  const suffix = libc === undefined ? "" : libc === "musl" ? "-musl" : "-gnu";
  return {
    preset,
    backend,
    name: `${source.name}-${backend}-${os}-${cpu}${suffix}`,
    platform: { os, cpu, ...(libc === undefined ? {} : { libc }) },
    // The addon links the shared C library, which loads from beside it, as do
    // the C++ runtime DLLs that a Windows build carries.
    files: [
      "maplibre_native_ffi.node",
      os === "win32"
        ? "maplibre-native-c.dll"
        : os === "darwin"
          ? "libmaplibre-native-c.dylib"
          : "libmaplibre-native-c.so",
      ...(os === "win32"
        ? readdirSync(
            join(binding, "native", "build", preset, "install"),
          ).filter(
            (file) => file.endsWith(".dll") && file !== "maplibre-native-c.dll",
          )
        : []),
    ],
  };
}

// Mobile apps compile the addon from the React Native package's sources.
const addons = presets
  .filter((preset) => !/^(android|ios|tvos|visionos)-/.test(preset))
  .map(addonOf);

function pack(
  directory: string,
  manifest: object,
  inventories: string[] = [],
  npm: string[] = [],
): void {
  const data = manifest as {
    name: string;
    files: string[];
    dependencies?: Record<string, string>;
    optionalDependencies?: Record<string, string>;
    peerDependencies?: Record<string, string>;
  };
  execFileSync(
    "python3",
    [
      join(root, "tools/release/binding_sbom.py"),
      "package",
      "--name",
      data.name,
      "--version",
      version,
      "--ecosystem",
      "npm",
      "--output",
      join(directory, "sbom/maplibre-native-ffi.cdx.json"),
      ...inventories.flatMap((file) => ["--input", file]),
      ...npm.flatMap((file) => ["--npm", file]),
      ...Object.entries(data.dependencies ?? {}).flatMap(([name, version]) => [
        "--dependency",
        `${name}@${version}`,
      ]),
      ...Object.entries({
        ...data.optionalDependencies,
        ...data.peerDependencies,
      }).flatMap(([name, version]) => [
        "--optional-dependency",
        `${name}@${version}`,
      ]),
    ],
    { stdio: "inherit" },
  );
  data.files.push("sbom", "licenses");
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
// bundle rather than the sources: entries for Node, browser, and Hermes
// share their chunks. The browser entry bundles emnapi's
// runtime, a development dependency, so the facade depends on nothing.
const dist = join(binding, "build", "dist");
rmSync(dist, { recursive: true, force: true });
execFileSync(
  "pnpm",
  [
    "exec",
    "vp",
    "pack",
    "src/index.ts",
    "src/browser.ts",
    "src/hermes.ts",
    "--no-config",
    // The browser entry runs where no require() exists, which the node
    // platform would bring through node:module.
    "--platform",
    "neutral",
    "--target",
    "es2021",
    "--fixed-extension",
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
  for (const file of addon.files) {
    cpSync(join(install, file), join(directory, file));
  }
  cpSync(join(install, "licenses"), join(directory, "licenses"), {
    recursive: true,
  });
  const { platform } = addon;
  pack(
    directory,
    {
      name: addon.name,
      ...shared,
      description:
        platform === undefined
          ? `The ${addon.preset} browser module of ${source.name}.`
          : `The ${addon.preset} Node-API addon of ${source.name}.`,
      ...(platform && {
        os: [platform.os],
        cpu: [platform.cpu],
        ...(platform.libc === undefined ? {} : { libc: [platform.libc] }),
      }),
      files: [...addon.files],
    },
    [join(install, "sbom/maplibre-native-ffi.cdx.json")],
  );
}

for (const backend of new Set(addons.map((addon) => addon.backend))) {
  const directory = join(staging, backend);
  mkdirSync(directory, { recursive: true });
  pack(directory, {
    name: `${source.name}-${backend}`,
    ...shared,
    description: `The ${titles[backend]} backend of ${source.name}: the native module for each platform.`,
    optionalDependencies: Object.fromEntries(
      addons
        .filter((addon) => addon.backend === backend)
        .map((addon) => [addon.name, version]),
    ),
    files: [],
  });
}

// The binding's native sources, which native embedding hosts compile. C sources
// stay separate from C++.
function copyNativeSources(directory: string): void {
  for (const name of [
    "generated_addon.c",
    "generated_values.c",
    "generated_addon.h",
    "runtime.c",
    "runtime.h",
    "realm.c",
    "mutex.h",
    "hermes",
  ]) {
    cpSync(join(binding, "native", name), join(directory, "native", name), {
      recursive: true,
    });
  }
  cpSync(join(root, "include"), join(directory, "native/include"), {
    recursive: true,
  });
  cpSync(
    join(binding, "node_modules/node-api-headers/include"),
    join(directory, "native/node-api-headers"),
    { recursive: true },
  );
  cpSync(
    join(binding, "node_modules/node-api-headers/LICENSE"),
    join(directory, "native/node-api-headers/LICENSE"),
  );
}

const facade = join(staging, "facade");
cpSync(dist, join(facade, "dist"), { recursive: true });
cpSync(join(binding, "README.md"), join(facade, "README.md"));
copyNativeSources(facade);
// The facade loads whichever backend package the application installed. The
// peer pins keep that package at the facade's version, and let a strict
// layout such as pnpm's link it where the facade can resolve it.
const backendPeers = Object.fromEntries(
  Object.keys(titles).map((backend) => [`${source.name}-${backend}`, version]),
);
pack(
  facade,
  {
    name: source.name,
    ...shared,
    description: source.description,
    type: "module",
    // A bundler that builds for the web takes the browser entry, and Node, Bun,
    // and Deno take the default.
    exports: {
      ".": {
        browser: {
          types: "./dist/browser.d.mts",
          default: "./dist/browser.mjs",
        },
        default: { types: "./dist/index.d.mts", default: "./dist/index.mjs" },
      },
      "./hermes": {
        types: "./dist/hermes.d.mts",
        default: "./dist/hermes.mjs",
      },
    },
    engines: source.engines,
    peerDependencies: backendPeers,
    peerDependenciesMeta: Object.fromEntries(
      Object.keys(backendPeers).map((name) => [name, { optional: true }]),
    ),
    files: ["dist", "native"],
  },
  [],
  [
    join(binding, "node_modules/@emnapi/runtime"),
    join(binding, "node_modules/node-api-headers"),
  ],
);

// The React Native package, whose podspec and Gradle module compile the
// binding's native sources beside the package's own against the C library.
// A desktop or browser build has no native libraries to put in it.
function packReactNative(): void {
  const reactNative = join(binding, "react-native");
  const reactNativeSource = JSON.parse(
    readFileSync(join(reactNative, "package.json"), "utf8"),
  );
  const reactNativeStaging = join(staging, "react-native");
  for (const name of [
    "README.md",
    "MaplibreNativeFfiReactNative.podspec",
    "react-native.config.js",
    "src",
    "cpp",
    "android/build.gradle",
    "android/CMakeLists.txt",
    "android/src",
  ]) {
    cpSync(join(reactNative, name), join(reactNativeStaging, name), {
      recursive: true,
    });
  }
  for (const name of readdirSync(join(reactNative, "ios"))) {
    if (/\.(h|mm)$/.test(name)) {
      cpSync(
        join(reactNative, "ios", name),
        join(reactNativeStaging, "ios", name),
      );
    }
  }
  copyNativeSources(reactNativeStaging);
  const reactNativeInventories: string[] = [];
  if (values.xcframework) {
    const xcframework = join(
      reactNativeStaging,
      "ios/MaplibreNativeC.xcframework",
    );
    cpSync(resolve(values.xcframework), xcframework, {
      recursive: true,
      dereference: true,
    });
    execFileSync(
      "python3",
      [
        join(root, "tools/release/binding_sbom.py"),
        "framework",
        xcframework,
        version,
      ],
      { stdio: "inherit" },
    );
    reactNativeInventories.push(
      ...readdirSync(xcframework, { recursive: true, withFileTypes: true })
        .filter((entry) => entry.isFile() && entry.name === "sbom.cdx.json")
        .map((entry) => join(entry.parentPath, entry.name)),
    );
  }
  if (values.android) {
    // The native archives of the Android EGL presets, as the release publishes
    // them.
    const archives = readdirSync(resolve(values.android), {
      recursive: true,
      withFileTypes: true,
    }).flatMap((entry) => {
      const match =
        /^maplibre-native-c-android-(arm|arm64|x86|x64)-egl\.tar\.gz$/.exec(
          entry.name,
        );
      return entry.isFile() && match !== null
        ? [{ arch: match[1]!, path: join(entry.parentPath, entry.name) }]
        : [];
    });
    if (archives.length === 0) {
      throw new Error(`${values.android} holds no Android EGL native archive`);
    }
    for (const { arch, path } of archives) {
      const extracted = join(staging, "android-archives", arch);
      mkdirSync(extracted, { recursive: true });
      execFileSync("tar", ["-xzf", path, "-C", extracted]);
      const prefix = join(extracted, `maplibre-native-c-android-${arch}-egl`);
      cpSync(
        join(prefix, "lib/libmaplibre-native-c.so"),
        join(
          reactNativeStaging,
          "android/libs",
          androidAbis[arch]!,
          "libmaplibre-native-c.so",
        ),
      );
      cpSync(
        join(prefix, "share/maplibre-native-c/licenses"),
        join(reactNativeStaging, "licenses"),
        { recursive: true },
      );
      reactNativeInventories.push(
        join(prefix, "share/maplibre-native-c/sbom.cdx.json"),
      );
    }
  }
  pack(
    reactNativeStaging,
    {
      name: reactNativeSource.name,
      ...shared,
      repository: {
        ...source.repository,
        directory: "bindings/typescript/react-native",
      },
      description: reactNativeSource.description,
      main: "src/index.ts",
      dependencies: { [source.name]: version },
      peerDependencies: reactNativeSource.peerDependencies,
      codegenConfig: reactNativeSource.codegenConfig,
      files: [
        "src",
        "cpp",
        "ios",
        "android",
        "native",
        "MaplibreNativeFfiReactNative.podspec",
        "react-native.config.js",
      ],
    },
    reactNativeInventories,
  );
}
if (values.xcframework !== undefined || values.android !== undefined) {
  packReactNative();
}
