# Completion boundaries

There are no known architectural blockers to generating the current C API. All
eight bindings account for all 264 public declarations through generated
operations or verified support relationships. The complete-coverage check
rejects an unsupported declaration or an unresolved contract.

The original gaps in retained callbacks, provider decisions, attachment
adoption, scoped GPU access, recursive values, and owner retirement now have
shared plans and generated adapters. Their implementations and behavior tests
replace the corresponding handwritten mechanical wrappers.

## Active validation

[Completion](completion.md) records the current test results and final gates.
Local binding suites, native teardown regressions, complete generation,
deterministic regeneration, repository hygiene, and documentation builds pass.
The [local matrix](local-validation.md) records completed platform suites and
remaining build gates. Swift generated code compiles without warnings. The
implementation is published in draft PR #760. Full platform CI exposed
portability and lifecycle defects. Their fixes pass local regressions and the
expanded platform matrix; the next full CI run remains the final gate. These
tasks require no further product or API decisions from the user.

## Runtime limits

A new C ownership protocol still needs a shared compiler rule and runtime
support before generation can succeed. The compiler rejects unknown shapes; it
cannot infer ownership or callback quiescence from pointer types alone.

Rust and Swift callers use weak captures when a retained callback refers to its
owner. Kotlin, .NET, Dart, and Python integrate callback roots with their
garbage collectors. Language runtime mechanisms remain handwritten.

Native disposal admission and scheduling are allocation-free. Teardown work on
other threads may allocate, so persistent allocation failure can prevent
eventual cleanup. Local runs validate macOS, mobile, Linux, and browser targets
as recorded in the matrix. Windows execution and complete remote coverage remain
CI gates.
