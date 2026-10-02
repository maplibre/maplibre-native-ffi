# Local Linux validation

Validation runs in the ARM64 `binding-codegen-linux` Docker container on Colima,
using `ghcr.io/maplibre/maplibre-native-ffi/devcontainer:main`. The source
snapshot is `/workspaces/binding-codegen`; `/source` mounts the host checkout
read-only. `EGL_PLATFORM=surfaceless` and `LIBGL_ALWAYS_SOFTWARE=true` select
software rendering. Kotlin JVM runs use `LC_ALL=C.UTF-8` for the Unicode
local-file URL test.

The snapshot includes initialized native dependencies and a container-local Git
repository. Automatic dependency preparation is disabled for the copied native
submodules and Rust certificate-verifier sources. Other dependencies use the
normal mise providers. Mise was upgraded from 2026.7.14 to 2026.9.15.

## Final results

Both native backends, all eight binding suites, and all CI-selected examples
passed on Linux GNU ARM64. Linux x64 and musl runtime coverage remains in CI.
Kotlin/Native Linux tests use the x64 runner; ARM64 local coverage uses the JVM.

| Suite        | EGL result                                        | Vulkan result                                     |
| ------------ | ------------------------------------------------- | ------------------------------------------------- |
| Native C API | 3/3 CTest executables passed                      | 3/3 CTest executables passed                      |
| Rust         | 164 tests passed; lint and API checks passed      | 164 tests passed; lint and API checks passed      |
| Zig          | 162 passed; six backend-specific skips            | 162 passed; six backend-specific skips            |
| Kotlin JVM   | 141 passed; no skips                              | 141 passed; no skips                              |
| Swift        | 106 passed                                        | 106 passed                                        |
| .NET         | 180 passed; two Metal-specific skips              | 180 passed; two Metal-specific skips              |
| Go           | Tests and vet passed                              | Tests and vet passed                              |
| Python       | 164 passed; 30 backend or platform-specific skips | 164 passed; 30 backend or platform-specific skips |
| Dart         | 80 passed; no skips                               | 80 passed; no skips                               |

The Rust example checks, Zig map build, Zig readback run, Compose map build,
LWJGL map build, and C map build passed for both backends. The Go map check
passed for EGL, where CI selects it.

The final Rust runs include callback-retirement fixes and their regressions. The
complete Rust core source directory, umbrella runtime, and Python generated
operation table matched the host files after regeneration. Rendering fixtures
now execute the generic Swift, .NET, Dart, and Kotlin workflows on both
backends. The .NET WGL fixture compiled locally; its Windows runtime validation
belongs to CI.

The Python Vulkan abandon test uses the caller graphics driver to guarantee that
graphics work has finished before abandonment. The core-worker API can return
`Busy` until its driver call finishes. The Go log-registration lifetime test
counts global registration roots because releases from earlier runtime tests can
change the process-wide callback count. Both corrected suites passed.

Runtime command captures now retire before ordered barriers complete. A native
regression queues a barrier from a command and checks capture retirement inside
the barrier callback. The old ordering failed deterministically with exit 134;
the corrected Linux Vulkan native suites passed all three CTest executables. The
geometry and MVT source lifetime tests also assert release after one barrier.

## Commands and logs

Native suites use `mise run --no-deps test <preset>`. Binding suites use
`mise run //bindings/<language>:test <preset>`, with `--no-deps` when dependency
preparation has already completed. Example commands come from
`ci.workflow.suite_commands()` and `ci/workflow.toml`.

Container logs preserve the individual commands and results:

- `/tmp/linux-egl-native.log`: native EGL.
- `/tmp/linux-vulkan-native-resume.log`: native Vulkan; compilation resumed with
  four jobs after an intentional interruption to reduce host contention.
- `/tmp/linux-egl-rust-retirement.log`: final Rust EGL source and regressions.
- `/tmp/linux-{egl,vulkan}-swift-final-helper.log`: final shared graphics
  helper.
- `/tmp/linux-{egl,vulkan}-dotnet-wgl-build.log`: final .NET fixture compilation
  and suites.
- `/tmp/linux-{egl,vulkan}-dart-rendering.log`: final Dart rendering suites.
- `/tmp/linux-{egl,vulkan}-kotlin-rendering-utf8.log`: final Kotlin JVM
  rendering suites.
- `/tmp/linux-{egl,vulkan}-go-logroots.log`: corrected Go log-registration test.
- `/tmp/linux-vulkan-python-abandon.log`: corrected Python Vulkan abandon test.
- `/tmp/linux-vulkan-dotnet-no-generic-skips.log`: generic .NET rendering guards
  removed.
- `/tmp/linux-barrier-mutant-test.log`: deterministic failure with the old
  retirement order.
- `/tmp/linux-barrier-source-tests.log`: native suites with the corrected
  retirement order.
- `/tmp/linux-suites/results.tsv`: the complete sequential CI-command ledger.
- `/tmp/linux-suites/linux-gnu-arm64-{egl,vulkan}-*.log`: per-command output.

Host copies of the final logs are in `/tmp/binding-codegen-linux-validation/`.
The sequential ledger retains the earlier Python failure; the corrected Python
log records its successful rerun.

The pinned Swift 6.3.1 ARM64 toolchain and libxml2 dependency both installed and
passed the full Swift mise task. The Swift binding and example tool declarations
now enable Linux ARM64, and the generated devcontainer tool list matches them.
