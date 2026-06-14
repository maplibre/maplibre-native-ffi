# dotnet-map Branch Checklist

Branch goal: add `examples/dotnet-map`, a C# `*-map` example analogous to
`examples/lwjgl-map` and conforming to
`docs/src/content/docs/development/specifications/map-example.md`.

## Context

These are settled branch decisions, not implementation steps:

- `dotnet-map` is a low-level graphics-host example. It exercises the C#
  binding, host graphics handles, and all three render-target modes from
  `map-example.md`.
- Future Avalonia, MAUI, Veldrid, or Compose Multiplatform examples should
  probably use a separate UI-embedded spec profile and may focus on one
  texture/view embedding path instead of all three render-target modes.
- The example targets plain `net10.0`, matching the C# binding conventions and
  the root `core:dotnet = "10.0.203"` pin in `mise.toml`.
- The graphics/window stack is Silk.NET + GLFW: `Silk.NET.GLFW`,
  `Silk.NET.OpenGL`, `Silk.NET.OpenGLES`, and `Silk.NET.Vulkan`.
- macOS/Metal uses GLFW Cocoa native access plus a minimal local C# Objective-C
  runtime helper. Do not use `net10.0-macos` or `dotnet/macios` for the first
  implementation.
- Do not use NObjective, Monobjc, CocoaSharp, or Dumbarton. They are too old or
  not viable as current dependencies.
- Keep SharpMetal out of the first pass. It can be revisited if the Metal
  compositor grows enough to justify the extra dependency.
- C# formatting is project-wide and should be rooted with the rest of the
  repository formatting configuration.
- ClangSharp generation is binding-specific and should remain scoped to
  `bindings/dotnet`.

References:

- Map example spec:
  `docs/src/content/docs/development/specifications/map-example.md`
- C# binding conventions: `docs/src/content/docs/development/bindings-csharp.md`
- Project overview: `docs/src/content/docs/development/overview.md`
- NuGet Central Package Management:
  `https://learn.microsoft.com/en-us/nuget/consume-packages/central-package-management`
- Silk.NET: `https://github.com/dotnet/Silk.NET`
- GLFW native access: `https://www.glfw.org/docs/latest/group__native.html`
- dotnet/macios: `https://github.com/dotnet/macios`

## Phase 1: .NET Project Configuration

- [x] Add root `Directory.Packages.props` with NuGet Central Package Management
      enabled.
  - Grounding: package manager config belongs at the repo root for this branch.
  - Include
    `<ManagePackageVersionsCentrally>true</ManagePackageVersionsCentrally>`.
  - Add central versions for xUnit/test SDK packages currently in the C# test
    project.
  - Add central versions for Silk.NET packages used by the example.

- [x] Add a repo-root .NET tool manifest for CSharpier.
  - Grounding: C# formatting applies to both `bindings/dotnet` and
    `examples/dotnet-map`.
  - Use a root `dotnet-tools.json` unless the branch also updates tooling to use
    the SDK default `.config/dotnet-tools.json` path.
  - Include only project-wide .NET tools such as `csharpier`.

- [x] Remove CSharpier from `bindings/dotnet/dotnet-tools.json`.
  - Grounding: `bindings/dotnet/dotnet-tools.json` should remain scoped to
    binding-specific generation tools.
  - Keep `clangsharppinvokegenerator` there.

- [x] Update root `mise.toml` postinstall to restore repo-root .NET tools.
  - Grounding: root setup should install the project-wide C# formatter.
  - Add a root
    `dotnet tool restore --tool-manifest dotnet-tools.json --verbosity quiet`
    step before or alongside the binding generator tool restore.

- [x] Keep the binding-specific ClangSharp tool restore in root `mise.toml`.
  - Grounding: `bindings/dotnet/scripts/generate-clangsharp.sh` runs
    `dotnet tool restore` in `bindings/dotnet` before generation.
  - Preserve the existing binding-scoped restore behavior for
    `clangsharppinvokegenerator`.
  - Prefer an explicit
    `dotnet tool restore --tool-manifest bindings/dotnet/dotnet-tools.json --verbosity quiet`
    command from the repo root.

- [x] Update `dprint.jsonc` to run CSharpier from the repository root.
  - Grounding: C# formatting should apply uniformly to binding and example C#
    files.
  - Change the C# formatter `cwd` from `bindings/dotnet` to the root context.
  - Update CSharpier `cacheKeyFiles` to the repo-root tool manifest.

