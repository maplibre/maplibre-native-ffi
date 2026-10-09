# Local platform validation

PR #760 includes the native executor and generated bindings against `main`. The
initial push did not have complete local mobile coverage. Follow-up validation
includes the macOS and mobile checks below, all four pinned OpenHarmony cross
builds, and both Linux ARM64 backends in Colima. Windows remains a remote CI
gate. Backend-specific exclusions select the renderer that a build contains.

## Completed runs

| Target                   | Evidence                                                                                                                                                                                    |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| macOS Metal, EGL, Vulkan | Native and all eight binding suites pass on each backend. Final Rust retirement tests pass on all three; rebuilt Python extensions pass 168, 164, and 163 tests respectively.               |
| macOS examples           | Zig examples build for all three backends; readback produces 512 by 512 images. LWJGL and Compose build for all three. Bounded Metal GUI runs demonstrate startup and rendering.            |
| iOS simulator            | Native CTest, Kotlin, Swift (113), and Zig suites pass. Kotlin passes on iOS 15.5 and 26.5. The Swift example and Flutter package build; Flutter launches on the simulator.                 |
| tvOS simulator           | Native CTest (2), Kotlin, Swift (113), and Zig suites pass.                                                                                                                                 |
| iOS and tvOS devices     | Native, Kotlin, Swift, and Zig builds pass. The iOS Swift example and unsigned Flutter package build. These are build results, without physical Apple-device execution.                     |
| Mac Catalyst             | Native CTest (2) and Swift (113) pass.                                                                                                                                                      |
| Android ARM64 EGL        | Pixel 8 passes native (171), minified Kotlin (127), Rust, Go, Zig, and Python (163) suites. Flutter launches and resolves C ABI 0.                                                          |
| Android ARM64 Vulkan     | Pixel 8 passes native (168, two EGL exclusions), minified Kotlin (127), Rust, Go, Zig, and Python (162) suites. Flutter launches and resolves C ABI 0.                                      |
| Android API 26 ARM64     | Isolated emulator passes native (171), minified Kotlin (127), final Rust (164), and Go suites.                                                                                              |
| Android cross builds     | All six native targets and Flutter packages pass. Kotlin Native, Rust, Go, and Zig builds pass for all six targets. Multi-ABI Kotlin packages and Android examples pass for both backends.  |
| Browser WebGL/WebGPU     | Final native and complete Rust suites plus strict Clippy pass on both backends. The map-close race found in CI is fixed, with a deterministic regression and 100 single-CPU Chromium runs.  |
| OpenHarmony              | All four native targets and Rust/Go cross builds pass with pinned SDK 6.1. Its x64 EGL target passes native (171), Rust (131 integration, 29 core, two sys), and Go runtime suites in QEMU. |

Python Android excludes the interpreter-shutdown subprocess case because its
embedded interpreter has no executable; host Python still runs that case.
Rendering tests run on Android. Android x64 runtime coverage remains a CI gate
because this host accelerates ARM64 emulators.

## Failures found locally

JavaCPP wraps null C function pointers in non-null Java objects. Generated
Android probes inspect the native fields, and array readers accept null data
when the element count is zero. Both repairs pass minified Android tests. The
ARM32 build also exposed a Zig message-offset narrowing error; generated readers
now check conversion to the target pointer width before slicing.

Kotlin Native inline coroutine completion can retain the submitted callback in a
temporary. Its lifetime test now submits from a normal helper and awaits the
close completion. Android's older `System.gc()` does not request collection on
every call; its lifetime tests use `Runtime.gc()` once and await cleanup. Both
suites pass, and retaining the Native callback deliberately fails its test.
Android cleanup clears drained phantom references and actions.

Browser tests exposed immediate-task scheduling and worker-finalizer progress
failures. Resource-provider tests exposed implicit-drop decision races and
cancellation-token reclamation before native quiescence. The fixes pass
regressions that fail when each repair is reverted.

