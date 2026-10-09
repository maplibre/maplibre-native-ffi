# Complete binding generation

The active target is a complete migration of all eight bindings in PR #760
against `main`. Ordinary implementation work remains active until the production
generators account for every C export through an emitted public operation or a
verified support relationship.

## Ownership of work

| Area                                                         | Owner                | Completion evidence                                               |
| ------------------------------------------------------------ | -------------------- | ----------------------------------------------------------------- |
| Shared protocols, header contracts, native ownership, Kotlin | Astra protocol agent | Typed transitions, adversarial lifetime tests, native suite       |
| .NET, Swift, Dart                                            | Astra managed agent  | Generated values and operations, public binding suites            |
| Rust and Zig                                                 | Astra native agent   | Generated values and operations, public binding suites            |
| Python, Go, generation gate, integration                     | Root agent           | Generated Python API, deterministic output, complete coverage, CI |

Each backend consumes the shared semantic model. Handwritten code implements
language runtime mechanisms such as callback roots and handle state. Adding a C
operation that uses an existing protocol requires only its header contract and
regeneration.

## Validation gates

1. Resolve value shapes and ownership transitions without language-specific
   reinterpretation of C metadata.
2. Replace handwritten mechanical methods and converters with emitted code.
3. Exercise native workflows and binding-owned lifetime failures through public
   APIs, including abandoned results and partial construction failures.
4. Require deterministic regeneration and complete operation coverage.
5. Run each binding suite, documentation checks, and full platform CI; fix
   failures before declaring completion.

The draft PR targets `main` and includes the executor work from PR #631.
Follow-up commits belong to this draft while implementation and validation
continue. Remaining-work notes record tasks, not reasons to stop execution.

## Implementation state

All eight bindings account for all 264 public C declarations through generated
operations or verified runtime support relationships. The shared model resolves
value shapes, fixed-width scalar carriers, execution categories, callback
lifetimes, ownership transfers, and scoped views. Equivalent fixed-width C
typedefs produce identical source across Clang host platforms. Event batch
storage fields are internal to decoding; callers receive copied event messages.

Generated code owns mechanical operations, values, conversion, and callback
descriptors. Handwritten code owns language runtime mechanisms. Kotlin's former
camera, geometry, query, style, map, runtime, resource, offline, and render
value layers have been removed. Its owners retain lifecycle state and parent
references.

Kotlin, .NET, and Dart registrations are held by their owners and resolved from
native tokens weakly. This permits collection when a callback captures its
owner. Native release retires registration state after quiescence. Rust and
Swift callers use weak captures to avoid their languages' reference-counting
cycles. Python garbage collection visits owner-held roots, and user future
callbacks run on host workers with native reentry admission checks.

Resource request completion reserves state without holding a mutex across a
native call. Rejected responses permit retry. Accepted responses retain the
caller's request owner until explicit close or finalization. Inline completion
claims the provider decision. Borrowed GPU scopes retain session resources
through callbacks and delay retirement until active scopes end.

The convention documents have been replaced by a binding generation guide that
links to the executable schema and semantic model. The complete-coverage check
is part of the repository generation gate.

## Validation state

The latest completed host runs include 182 .NET tests, 80 Dart tests, 113 Swift
tests, 168 Python tests, the Go suite with race and strict C-pointer checks, and
164 Zig tests. Python and Zig exclude tests for other render backends in the
Metal preset. Rust passes 133 integration, 29 core, and two sys tests. Kotlin
passes 141 JVM and 139 Native tests. The native retirement race and destructive
abandonment reentry are fixed, with direct regression coverage.

Both Zig examples compile for Metal, OpenGL, and Vulkan. The Metal readback
writes a nonuniform 512 by 512 image, and all three interactive target modes
complete bounded smoke runs without reported errors. The LWJGL example builds
and completes bounded Metal runs in all three target modes. The Compose example
builds and completes a bounded Metal run. Both Android examples package all
three ABIs. Bounded GUI runs establish startup and rendering evidence, not
graceful shutdown.

Complete generation and deterministic regeneration pass across all eight
bindings. All 297 wholly generated binding files have generated Git attributes;
handwritten runtime files remain visible in reviews. Repository hygiene and the
documentation build pass. Swift output is split by C header and owner.

[Local platform validation](local-validation.md) records macOS, Apple mobile,
Android, browser, and OpenHarmony execution and build results. Swift generated
code compiles without warnings. Draft PR #760 publishes the implementation. CI
exposed portability and lifecycle defects whose fixes now pass local
regressions, including deliberate mutations of the original faulty behavior.
Full remote platform validation remains active and requires no further user
input.
