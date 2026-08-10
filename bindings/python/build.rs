use std::env;

fn main() {
    println!("cargo:rerun-if-env-changed=CARGO_CFG_TARGET_OS");

    // Android loads extension modules into an app process that does not export
    // CPython's symbols globally, so the extension must link libpython itself.
    if env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("android") {
        println!("cargo:rustc-link-lib=dylib=python3.14");
    }
}
