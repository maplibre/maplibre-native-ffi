# C# binding review record

This file records requirement-focused review evidence for the non-packaging C#
binding implementation.

## Initial review

Parallel reviewer run `5bcdefce` inspected `bindings/dotnet/SPEC.md`,
`bindings/dotnet/**`, Java parity, and binding conventions.

Findings fixed afterward:

- `RuntimeEvent` exposed a raw native source pointer instead of typed sources.
- Custom geometry callback state could free its `GCHandle` while callbacks were
  active.
- Runtime event payload copying lacked size checks for known payload structs.
- Wrong-thread propagation lacked native-backed C# coverage.
- `ResourceRequestHandle` pass-through/one-shot lifecycle coverage was thin.
- Binding CI did not include C# format verification.

## Follow-up review

Reviewer run `eb4d5de2` rechecked the fixes and reported **PASS — no remaining
blockers found**.

Confirmed by review:

- `RuntimeEvent` exposes typed `RuntimeSource` / `MapSource`.
- Runtime map registry resolves map event sources without exposing raw ABI
  pointers.
- Custom geometry callback `GCHandle` is retained until active callbacks exit.
- Runtime payload readers check payload size before typed reads.
- Wrong-thread propagation has test coverage.
- Resource request pass-through lifecycle has test coverage.
- `mise run //bindings/dotnet:ci` runs format plus native-backed tests.
- Packaging/publication and GUI map work remain out of scope.

## Final auditor follow-up

The independent goal auditor rejected completion because leak/failure reporting
for thread-affine handles was missing. The follow-up implementation added:

- `Internal/Handle/NativeLeakReporter.cs` for best-effort dispose failure and
  finalizer leak reports.
- `NativeHandleState` finalizer leak reporting and `TryClose()` failure
  reporting.
- `NativeHandleStateTests` coverage for dispose failure reports and finalizer
  leak-reporting hooks.

Validation after the follow-up fix: `mise run //bindings/dotnet:ci` passed with
format plus 65 tests.
