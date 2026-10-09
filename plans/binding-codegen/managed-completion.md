# Managed compiler migration

.NET, Swift, and Kotlin generate all 264 public operations from the shared typed
model. Dart generates 262 operations and uses two verified native adapter
response setters. The superseded handwritten API layers, value converters, and
callback descriptors are removed. The complete generator check reports zero
unsupported operations across all eight languages.

## Generated ownership and callbacks

Recursive value conversion covers records, nullable buffers and arrays, presence
groups, tagged unions, defaults, and nested collections. Pointer presence
preserves absent versus present-empty buffers. Event copies use the declared
byte stride and validate message arena bounds. Private runtime adapter values
stay outside public enum inventories. Swift emits 50 source files grouped by C
header and owner, with callback thunks beside their declarations.

Factories and attachments adopt raw owners transactionally. Failed wrapper
construction retires the raw owner; accepted asynchronous attachment failure
preserves the session for explicit cleanup. Borrowed copies reserve their owner
through conversion. Scoped GPU views hold a native token through host access;
sibling frame finalization defers resource retirement until that scope ends.

Callback admission follows the shared reentry protocol. Scoped responses remain
restricted to their callback lifetime and invoking thread. Rejected resource
completion preserves retry, accepted completion retains the request owner, and
explicit inline close claims the provider decision. Weak registration tokens
permit collection of tracing-GC owners captured by their callbacks. Native
quiescence releases callback storage. Swift callers use weak captures when an
owning callback refers to its owner to avoid an ARC cycle.

Dart uses generated native ports and VM-owned queue storage. Its collection
regression covers callbacks that capture their runtime through wake and provider
registrations; restoring a strong receive-port handler makes the regression
fail. Kotlin and Go callback epilogues retain provisional request wrappers
through decision finalization.

## Browser and request retirement fixes

Emscripten async tasks report an already-due deadline. Reading the clock again
could otherwise place them after the run loop's readiness cutoff indefinitely.
The wake calculation checks overdue deadlines before subtraction to avoid signed
duration overflow. A native regression advances the clock between task checks
and verifies that a callback can enqueue another immediately due task. Reverting
only the deadline fix makes that regression fail.

Rust deferred finalization uses the existing worker queue on Emscripten. Native
worker callbacks cannot depend on returning to the JavaScript event loop. The
regression blocks the original callback thread while waiting for finalization;
it fails with the previous timer dispatch.

Implicit request Drop relinquishes its shared reference. The shared state
finalizes the provider decision before releasing an accepted owner. Its
destructor defers the native release itself when inside a callback. The public
provider regression waits for queued finalizer work before returning
PassThrough; restoring the old wrapper's explicit close makes the test fail. A
separate core regression verifies that final native release occurs off the
callback stack.

Rust and Python cancellation token reclamation waits for native retirement on
the finalizer worker. PassThrough returns ownership before native retirement,
and cancellation can outlive self-release. A regression invokes a captured weak
token while retirement is blocked; removing the retirement wait makes it fail.
`DecisionPlan.wait_retired` is an explicit verified relationship. The compiler
rejects a different receiver, extra parameters, a consuming operation, and
callback reentry into that wait.

The already-constant TLS initializer requires a static-scoped Clippy allowance
on Android and OpenHarmony with Rust 1.95. Minimal reproductions fail without
the allowance, and actual core all-target Clippy checks pass with it.

## Validation

All 82 generator fixtures pass. Complete generation followed by the
deterministic check passes for all eight languages. .NET DocFX, Swift DocC, and
Dart dartdoc references build with zero warnings or errors. Swift compiles
without generated source warnings, and the Dart analyzer reports no issues.

| Scope                                         | Result                                                                        |
| --------------------------------------------- | ----------------------------------------------------------------------------- |
| Swift macOS Metal, EGL, Vulkan                | 113 tests pass per backend                                                    |
| .NET macOS Metal                              | 182 tests pass                                                                |
| .NET macOS EGL                                | 180 pass; two Metal-only exclusions                                           |
| .NET macOS Vulkan                             | 177 pass; five backend-specific exclusions                                    |
| Dart macOS Metal                              | 79 tests pass                                                                 |
| Dart macOS EGL and Vulkan                     | 76 pass per backend; three Metal-only exclusions                              |
| Kotlin JVM and macOS ARM64                    | 141 and 139 tests pass                                                        |
| Swift iOS simulator, tvOS simulator, Catalyst | 113 tests pass per target                                                     |
| Swift iOS device binding and example          | Builds pass                                                                   |
| Swift tvOS device binding                     | Build passes                                                                  |
| Dart iOS device and simulator                 | Packages build; simulator displays the packaged C ABI result                  |
| Dart Android ARM64 EGL and Vulkan             | Release packages build; both display the packaged C ABI result on a Pixel 8   |
| Dart Android ARM32 and x64 EGL and Vulkan     | Release packages build                                                        |
| Browser WebGL and WebGPU                      | Complete Rust suites, core/sys tests, all-target Clippy, and both CTests pass |

The .NET Linux musl ARM64 package smoke also passes remotely. A fresh NuGet
package directory prevents an overwritten fixed-version local package from
reusing an older cached assembly.

All package checks in this scope pass. See
[local validation](local-validation.md) for the complete platform matrix and
outstanding work owned by the other agents.
