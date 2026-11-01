---
title: TypeScript
description: Add the TypeScript package to a project.
---

## Install

The TypeScript binding runs on Node.js, Bun, Deno, in browsers, in Hermes, and
on OpenHarmony. Each runtime takes one backend package, which holds the native
library for your [backend](/maplibre-native-ffi/install/#choose-a-backend).
Install from npm, or from OHPM on OpenHarmony, as the section for your runtime
shows.

## Node.js, Bun, and Deno

Add the binding and one backend package from
[npm](https://www.npmjs.com/package/@maplibre/native-ffi):
`@maplibre/native-ffi-opengl`, `-metal`, or `-vulkan`. It needs Node.js 24.11,
Bun 1.4.3, Deno 2.9.7, or later.

```bash frame="terminal"
npm install @maplibre/native-ffi @maplibre/native-ffi-vulkan
```

Deno imports the binding as `npm:@maplibre/native-ffi`, and needs
`--allow-ffi --allow-read --allow-env`.

The backend packages use MSVC on Windows. For MinGW on x64, install
`@maplibre/native-ffi-opengl-mingw` or `@maplibre/native-ffi-vulkan-mingw`
instead.

## Browser

Add the binding and `@maplibre/native-ffi-opengl`, which renders with WebGL 2,
or `@maplibre/native-ffi-webgpu`, which renders with WebGPU. Serve
`maplibre_native_ffi.mjs` and `maplibre_native_ffi.wasm` from the backend's
`-wasm32` package as static files, such as from Vite's `public` directory. Load
them before any other call:

```ts title="main.ts"
import { loadModule, RuntimeHandle } from "@maplibre/native-ffi";

await loadModule(new URL("/maplibre_native_ffi.mjs", location.href));
const runtime = await RuntimeHandle.create();
```

The page must meet the [web requirements](/maplibre-native-ffi/install/#web).

## Hermes

To embed the binding in a Hermes runtime, install the package and import its
Hermes entry:

```ts title="index.ts"
import { RuntimeHandle } from "@maplibre/native-ffi/hermes";
```

Your native host links the C library and the package's `native/hermes` CMake
target against its Hermes JSI library. It installs `mln::hermes::Module` before
it loads JavaScript, and supplies a scheduler for the runtime thread. The
package's `module.h` states the threading and lifetime rules. On Android, the
host also calls `mln_android_init` before it creates a runtime.

## OpenHarmony

Add one backend package from
[OHPM](https://ohpm.openharmony.cn/#/en/detail/@maplibre%2Fnative-ffi-opengl):
`@maplibre/native-ffi-opengl` or `@maplibre/native-ffi-vulkan`. On OpenHarmony,
the backend package holds the binding too, so import from it.

```bash frame="terminal"
ohpm install @maplibre/native-ffi-opengl
```

```ts title="Index.ets"
import { RuntimeHandle } from "@maplibre/native-ffi-opengl";

const runtime = await RuntimeHandle.create();
```

## Use your own build

npm has no snapshots. To try `main`, pack the packages for your preset in a
checkout of this repository, then install the tarballs:

```bash frame="terminal"
mise run //bindings/typescript:pack linux-gnu-x64-vulkan
npm install path/to/maplibre-native-ffi/bindings/typescript/build/npm/*.tgz
```

For OpenHarmony, pack and install the HAR instead:

```bash frame="terminal"
mise -E ohos run //bindings/typescript:pack-ohpm ohos-arm64-egl
ohpm install path/to/maplibre-native-ffi/bindings/typescript/build/ohpm/native-ffi-opengl-0.0.0.har
```
