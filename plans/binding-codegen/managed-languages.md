# Managed binding generation

The managed bindings can share one semantic model and one rule set per language.
The C declarations define layout and callable signatures. Header metadata must
also define result types, ownership, optional values, and input buffer meaning.
A completion pointer alone erases these properties.

## Current structure

The inventory below counts source files under each binding's source directories
at `5f6976bdf`. Kotlin includes its source-set tests. Generated declarations are
identified by a generated directory or `.g.` filename. Files that call raw C
functions include runtime infrastructure, so their line count is an upper bound
on removable handwritten code.

| Binding | Source lines | Generated declaration lines | Handwritten files calling C |
| ------- | -----------: | --------------------------: | --------------------------: |
| Kotlin  |       99,140 |                      58,679 |                      22,906 |
| .NET    |       15,943 |                       3,684 |                      10,144 |
| Python  |       15,605 |                           0 |                       8,068 |
| Dart    |       21,925 |                       8,677 |                      10,001 |

.NET has a useful separation already. `NativeCompletion` owns callback roots,
acceptance failure, diagnostic copies, result conversion, and asynchronous
continuations. `NativeHandleState` owns wrapper lifetime. Generated partial
classes can replace individual methods in the existing public classes while
sharing those helpers. The first implementation uses this seam.

Python repeats the operation in a PyO3 method, a Python wrapper, and a stub. Its
completion runtime is reusable. Generating both the PyO3 method and public
Python method preserves wheel packaging and the existing callback/GIL boundary.
A direct Python FFI would remove the Rust wrapper but would also change native
library packaging and callback execution; that is a separate design decision.

Dart has a generated ABI module and a handwritten public runtime module.
`NativeCallable.listener` requires copying callback data before posting it to an
isolate. The native callback adapter therefore remains part of the runtime.
Generated result decoders can share its completion queue, cancellation, and
handle state.

Kotlin needs one common API emitter and three platform lowering strategies: JVM
memory segments, Android JavaCPP/JNI, and Kotlin/Native interop. Their operation
semantics can share one model, while their allocation and callback mechanisms
remain platform-specific. The existing common expect/platform actual classes can
receive generated methods during migration. JVM `NativeAccess` currently repeats
the public method forwarding and can eventually disappear when generated actual
methods call the ABI directly.

## Rules that the model must express

Execution categories select an operation skeleton: command completion, ordered
query, immediate operation, published snapshot, event drain, or driver call.
These categories alone cannot select the memory conversion. The model also
needs:

- Completion result type and cardinality: one value, an optional value, or a
  list.
- Result ownership: callback-borrowed, owned handle, or value.
- Input and output buffer meaning: UTF-8 text, opaque bytes, or an element list.
- Optional-value representation: null pointer, empty view, mask bit, or count
  zero.
- Handle construction and release relationships, including detached handles.
- Struct defaults and fields controlled by a presence mask.
- Callback retention and release contracts.

For example, layer entries contain required `id` and `type` strings plus
optional `source_id` and `source_layer` strings. All four use the same C
buffer-view type. Clang recovers their layout, but metadata must distinguish
their meanings. The generated decoder copies them during the completion
callback.

Owned-handle completions require a different cancellation rule from copied
values. Abandoning an accepted create before wrapping or releasing its result
leaks a native handle. The existing .NET create method takes no cancellation
token; generated handle creation must preserve that property.

## Validation and migration

The first production slice replaces existing .NET methods for a command, scalar
query, copied layer list, and detached immediate query. It must compile and pass
the existing public integration tests against the current native library.
Subsequent slices cover record conversion, snapshots, and handle construction
and release. Generated files are checked in, and regeneration must leave no
diff.

The full migration requires generated records and input encoders before deleting
all per-operation conversion helpers. Backend descriptors and retained host
callbacks require explicit metadata and lifecycle tests. Unsupported shapes must
carry a diagnostic and declaration location in coverage; silently keeping an
unclassified manual method would restore the maintenance problem.

## Implemented production generation

The implementation now replaces existing public methods rather than adding a
parallel API:

| Binding | Generated operations | Integration                                                                                  |
| ------- | -------------------: | -------------------------------------------------------------------------------------------- |
| .NET    |                   66 | Partial `MapHandle` and `MapProjectionHandle` classes, copied value records, and input enums |
| Dart    |                   44 | Private generated mixins inherited by the existing public handle classes                     |
| Kotlin  |                   43 | A generated common base class with JVM, Android, and Kotlin/Native actual implementations    |

