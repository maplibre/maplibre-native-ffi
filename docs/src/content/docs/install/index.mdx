---
title: Install
description: Choose a backend, then add the package for your language.
sidebar:
  label: Overview
  order: 0
# Scope: what every language shares when installing: the backend choice and the
# platform requirements. Each language's setup lives on its own page.
---

Add the package for your language:

- [C](/maplibre-native-ffi/install/c/)
- [Dart](/maplibre-native-ffi/install/dart/)
- [.NET](/maplibre-native-ffi/install/dotnet/)
- [Go](/maplibre-native-ffi/install/go/)
- [Kotlin](/maplibre-native-ffi/install/kotlin/)
- [Python](/maplibre-native-ffi/install/python/)
- [Rust](/maplibre-native-ffi/install/rust/)
- [Swift](/maplibre-native-ffi/install/swift/)
- [TypeScript](/maplibre-native-ffi/install/typescript/)
- [Zig](/maplibre-native-ffi/install/zig/)

The [support matrix](/maplibre-native-ffi/#status) shows which platforms,
architectures, and backends each language supports.

## Choose a backend

The library renders with one graphics backend, which you choose when you install
it. Where a package has a default, it's Metal on Apple platforms, OpenGL on
Android and OpenHarmony, WebGPU in browsers, and Vulkan elsewhere. Choose OpenGL
in emulators and virtual machines, which often lack Vulkan, and choose the
backend your application already uses if it draws its own graphics.

## Web

The library runs on several threads, which in a browser share memory, and
browsers allow shared memory only on a cross-origin isolated page. Serve a page
that loads the library with these headers, which make it isolated:

```text title="HTTP response headers"
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

Most static hosts let you set them, such as with a
[`_headers` file](https://developers.cloudflare.com/pages/configuration/headers/)
on Cloudflare Pages and
[Netlify](https://docs.netlify.com/manage/routing/headers/), or
[`vercel.json`](https://vercel.com/docs/project-configuration/vercel-json) on
Vercel. GitHub Pages can't set headers, but a service worker such as
[coi-serviceworker](https://github.com/gzuidhof/coi-serviceworker) can add them.

The WebGPU backend also needs JSPI, which lets it wait for the browser's
asynchronous WebGPU calls. Not every browser supports JSPI yet; see
[WebAssembly features](https://webassembly.org/features/). Where it's missing,
use the OpenGL backend, which renders with WebGL and doesn't need it.

## macOS

macOS has neither Vulkan nor EGL. To use the Vulkan or OpenGL backend there,
load [MoltenVK](https://github.com/KhronosGroup/MoltenVK) or
[ANGLE](https://chromium.googlesource.com/angle/angle) yourself.