- [x] Remove package `Version` attributes from existing C# test project
      references.
  - Grounding: NuGet Central Package Management uses versionless
    `PackageReference` entries in projects and central `PackageVersion` entries
    in `Directory.Packages.props`.

- [x] Create the `examples/dotnet-map` directory.
  - Grounding: examples live under `examples/` and keep their own mise tasks.

- [x] Add `examples/dotnet-map/Maplibre.Native.Examples.DotnetMap.csproj`.
  - Grounding: `map-example.md#what-an-example-is-not`.
  - Configure it as an executable, `net10.0`, nullable enabled, unsafe allowed,
    and non-packable.
  - Add a project reference to
    `../../bindings/dotnet/src/Maplibre.Native/Maplibre.Native.csproj`.
  - Add versionless package references for the Silk.NET packages used by the
    example.

- [x] Add an example-local `.gitignore` for .NET build output.
  - Grounding: generated `bin/` and `obj/` directories should stay out of git.

- [x] Add a repo-root `Maplibre.Native.slnx`.
  - Grounding: `map-example.md#architecture`.
  - Include:
    - `bindings/dotnet/src/Maplibre.Native/Maplibre.Native.csproj`
    - `bindings/dotnet/tests/Maplibre.Native.Tests/Maplibre.Native.Tests.csproj`
    - `examples/dotnet-map/Maplibre.Native.Examples.DotnetMap.csproj`

- [x] Update `bindings/dotnet/mise.toml` to build from the repo-root solution.
  - Grounding: the root solution is the branch's .NET build entrypoint.
  - Build command should run from `{{env.MLN_FFI_REPO_ROOT}}`.
  - Preserve `-p:Platform="Any CPU"`.

- [x] Update `bindings/dotnet/mise.toml` to test with root-relative project
      paths.
  - Grounding: binding test task should keep working after moving solution
    ownership to the repository root.
  - Test command should run from `{{env.MLN_FFI_REPO_ROOT}}`.
  - Preserve `-p:Platform="Any CPU"`.

- [x] Remove `bindings/dotnet/Maplibre.Native.slnx` after root solution tasks
      work.
  - Grounding: avoid two competing authoritative .NET solutions in the branch.

- [x] Add `examples/dotnet-map/mise.toml`.
  - Grounding: `map-example.md#command-line-interface`.
  - Provide `build`, `run`, `run:owned-texture`, `run:borrowed-texture`, and
    `run:native-surface`.
  - Make build/run tasks depend on `//:ensure-native-library`.
  - Run tasks from `{{env.MLN_FFI_REPO_ROOT}}` so root solution and central
    package config are used.

## Phase 2: Source Skeleton and Contracts

- [ ] Create the C# module files matching the spec's logical modules.
  - Grounding: `map-example.md#logical-modules`.
  - Initial files:
    - `Program.cs`
    - `Shell.cs`
    - `Viewport.cs`
    - `MapState.cs`
    - `InputController.cs`
    - `RenderTargetMode.cs`
    - `IRenderTarget.cs`
    - `IGraphicsContext.cs`
    - `OpenGLContext.cs`
    - `VulkanContext.cs`
    - `MetalContext.cs`
    - backend compositor files
    - `MacObjectiveC.cs`

- [ ] Define `RenderTargetMode` with the three required CLI values.
  - Grounding: `map-example.md#render-target-selection`.
  - Values: `owned-texture`, `borrowed-texture`, `native-surface`.
  - Store the exact required startup status line for each mode.

- [ ] Define the `IGraphicsContext` contract for backend-level resources.
  - Grounding: `map-example.md#graphics-api-and-mode-matrix`.
  - Include backend identity, window handle/accessors, resize hook, per-frame
    maintenance hook, and shutdown.

- [ ] Define the `IRenderTarget` contract for render-session and mode resources.
  - Grounding: `map-example.md#render-target-modes`.
  - Include render update/draw behavior, resize behavior,
    `NeedsReattachOnResize`, and shutdown.

- [ ] Implement separate factories for graphics API selection and render-target
      mode selection.
  - Grounding: `map-example.md#graphics-api-and-mode-matrix`.
  - `IGraphicsContext.Create` selects Metal/Vulkan/OpenGL.
  - render-target factories dispatch on both active backend and CLI mode.

