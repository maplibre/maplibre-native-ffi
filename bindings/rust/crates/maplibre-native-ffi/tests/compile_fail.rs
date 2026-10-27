//! The borrows that keep lent native storage valid, checked by the code they
//! reject.

#[cfg(mln_host_build)]
#[test]
fn lent_native_storage_cannot_outlive_its_lender() {
    trybuild::TestCases::new().compile_fail("tests/compile_fail/*.rs");
}
