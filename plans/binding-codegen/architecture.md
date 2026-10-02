# Binding compiler architecture

This iteration introduces a shared binding compiler and uses its value plans for
native completion captures and recursive .NET conversions. The other backends
pass through shared validation while their remaining lowering logic is migrated.
The user ranks correctness and performance first, readability second, usability
third, code size fourth, and compatibility last. Existing API shapes may change
when that improves those priorities.

## Decisions

- Public C declarations remain the source for types and exported operations.
  Header metadata supplies relationships that C erases. Metadata describes
  behavior, not language-specific source templates.
- One semantic resolver owns value shapes, presence, registration lifetimes,
  result ownership, and operation effects. Language backends emit static code
  from resolved plans.
- Direct callbacks and borrowed completion values remain the fast path. A host
  runtime that requires deferred delivery captures data before returning from
  native; capture code comes from the same value model.
- Callback roots, host scheduling, library loading, and handle close-once state
  are runtime mechanisms. Per-operation signatures and conversions are
  generated.
- Native owns call leases and retirement. Bindings preserve host memory safety
  without duplicating native synchronization.
- Cleanup of an unclaimed owned result must have a defined failure-free path. An
  optional completion observer must not be required to initiate cleanup.

## Work and evidence

The architecture reviewer owns the shared semantic model and header contracts.
The core implementor owns retirement and callback cleanup mechanisms. The
compiler implementor owns generated native captures and host delivery. The
coordinator owns integration, language backend migration, build verification,
and this decision record. Each implementation receives independent review before
it is reported complete.

The existing production integration suites remain the behavioral oracle. New
checks target semantic boundaries: rejection versus acceptance, callback
retirement during replacement, unclaimed owned results, and borrowed data
captured before return. Performance checks measure the cleanup and copy paths
that this design changes. Generated files must reproduce exactly after
formatting, and missing semantic contracts must fail before emission.