## Phase 3: Graphics Contexts

- [ ] Implement GLFW lifecycle shared by all backends.
  - Grounding: `map-example.md#startup`, `map-example.md#shutdown`.
  - Initialize GLFW, create a resizable `960 x 640` window, expose event
    polling, and terminate GLFW during graphics shutdown.

- [ ] Implement OpenGL/EGL context creation on Linux.
  - Grounding: `map-example.md#opengl--egl--wgl`.
  - Create a GLFW OpenGL ES/EGL context.
  - Expose EGL display/config/context and EGL surface for C# render descriptors.
  - Initialize Silk.NET OpenGLES bindings for compositor calls.

- [ ] Implement OpenGL/WGL context creation on Windows.
  - Grounding: `map-example.md#opengl--egl--wgl`.
  - Create a GLFW OpenGL context.
  - Expose WGL device/context handles and window surface for C# render
    descriptors.
  - Initialize Silk.NET OpenGL bindings for compositor calls.

- [ ] Implement Vulkan instance, surface, device, and queue creation.
  - Grounding: `map-example.md#vulkan`.
  - Use Silk.NET Vulkan and GLFW Vulkan helpers.
  - Enable required GLFW instance extensions.
  - Pick a physical device and graphics-present queue family.
  - Expose instance, physical device, device, queue, queue family,
    `vkGetInstanceProcAddr`, `vkGetDeviceProcAddr`, and `VkSurfaceKHR`.

- [ ] Implement the macOS Objective-C runtime helper.
  - Grounding: `map-example.md#metal`.
  - Provide minimal helpers for class lookup, selector lookup, `objc_msgSend`
    variants, retain/release, autorelease pool, CoreFoundation strings,
    framework loading, and `MTLCreateSystemDefaultDevice`.

- [ ] Implement Metal context creation on macOS.
  - Grounding: `map-example.md#metal`.
  - Create a GLFW no-API window.
  - Get the Cocoa view from GLFW native access.
  - Create an `MTLDevice`.
  - Create and attach a `CAMetalLayer`.
  - Set layer device, pixel format, opacity, and drawable size.
  - Expose raw device and layer pointers for C# render descriptors.

- [ ] Implement graphics-context resize hooks for each backend.
  - Grounding: `map-example.md#resize`.
  - OpenGL: ensure context is current and update any surface state.
  - Vulkan: update swapchain/presentation resources as required.
  - Metal: update `CAMetalLayer.drawableSize`.

- [ ] Implement graphics-context shutdown for each backend.
  - Grounding: `map-example.md#shutdown`.
  - Wait for in-flight GPU work where required.
  - Release backend resources before destroying the window.

## Phase 4: Viewport

- [ ] Implement `Viewport` with logical size, physical size, and scale factor.
  - Grounding: `map-example.md#viewport`.

- [ ] Read the initial viewport after window creation.
  - Grounding: `map-example.md#window`, `map-example.md#viewport`.
  - Use GLFW window size, framebuffer size, and content scale.

- [ ] Compute logical dimensions from physical size and scale when needed.
  - Grounding: `map-example.md#viewport`.
  - Use `ceil(physical / scale)` with minimum `1`.

- [ ] Log initial viewport values.
  - Grounding: `map-example.md#viewport`.
  - Format includes `logical=... physical=... scale=...`.

- [ ] Register GLFW window size, framebuffer size, and content-scale callbacks.
  - Grounding: `map-example.md#resize`.

- [ ] Recompute and log viewport changes from resize callbacks.
  - Grounding: `map-example.md#viewport`, `map-example.md#resize`.

- [ ] Add a viewport-empty guard to the frame loop.
  - Grounding: `map-example.md#resize`.
  - Skip render updates while the viewport extent is empty.

## Phase 5: CLI, Startup, and Diagnostics

- [ ] Implement exact CLI parsing in `Program.cs`.
  - Grounding: `map-example.md#command-line-interface`.
  - Accept exactly one positional mode argument.
  - Support only `--help`.
  - Print usage and exit `0` for `--help`.
  - Print usage and exit `1` for invalid arguments.
  - Do all of this before creating a window.

- [ ] Load the native library through the existing C# loader.
  - Grounding: `map-example.md#startup`.
  - Project grounding:
    `bindings/dotnet/src/Maplibre.Native/Internal/Loader/NativeLibraryLoader.cs`
    already resolves `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH` and `MLN_FFI_BUILD_DIR`.
  - Call `Maplibre.LoadNativeLibrary()` before querying backend support.