OpenHarmony execution exposed an assertion that observed callback ownership
before command cleanup. Go tests now await a runtime barrier after the rejected
command. Deliberately retaining its callback still fails the test.

Flutter treated shared install libraries as generated hook outputs and removed
them after a backend switch. The hook now copies libraries into its output
directory, with a regression that removes outputs and verifies both original
installs remain intact.

## Rendering coverage audit

Some earlier passing suites omitted generic rendering tests outside their first
supported backend. The follow-up replaces those omissions with actual driver
fixtures. Each test submits MapLibre work through its public language binding;
the shared C helper creates only EGL or Vulkan driver objects.

Dart passes 80 tests on macOS Metal, EGL, and Vulkan and on both Linux backends.
Kotlin passes 141 JVM and 139 Native tests on each macOS backend, 141 JVM tests
on each Linux backend, and 127 minified Android Vulkan tests on the Pixel 8.
Swift passes 113 tests on all three macOS backends and the iOS simulator, with
106 tests on each Linux backend. Its shared helper also compiles and passes the
Mac Catalyst suite. .NET passes 180 Vulkan tests on macOS and Linux; its two
exclusions require Metal. Windows WGL fixtures await remote execution.

## Remaining gates

Full CI for `edb4a6593` passes all Android and OpenHarmony targets and Android
multi-ABI packaging. The Windows native and Rust runtime suites pass; three
unnecessary `unsafe` blocks stopped Clippy and are removed in the follow-up.
Linux x64 EGL exposed a Go test counter that included unrelated callback
registrations; counting the tested global registrations fixes the local suite.
Browser map closure exposed a redundant wake after run-loop shutdown: the worker
could already have destroyed the loop. The follow-up removes that late access
and adds a deterministic destruction regression. Restoring the extra wake makes
that regression trap on retired storage; the fixed WebGPU native suite passes.
Both complete Rust browser suites, their 29 core and two sys tests, Clippy, and
the native suites pass. The fixed artifact also passes 100 fresh Linux Chromium
runs on one CPU; the original artifact timed out under that schedule.

A Swift lifetime assertion exposed another native ordering bug. Runtime barriers
could complete before the preceding command released its captured callbacks.
Command captures now retire before the submission becomes terminal. A nested
barrier regression fails on the old code and passes with the fix; both custom
source ABI tests also require release after one barrier. Final native and Swift
suites pass on all three macOS backends. The final iOS simulator native suite
passes both CTest executables; the Pixel Vulkan native suite and all 127
minified Kotlin tests also pass.

Complete generation, 82 compiler fixtures, 28 CI planning tests, documentation
snippets, and the documentation site pass locally. The
[Linux matrix](local-linux-validation.md) records container execution separately
from remote CI. Full remote coverage remains the completion gate.

## Follow-up CI timing and driver fixes

CI for `695ebdc9b` passes every Linux and macOS target, both browsers, Windows
ARM64 Vulkan, and the Android targets other than x64 EGL. Its remaining failures
are binding-test timing or driver-fixture issues.

The Apple static-image test now orders the pending render update with a later
camera query before inspecting its frame. It records the old projection while
the style response is withheld, fulfills the response, and then checks both
observations. Publishing a projection before a frame completes deliberately
fails this test. Full macOS Native, iOS simulator, and tvOS simulator Kotlin
suites pass with the fix.

The Zig concurrent-release test accepts either the binding's closed-owner error
or the C API's retired-handle error for an overlapping query. The native handle
table retains the object during a call and checks retirement under its mutex.
The test still requires a successful query before release, successful release,
and the exact binding closed-state error after the probe joins. Full macOS and
Pixel Vulkan Zig suites pass.

WGL fixtures select dedicated contexts for core-worker rendering and shared
contexts for caller-driver rendering. The .NET fixture retains private readback
coverage; Dart also retains its caller-driver frame-view workflow. Kotlin JVM
passes the existing native loader directories to LWJGL, matching the map
example. Local compilation, analysis, and relevant suites pass; Windows runtime
validation belongs to the next CI run.