.NET generates primitive and buffer commands, scalar and copied-buffer queries,
plain record inputs and results, copied record arrays, detached projection
creation and close, and immediate projection conversions. Its generated value
records use the namespace of their declaring header. For example, `LatLng` and
`ScreenPoint` now belong to `Map`; `CanonicalTileId` belongs to `Style`.
`MapDebugOption` is generated from the C enum. Generated method names derive
from C names, and callers were updated with the production migration.

Kotlin generated methods call each platform's ABI directly. The JVM migration
also removes 43 forwarding methods from `NativeAccess`; its generic memory
converters remain available to both generated and handwritten code. The public
handle classes contain one platform-specific handle accessor that their
generated base class uses. Android's JavaCPP configuration explicitly parses
`binding.h` so that its parser consumes the annotation macro before parsing
function parameters. Its conditional-preprocessor mapping explicitly disables
the annotation branch for JavaCPP; this parser matches the full conditional
expression rather than evaluating C preprocessor expressions.

Dart uses the existing native callback adapter for flat values and copied buffer
views. Its generated result decoders run on the receiving isolate after that
adapter has copied native callback storage. Generated optional string queries
interpret an empty view as null, matching header metadata. Kotlin follows the
same rule; tests that previously expected an empty string now expect null.

The generated methods preserve the handwritten completion and handle runtimes.
.NET's detached projection creation returns a noncancellable task, so an owned
native result always reaches a wrapper. Primitive and byte/string inputs are
allocated through submission and then released; declarations that request owner,
completion, or process lifetimes are rejected by these emitters.

## Remaining generation boundaries

The coverage manifest records each unsupported declaration and its source
location. The current boundaries are implementation gaps with explicit safety
requirements:

- Tagged unions need active-variant selection; reading every overlapping field
  is unsafe. All emitters reject untagged union conversion.
- Masked and nested records need a generated representation and converters.
  Camera updates and snapshots contain these records. A generated representation
  can expose the C field mask and values directly, but the existing optional
  DTOs and their remaining consumers must migrate together.
- Borrowed pointer-bearing records need recursive copy rules. .NET implements
  the layer-entry record because its four text fields carry metadata. Dart
  additionally needs a generated native callback copier before arbitrary nested
  records can cross an isolate boundary; retaining their original pointers would
  read callback storage after release.
- Callbacks and owner-linked handles need generated registration and teardown
  rules. Resource providers, custom-source callbacks, render sessions, and
  acquired frames have different ownership relationships. Current emitters keep
  these implementations handwritten and reject them in coverage.
- Kotlin and Dart still need generated open-value enum and plain-record
  representations before their emitters can cover the remaining .NET domain.
- Process-global helpers, event batches, render-driver calls, and runtime
  lifecycle methods need their generated owners. The parser accounts for these
  declarations even when a language emitter has no corresponding owner yet.

These boundaries do not indicate that a language cannot support generation. They
prevent an incomplete converter from being reported as generated coverage. The
existing handwritten implementations continue to serve uncovered APIs.

## Validation evidence

The production .NET migration passed all 208 host tests against the existing
Metal library. Its raw ClangSharp declarations were regenerated after the final
header annotation formatting.

The Dart migration passed all 85 tests, and `dart analyze` reported no issues.
Ffigen regeneration also succeeded.

The Kotlin migration passed 187 JVM tests and 211 macOS ARM64 tests with no
skips. Jextract regeneration succeeded, followed by another passing JVM run.
Android Kotlin and generated Java main sources and Android device test sources
compiled successfully. These checks disabled native library and JNI producer
tasks and reused the available prebuilt build root; they do not establish an
Android native build or device-test result. No Android device tests ran.

The three migrations removed approximately 1,335 handwritten .NET source lines,
719 Dart source lines, and 1,961 Kotlin source lines, measured as net deletions
in nongenerated production source files. This includes the eliminated JVM
forwarding methods and excludes generated output and test migrations.

## Emitter safety review

Mutation tests cover reserved and colliding identifiers, optional buffer arrays,
nullable inputs, binary absence representations, counted outputs, and commands
on owners without a receipt runtime. Unsupported contracts produce coverage
diagnostics before emitting a method. New .NET records receive their generated
namespace imports automatically, and raw C fields that are C# keywords use
escaped accessors.

The emitters reject union conversions, field-presence metadata without a
converter, and consumption contracts without ownership transfer. Future APIs
that use these shapes require an emitter/runtime implementation before their
handwritten methods can be removed; schema acceptance alone does not mark them
as generated.