- [ ] Query and print the loaded library's supported render backend mask.
  - Grounding: `map-example.md#startup`.
  - Use `Maplibre.SupportedRenderBackends()`.

- [ ] Validate that the loaded native library supports a usable backend.
  - Grounding: `map-example.md#startup`.
  - Fail fast with a readable message before creating graphics resources.

- [ ] Implement backend preference order in `IGraphicsContext.Create`.
  - Grounding: `map-example.md#implementations`, `map-example.md#graphics-api`.
  - macOS: prefer Metal, then Vulkan.
  - Linux/Windows: prefer OpenGL, then Vulkan.

- [ ] Register a native log callback during startup.
  - Grounding: `map-example.md#diagnostics`.
  - Print concise MapLibre log records to stderr.

- [ ] Clear the native log callback during shutdown.
  - Grounding: `map-example.md#diagnostics`.
  - Use `finally` so fatal setup/runtime failures still clear process-global
    logging state.

- [ ] Print the required active render-target startup lines.
  - Grounding: `map-example.md#startup-status-lines`.
  - Print `render target: <mode>`.
  - Print exactly one required `render target status: ...` line.

## Phase 6: Render Targets and Compositors

- [ ] Implement the shared render-target factory.
  - Grounding: `map-example.md#render-target-modes`.
  - Dispatch by active graphics backend and selected render-target mode.

- [ ] Implement `owned-texture` attach for Metal.
  - Grounding: `map-example.md#owned-texture`, `map-example.md#metal`.
  - Use `MetalOwnedTextureDescriptor` with the shared device/context.

- [ ] Implement `owned-texture` attach for Vulkan.
  - Grounding: `map-example.md#owned-texture`, `map-example.md#vulkan`.
  - Use `VulkanOwnedTextureDescriptor` with shared Vulkan handles.

- [ ] Implement `owned-texture` attach for OpenGL.
  - Grounding: `map-example.md#owned-texture`,
    `map-example.md#opengl--egl--wgl`.
  - Use `OpenGLOwnedTextureDescriptor` with EGL or WGL context descriptor.

- [ ] Implement `owned-texture` frame acquisition and release.
  - Grounding: `map-example.md#owned-texture`.
  - Acquire the backend frame after successful render update.
  - Draw it through the compositor.
  - Release/close the frame before the next render-session operation.

- [ ] Implement `borrowed-texture` host texture creation for Metal.
  - Grounding: `map-example.md#borrowed-texture`, `map-example.md#metal`.
  - Create an exportable `MTLTexture` sized to the physical viewport.

- [ ] Implement `borrowed-texture` host image creation for Vulkan.
  - Grounding: `map-example.md#borrowed-texture`, `map-example.md#vulkan`.
  - Create exportable `VkImage` and `VkImageView` sized to the physical
    viewport.

- [ ] Implement `borrowed-texture` host texture creation for OpenGL.
  - Grounding: `map-example.md#borrowed-texture`,
    `map-example.md#opengl--egl--wgl`.
  - Create a GL texture sized to the physical viewport.

- [ ] Implement `borrowed-texture` attach for each backend.
  - Grounding: `map-example.md#borrowed-texture`.
  - Use the matching C# borrowed-texture descriptor.
  - Return `NeedsReattachOnResize = true`.

- [ ] Implement `native-surface` attach for Metal.
  - Grounding: `map-example.md#native-surface`, `map-example.md#metal`.
  - Use `MetalSurfaceDescriptor` with the `CAMetalLayer`.

- [ ] Implement `native-surface` attach for Vulkan.
  - Grounding: `map-example.md#native-surface`, `map-example.md#vulkan`.
  - Use `VulkanSurfaceDescriptor` with `VkSurfaceKHR`.

- [ ] Implement `native-surface` attach for OpenGL.
  - Grounding: `map-example.md#native-surface`,
    `map-example.md#opengl--egl--wgl`.
  - Use `OpenGLSurfaceDescriptor` with EGL surface or WGL device context.

- [ ] Add a native-surface render path that bypasses compositor draw.
  - Grounding: `map-example.md#native-surface`.

- [ ] Implement the fullscreen triangle compositor for OpenGL.
  - Grounding: `map-example.md#compositor-shaders-texture-modes`.
  - Use pass-through UVs and straight texture copy.

