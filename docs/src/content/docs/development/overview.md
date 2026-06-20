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

Read the
[Binding specification](/maplibre-native-ffi/development/binding-specification/)
before implementing or reviewing a binding.

## Getting Set Up

Install the platform toolchain:

- On macOS, install the Xcode version listed in `.xcode-version`, or a recent
  compatible Xcode. Run the pinned
  [`xcodes`](https://github.com/XcodesOrg/xcodes) tool with `xcodes select` to
  switch to the repository version.
- On Fedora, install the `C Development Tools and Libraries` package group with
  `sudo dnf group install c-development`.
- On Debian or Ubuntu, install the C and C++ compiler packages with
  `sudo apt install build-essential`.
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
# Build and test the C API
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

## How Tools Fit Together

This repository spans native code, language bindings, examples, tests, and
documentation. Each tool owns the layer where it has the clearest dependency
model.

Project-managed dependency state keeps builds reproducible. Mise supplies
repository tools, owns task execution, and exports the environment consumed by
build systems, bindings, and examples. Pixi currently supplies desktop native
libraries from [`conda-forge`](https://conda-forge.org/). Platform SDKs such as
Xcode, Visual Studio, Linux compiler packages, and the Android SDK remain host
toolchain inputs.

[`mise`](https://mise.jdx.dev/) is the contributor entrypoint. It pins top-level
tools, installs Git hooks, selects backend variants, and runs repository tasks.
Use `mise run ...` for common workflows: build, test, check, fix, and examples.
Mise also provides ecosystem entrypoints such as Pixi, Zig, Python, uv, Node,
and pnpm.

[`pixi`](https://pixi.sh/) installs the desktop native libraries used by local
builds. CMake, Ninja, pkg-config, shader tools, Doxygen, and clang-tidy are
pinned by mise. Host platform toolchains provide C and C++ compilers, and CMake
builds the native C/C++ library.

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
`mise run test` command builds the native library and runs the direct Zig C API
suite. Language binding suites run through their binding-specific CI tasks.

Use examples for demos and behavior that needs manual validation, such as visual
output, interactive input, or host graphics integration.

Keep examples small. This repository includes low-level language bindings and
focused integration examples. Full application SDKs live outside this
repository.
