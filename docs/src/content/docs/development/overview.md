---
title: Overview
description: Contributor setup, project scope, workflow commands, tests, and examples.
sidebar:
  order: 1
---

## Project Scope

The project exposes MapLibre Native through two layers.

The C API exposes core MapLibre Native features on supported native platforms:
runtime, resources, maps, cameras, events, diagnostics, logging, render target
primitives, texture readback, and low-level extension points such as resource
providers and URL transforms. It excludes convenience APIs such as snapshotting
and platform integrations such as gestures and device sensors.

Language bindings sit directly above the C API. In the target language, they
manage C handles, struct initialization, scoped lifetimes, status codes,
diagnostics, borrowed data, threading, and event draining. They preserve the C
API's concepts. Higher-level adapters may provide full SDKs, async models, view
lifecycle integrations, convenience workflows, or new abstractions.

Read the [Binding Conventions](/maplibre-native-ffi/development/bindings/)
before implementing or reviewing a binding.

## Getting Set Up

Install the platform toolchain:

- On macOS, install the Xcode version listed in `.xcode-version`, or a recent
  compatible Xcode. Run the pinned
  [`xcodes`](https://github.com/XcodesOrg/xcodes) tool with `xcodes select` to
  switch to the repository version.
- On Windows, install a recent version of Visual Studio Community (or Build
  Tools) 2022 with the `Desktop development with C++` workload and
  `Git for Windows`. We rely on Git Bash to run project scripts.

Install [`mise`](https://mise.jdx.dev/), then install pinned project tools and
run repository setup hooks:

```bash
mise install
```

The setup hooks install project dependencies and initialize the MapLibre Native
submodule at `third_party/maplibre-native`.

Run the Zig map example as a smoke test:

```bash
mise run //examples/zig-map:run
```

Mise selects the native build profile that matches your host when one is
available. Set `MISE_ENV=<variant>` before `mise run ...`, or pass
`mise -E <variant> run ...`, to build, test, or run examples for another
platform and render backend.

## Common Commands

```bash
# Build and test the C API (via Zig bindings)
mise run test

# Build only
mise run build

# Run linters and formatters
mise run fix

# Run examples
mise run //examples/zig-map:run

# Build the documentation site
mise run //docs:build
```

## CI Variant Matrix

GitHub Actions builds native artifacts and examples for the variants described
by the mise profiles, such as `.mise/config.linux-x64-vulkan.toml` and
`.mise/config.macos-arm64-metal.toml`. The CI matrix generator reads each
profile's platform, architecture, and render backend metadata.

Use `.github/config/variants.toml` to configure CI matrix policy: runner
selection, example tasks, compatibility requirements, and explicit exclusions.

Preview the generated matrices locally:

```bash
mise run ci:matrix native --pretty
mise run ci:matrix examples --pretty
```

## How Tools Fit Together

This repository spans native code, language bindings, examples, tests, and
documentation. Each tool owns the layer where it has the clearest dependency
model.

Project-managed dependency state keeps builds reproducible. Pixi supplies native
build dependencies instead of Homebrew, apt, or other machine-global package
managers. Platform SDKs such as Xcode, Visual Studio, and the Android SDK remain
pragmatic external exceptions.

[`mise`](https://mise.jdx.dev/) is the contributor entrypoint. It pins top-level
tools, installs Git hooks, and runs repository tasks. Use `mise run ...` for
common workflows: build, test, check, fix, and examples. Mise also provides
ecosystem entrypoints such as Pixi, Zig, Python, uv, Node, and pnpm.

[`pixi`](https://pixi.sh/) owns the C and C++ native build environment. It
supplies native packages from [`conda-forge`](https://conda-forge.org/):
libraries, C/C++ tooling, and build tools that participate in CMake
configuration or native package discovery. CMake, Ninja, pkg-config, shader
tools, and clang tooling live in Pixi for that reason. Pixi runs
[CMake](https://cmake.org/), and CMake builds the native C/C++ library.

Language package managers own dependencies inside their ecosystems. For example,
`uv` owns Python package dependencies, `pnpm` owns Node package dependencies,
Gradle owns Java and Kotlin dependencies, and Cargo owns Rust dependencies.
Language-specific formatters, linters, analyzers, test frameworks, and code
generators usually live with the language package graph they serve.

[`hk`](https://github.com/jdx/hk) orchestrates repository checks for pre-commit,
`mise run check`, and `mise run fix`. [`dprint`](https://dprint.dev/) owns
repository-wide formatting defaults.

[Astro](https://astro.build/) and [Starlight](https://starlight.astro.build/)
build the documentation site. Generated API reference HTML is installed into
`docs/public/reference/` before each docs build.

## Tests And Examples

Every feature needs automated CI coverage when practical. The root
`mise run test` command builds the native library, runs the direct Zig C API
suite, and runs the public Zig binding suite for supported host variants.

Use examples for demos and behavior that needs manual validation, such as visual
output, interactive input, or host graphics integration.

Keep examples small. This repository includes low-level language bindings and
focused integration examples. Full application SDKs live outside this
repository.
