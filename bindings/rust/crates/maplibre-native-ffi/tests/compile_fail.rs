//! The borrows that keep lent native storage valid, and the ownership of
//! queued deliveries, checked by the code they reject.

// A cross-built test binary takes its unwinder from the native library, so it
// links the crate even where the compile-fail cases don't run.
#[cfg(not(mln_host_build))]
use maplibre_native_ffi as _;

#[cfg(mln_host_build)]
#[test]
fn lent_native_storage_cannot_outlive_its_lender() {
    trybuild::TestCases::new().compile_fail("tests/compile_fail/*.rs");
}
