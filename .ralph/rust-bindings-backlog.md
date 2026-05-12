# Rust bindings backlog completion

Implement each new backlog item from `bindings/rust/PLAN.md`, with review and
commit/push after each item.

## Goals

- Complete the missing API parity backlog.
- Complete the API polish backlog before review.
- Continue after parity work until every API polish checklist item is complete;
  ordering is flexible, but stopping after parity is not complete.
- For each item: implement via worker, run parallel fresh-context reviewers,
  apply sensible fixes, mark item complete in PLAN.md, commit, and push.

## Checklist

- [x] Commit and push current backlog/tooling changes (`6efbc1a`).
- [x] Runtime options and explicit runtime creation.
- [x] Ambient cache operations.
- [x] Process-global logging callbacks, clearing, severity values, and async
      severity mask.
- [ ] Style source removal and source existence checks.
- [ ] Style source type, source info, attribution, and copied source output
      types.
- [ ] Style image add/remove/query APIs and image metadata/value types.
- [ ] Image source APIs for URL, coordinates, and image updates.
- [ ] Remaining layer/source helpers exposed by Java FFM over the C style API.
- [ ] RenderSession feature state set/get/remove.
- [ ] FeatureStateSelector and selector materialization.
- [ ] Rendered query geometry and rendered/source query option types.
- [ ] Copied queried feature and feature-extension result types.
- [ ] Rendered feature, source feature, and feature extension query methods.
- [ ] Revisit every public destructive or one-shot operation; prefer consuming
      close/complete and preserve retry semantics explicitly.
- [ ] Replace frame-derived bare NativePointer returns with lifetime-bearing
      FrameNativePointer<'frame> or equivalent.
- [ ] Split large Rust modules, aligned with Java FFM package structure where
      appropriate.

## Verification

- Runtime options and explicit runtime creation:
  `mise run //bindings/rust:test`.
- Runtime options reviewer fixes: `cargo fmt --all --check`,
  `cargo test -p maplibre-native runtime_options`, and
  `cargo test -p maplibre-native runtime_create_with_explicit_options_uses_real_c_abi`.
- Ambient cache operations: `cargo fmt --all --check`,
  `cargo test -p maplibre-native ambient_cache_operation_raw_values_match_c_abi`,
  `cargo test -p maplibre-native runtime_ambient_cache_operations_use_real_c_abi`,
  `cargo test -p maplibre-native --test public_api public_handles_create_pump_drain_and_close`,
  and `mise run //bindings/rust:test`.
- Initial push: `git push origin rust-bindings-plan`.
- Ambient cache reviewer fixes: `cargo fmt --all --check`,
  `cargo test -p maplibre-native ambient_cache_operation_raw_values_match_c_abi`,
  and
  `cargo test -p maplibre-native runtime_ambient_cache_operations_use_real_c_abi`.
- Process-global logging APIs: `cargo fmt --all`,
  `cargo test -p maplibre-native logging`, and `mise run //bindings/rust:test`.
- Process-global logging reviewer fixes: `cargo fmt --all --check` and
  `cargo test -p maplibre-native logging`.

## Notes

- Use `mise run //bindings/rust:test` and targeted checks after each
  implementation.
- Run parallel reviewers after each item before fix/commit.