- [ ] Implement the fullscreen triangle compositor for Vulkan.
  - Grounding: `map-example.md#compositor-shaders-texture-modes`.
  - Use pass-through UVs and straight texture copy.

- [ ] Implement the fullscreen triangle compositor for Metal.
  - Grounding: `map-example.md#compositor-shaders-texture-modes`.
  - Compile MSL source, create command queue, create render pipeline, draw into
    the next `CAMetalDrawable`.

- [ ] Implement render-target resize behavior for each mode.
  - Grounding: `map-example.md#resize`.
  - `borrowed-texture`: destroy and recreate mode resources, then reattach.
  - `owned-texture`: resize session and compositor resources in place.
  - `native-surface`: resize session and presentation resources in place unless
    the platform provides a new surface handle.

- [ ] Implement render-target shutdown for each mode.
  - Grounding: `map-example.md#shutdown`.
  - Release compositor resources and borrowed textures/images before or with the
    render session according to backend lifetime rules.

## Phase 7: Map State

- [ ] Create `RuntimeHandle` with cache path `:memory:`.
  - Grounding: `map-example.md#map-and-runtime`, `map-example.md#map-state`.

- [ ] Create `MapHandle` with the current viewport and continuous mode.
  - Grounding: `map-example.md#map-and-runtime`.

- [ ] Load the required style URL during map initialization.
  - Grounding: `map-example.md#style`.
  - URL: `https://tiles.openfreemap.org/styles/bright`.

- [ ] Apply the required initial camera with an immediate jump.
  - Grounding: `map-example.md#initial-camera`.
  - Center: latitude `37.7749`, longitude `-122.4194`.
  - Zoom `13.0`, bearing `12.0`, pitch `30.0`.

- [ ] Attach the initial render target after style and camera setup.
  - Grounding: `map-example.md#startup`, `map-example.md#map-state`.

- [ ] Implement runtime event draining in `MapState`.
  - Grounding: `map-example.md#map-state`.
  - Set render pending for `map_render_update_available` targeting this map.
  - Set render pending for `map_render_frame_finished` targeting this map when
    `needs_repaint` is true.

- [ ] Implement `MapState.Resize`.
  - Grounding: `map-example.md#resize`, `map-example.md#map-state`.
  - Reattach when the active render target needs reattach on resize.
  - Otherwise resize graphics and render-session resources in place.

- [ ] Implement deterministic `MapState` shutdown.
  - Grounding: `map-example.md#shutdown`.
  - Close render target, then map, then runtime.

## Phase 8: Input

- [ ] Print the required control help once at startup.
  - Grounding: `map-example.md#control-scheme`.

- [ ] Implement left-drag pan.
  - Grounding: `map-example.md#behavioral-constants`.
  - Call `move_by` with pointer delta in logical coordinates.

- [ ] Implement right-drag and Ctrl-left-drag rotate/pitch.
  - Grounding: `map-example.md#behavioral-constants`.
  - Bearing changes by `0.5 * deltaX` degrees.
  - Pitch changes by `0.5 * deltaY` degrees.

- [ ] Cancel in-flight camera transitions when a drag starts.
  - Grounding: `map-example.md#input`.

- [ ] Implement scroll zoom at cursor.
  - Grounding: `map-example.md#behavioral-constants`.
  - Use `scale_by(2^(delta * 0.25), anchor)`.

- [ ] Implement arrow-key and WASD pan.
  - Grounding: `map-example.md#behavioral-constants`.
  - Pan `120` logical units per key press.

- [ ] Implement `+` and `-` center zoom.
  - Grounding: `map-example.md#behavioral-constants`.
  - Use factors `1.25` and `1 / 1.25`.

- [ ] Implement `Q` and `E` bearing controls.
  - Grounding: `map-example.md#behavioral-constants`.
  - Rotate by `10` degrees with keyboard animation.

- [ ] Implement `]` and `[` pitch controls.
  - Grounding: `map-example.md#behavioral-constants`.
  - Adjust by `5` degrees and clamp to `[0, 60]`.

- [ ] Implement `0` pitch/bearing reset.
  - Grounding: `map-example.md#behavioral-constants`.
  - Animate pitch and bearing to `0`.

- [ ] Mark render pending after every input-driven camera change.
  - Grounding: `map-example.md#frame-loop`, `map-example.md#input`.

