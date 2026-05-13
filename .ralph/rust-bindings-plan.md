# Rust binding refactor cleanup plan

Work from `bindings/rust/PLAN.md`. The branch/PR scope is Milestones 1 through
9: rename the shared crate, move reusable ABI adaptation into
`maplibre-native-core`, and keep `maplibre-native` focused on public Rust
policy. Python, Node.js, and JNI skeleton crates are future work.

## Completed scope

- Established `maplibre-native-core` and removed the old support crate boundary.
- Moved shared copied values, enum mappings, JSON, geometry, GeoJSON,
  descriptors, result readers, events, logging records, resource primitives, and
  native handle state into core.
- Kept Rust handle ownership, thread affinity, callback ergonomics, and
  map/source lookup policy in `maplibre-native`.
- Added core tests for shared adaptation and Rust tests for public API/policy.

## Remaining cleanup checklist

- [ ] Remove compatibility aliases such as `use maplibre_native_core as support`
      and `crate::support`; use `core`-named imports or direct
      `maplibre_native_core` paths.
- [ ] Audit remaining direct `sys::` uses in `maplibre-native`; keep Rust handle
      operations, backend attach calls, and callback trampolines, and move any
      bridge-neutral ABI bookkeeping into `core`.
- [ ] Revisit custom geometry source option materialization. Move default
      option, field-mask, and native descriptor setup into `core` if it can be
      separated cleanly from Rust callback `user_data` and trampoline policy;
      otherwise document why it stays above `core`.
- [ ] Revisit resource/runtime descriptor setup that still initializes C structs
      in the public crate. Move bridge-neutral pieces to `core` or document the
      Rust policy that keeps them public.
- [ ] Move shared-adaptation tests from `maplibre-native` to
      `maplibre-native-core` when equivalent core coverage exists.
- [ ] Deduplicate tests after moving them. Keep public-crate tests focused on
      Rust-specific invariants: `!Send`/`!Sync`, parent retention, owner-thread
      close behavior, frame lifetimes, callback replacement, and public imports.
- [ ] Fix stale docs that imply `core` owns build/link utilities. `sys` owns raw
      ABI declarations and loading/link boundary details; `core` owns ABI
      adaptation.
- [ ] Run validation: `cargo fmt --all --manifest-path Cargo.toml`,
      `cargo clippy
      --manifest-path Cargo.toml -p maplibre-native-core -p maplibre-native
      --all-targets -- -D warnings`,
      and `mise run -C bindings/rust test`. Also run `mise run test` if cleanup
      changes public C ABI coverage, build configuration, or shared repository
      behavior.
- [ ] Run a parallel review of this branch against `bindings/rust/PLAN.md`; ask
      reviewers to inspect the diff directly and answer whether the refactor is
      complete for this PR scope, clean, correct, and net positive or negative.
      Apply or record the findings.

## Parallel review prompts

Use separate reviewers with distinct angles:

1. **Completeness:** Is the refactor complete for the branch/PR scope in
   `bindings/rust/PLAN.md`? Treat Python, Node.js, and JNI skeleton crates as
   out of scope. Identify remaining plan gaps with file references.
2. **Cleanliness:** Is the refactor clean? Check boundary clarity, compatibility
   aliases, duplicated code, test placement, stale docs, and PR hygiene.
3. **Correctness:** Is the refactor correct? Check C ABI invariants, ownership,
   thread affinity, RAII/result release, callback/resource request exactly-once
   behavior, and public Rust API compatibility.
4. **Net diff size:** Is the non-Markdown code/config diff net positive or
   negative in size? Exclude Markdown and planning artifacts. Report insertions,
   deletions, and major file/category contributors, then explain why the code
   expanded or contracted.
