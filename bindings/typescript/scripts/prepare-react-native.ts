// Stage the pinned stock app, shared suite, and reload probe. Both platforms
// use release bundles, with no development server.
import { execFileSync } from "node:child_process";
import {
  cpSync,
  existsSync,
  mkdirSync,
  readFileSync,
  rmSync,
  writeFileSync,
} from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "../../..");
const work = resolve(root, "build/react-native-spike");
const app = resolve(work, "app");
const fixture = resolve(root, "bindings/typescript/test/react-native");
const binding = resolve(root, "bindings/typescript");
const packages = resolve(work, "npm");
const preset = process.argv[2] ?? "ios-simulator-arm64-metal";
const prefix = resolve(root, "build", preset, "install");
mkdirSync(work, { recursive: true });
if (!existsSync(resolve(work, "template-0.88.1/package/template"))) {
  const archive = execFileSync(
    "npm",
    [
      "pack",
      "@react-native-community/template@0.88.1",
      "--silent",
      "--pack-destination",
      work,
    ],
    { encoding: "utf8" },
  ).trim();
  mkdirSync(resolve(work, "template-0.88.1"), { recursive: true });
  execFileSync("tar", [
    "-xzf",
    resolve(work, archive),
    "-C",
    resolve(work, "template-0.88.1"),
  ]);
}
cpSync(resolve(work, "template-0.88.1/package/template"), app, {
  recursive: true,
});
cpSync(resolve(fixture, "index.js"), resolve(app, "index.js"));
mkdirSync(resolve(app, "specs"), { recursive: true });
cpSync(
  resolve(fixture, "NativeLifecycleProbe.ts.in"),
  resolve(app, "specs/NativeLifecycleProbe.ts"),
);
cpSync(fixture, resolve(app, "probe"), { recursive: true });
cpSync(resolve(binding, "test/support"), resolve(app, "probe/support"), {
  recursive: true,
});
cpSync(resolve(root, "tests/graphics"), resolve(app, "probe/graphics"), {
  recursive: true,
});
cpSync(
  resolve(root, "third_party/maplibre-native/vendor/Vulkan-Headers/include"),
  resolve(app, "probe/vulkan"),
  { recursive: true },
);
execFileSync("pnpm", ["exec", "vp", "pack"], {
  cwd: fixture,
  env: {
    ...process.env,
    MAPLIBRE_NATIVE_FFI_REACT_NATIVE_BUNDLE: resolve(app, "suite"),
  },
  stdio: "inherit",
});
const manifestPath = resolve(app, "package.json");
const manifest = JSON.parse(readFileSync(manifestPath, "utf8"));
manifest.dependencies["@maplibre/native-ffi"] =
  `file:${resolve(packages, "maplibre-native-ffi-0.0.0.tgz")}`;
manifest.codegenConfig = {
  name: "LifecycleProbeSpec",
  type: "modules",
  jsSrcsDir: "specs",
  android: { javaPackageName: "org.maplibre.lifecycle" },
  ios: { modulesProvider: { LifecycleProbe: "LifecycleProbe" } },
};
manifest.swiftpmConfig = {
  modules: [{ name: "LifecycleProbe", path: "probe" }],
};
writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + "\n");

const packArgs = ["scripts/pack.ts"];
if (preset.startsWith("ios-")) {
  rmSync(resolve(work, "MaplibreNativeC.xcframework"), {
    recursive: true,
    force: true,
  });
  execFileSync(
    "xcodebuild",
    [
      "-create-xcframework",
      "-framework",
      resolve(prefix, "Frameworks/MaplibreNativeC.framework"),
      "-output",
      resolve(work, "MaplibreNativeC.xcframework"),
    ],
    { stdio: "inherit" },
  );
  packArgs.push("--xcframework", resolve(work, "MaplibreNativeC.xcframework"));
}
execFileSync("node", [...packArgs, packages, preset], {
  cwd: binding,
  stdio: "inherit",
});

function patch(path: string, before: string, after: string) {
  const target = resolve(app, path);
  const text = readFileSync(target, "utf8");
  if (!text.includes(before)) throw new Error(`Template changed: ${path}`);
  writeFileSync(target, text.replace(before, after));
}

patch(
  "android/app/src/main/java/com/helloworld/MainApplication.kt",
  "// add(MyReactNativePackage())",
  "add(org.maplibre.lifecycle.LifecycleProbePackage())",
);
const kotlin = resolve(app, "android/app/src/main/java/org/maplibre/lifecycle");
mkdirSync(kotlin, { recursive: true });
for (const name of ["LifecycleProbeModule.kt", "LifecycleProbePackage.kt"]) {
  cpSync(resolve(fixture, "android", name), resolve(kotlin, name));
}
patch(
  "android/app/build.gradle",
  "android {",
  `android {
    buildFeatures { prefab true }
    externalNativeBuild { cmake { path file("../../probe/android/CMakeLists.txt") } }
`,
);
patch(
  "android/build.gradle",
  'ndkVersion = "27.1.12297006"',
  `ndkVersion = "${process.env.MLN_FFI_ANDROID_NDK_VERSION ?? "30.0.16248370"}"`,
);
execFileSync(
  "npm",
  [
    "install",
    "--ignore-scripts",
    "--no-audit",
    "--no-fund",
    // An explicit request refreshes a rebuilt tarball at the same path.
    manifest.dependencies["@maplibre/native-ffi"],
  ],
  {
    cwd: app,
    stdio: "inherit",
  },
);
// Each test module owns its shim; the package's Node-API symbols stay private.
cpSync(
  resolve(
    app,
    "node_modules/@maplibre/native-ffi/native/react-native/shim.cpp",
  ),
  resolve(app, "probe/native/shim.cpp"),
);
console.log(app);