## Phase 9: Frame Loop and Shutdown

- [ ] Implement the main GLFW pump loop in `Shell`.
  - Grounding: `map-example.md#frame-loop`.
  - Continue until window close is requested.

- [ ] Poll window and input events every loop iteration.
  - Grounding: `map-example.md#pump-every-iteration`.

- [ ] Run one runtime pump per loop iteration.
  - Grounding: `map-example.md#frame-loop`.
  - Call the C# runtime handle's `RunOnce` equivalent.

- [ ] Drain runtime events every loop iteration.
  - Grounding: `map-example.md#frame-loop`.

- [ ] Run graphics-context frame maintenance every loop iteration.
  - Grounding: `map-example.md#pump-every-iteration`.
  - This is where swapchain or surface upkeep belongs.

- [ ] Call render update only while render is pending.
  - Grounding: `map-example.md#render-render_pending`.

- [ ] Handle invalid-state render-update failures by leaving render pending.
  - Grounding: `map-example.md#frame-loop`.

- [ ] Clear render pending after successful render update.
  - Grounding: `map-example.md#frame-loop`.

- [ ] Draw through the compositor after successful texture-mode render updates.
  - Grounding: `map-example.md#render-render_pending`,
    `map-example.md#render-target-modes`.

- [ ] Present or finish backend frame work after rendering.
  - Grounding: `map-example.md#frame-loop`.

- [ ] Idle briefly when a loop iteration makes no progress.
  - Grounding: `map-example.md#frame-loop`.
  - Use `glfwWaitEventsTimeout` or an equivalent short wait.

- [ ] Implement graceful process exit on window close.
  - Grounding: `map-example.md#what-every-example-provides`,
    `map-example.md#shutdown`.

- [ ] Release resources in required shutdown order.
  - Grounding: `map-example.md#shutdown`.
  - Finish/wait GPU work.
  - Close render target.
  - Close map.
  - Close runtime.
  - Close graphics context and window.

## Phase 10: Verification

- [x] Verify repo-root .NET tool restore succeeds.
  - Grounding: CSharpier is now a project-wide formatter tool.
  - Command: `dotnet tool restore --tool-manifest dotnet-tools.json`.

- [x] Verify binding-local .NET generator tool restore succeeds.
  - Grounding: ClangSharp remains scoped to `bindings/dotnet`.
  - Command:
    `dotnet tool restore --tool-manifest bindings/dotnet/dotnet-tools.json`.

- [x] Verify `dotnet restore Maplibre.Native.slnx` from the repository root.
  - Grounding: root solution and central package versions are the normal .NET
    entrypoint.

- [x] Verify `dotnet build Maplibre.Native.slnx -p:Platform="Any CPU"`.
  - Grounding: existing C# mise tasks force Any CPU because Visual Studio may
    export `Platform=x64` on Windows.

- [ ] Verify `mise run //bindings/dotnet:test`.
  - Grounding: binding tests should keep passing after solution and package
    configuration changes.

- [ ] Verify `mise run //examples/dotnet-map:build`.
  - Grounding: example build must integrate with native library setup.

- [ ] Smoke-test `owned-texture` on the host backend.
  - Grounding: `map-example.md#owned-texture`.
  - Command: `mise run //examples/dotnet-map:run:owned-texture`.

- [ ] Smoke-test `borrowed-texture` on the host backend.
  - Grounding: `map-example.md#borrowed-texture`.
  - Command: `mise run //examples/dotnet-map:run:borrowed-texture`.

- [ ] Smoke-test `native-surface` on the host backend.
  - Grounding: `map-example.md#native-surface`.
  - Command: `mise run //examples/dotnet-map:run:native-surface`.

- [ ] Smoke-test at least one non-default backend variant where available.
  - Grounding: `map-example.md#graphics-api`.
  - macOS example:
    `mise -E macos-arm64-vulkan run //examples/dotnet-map:run:owned-texture`.
  - Linux example: run the alternate Vulkan/OpenGL variant configured for the
    host.

- [ ] Run targeted formatting on touched files.
  - Grounding: repository formatting is orchestrated by `hk` and `dprint`.
  - Command: `hk fix [FILES...]`.

- [ ] Run the full repository formatter/linter before final handoff.
  - Grounding: `mise run fix` is the repository-wide cleanup entrypoint.
  - Command: `mise run fix`.
