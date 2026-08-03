//! Builds the shared host-support shim into the Node-API addon.
//!
//! The generated dispatch table is checked in, so it describes one exact library
//! ABI. This build refuses a native artifact whose public headers differ from
//! the ones the generation ran against, rather than warning: a signature change
//! behind an unchanged symbol name would still link, and the call ABI indexes
//! entrypoints rather than naming them.

use std::path::{Path, PathBuf};

use sha2::{Digest, Sha256};

fn main() {
    napi_build::setup();

    let include_dir = PathBuf::from(
        std::env::var("DEP_MAPLIBRE_NATIVE_C_INCLUDE_DIR")
            .expect("the sys crate reports the native include directory"),
    );
    let host_support = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../host-support");
    let generated = host_support.join("generated");

    check_header_digest(&include_dir, &generated.join("fingerprint.h"));

    println!("cargo:rerun-if-changed={}", host_support.display());
    cc::Build::new()
        .file(host_support.join("src/mln_abi.c"))
        .file(generated.join("layout_assert.c"))
        .include(host_support.join("include"))
        .include(&generated)
        .include(&include_dir)
        .std("c23")
        .warnings(true)
        .compile("mln_abi");

    // The sys crate's link directives are emitted before this crate's static
    // library, and the linker drops a shared library nothing has needed yet, so
    // the native library is named again here, after the objects that call it.
    let install_dir = include_dir
        .parent()
        .expect("the include directory sits inside an install prefix");
    let link_dir = install_dir.join("lib");
    println!("cargo:rustc-link-search=native={}", link_dir.display());
    println!("cargo:rustc-link-lib=dylib=maplibre-native-c");
    if cfg!(unix) {
        let runtime_dir = std::env::var("DEP_MAPLIBRE_NATIVE_C_RUNTIME_DIR")
            .unwrap_or_else(|_| link_dir.display().to_string());
        println!("cargo:rustc-link-arg=-Wl,-rpath,{runtime_dir}");
    }
}

/// Compares the artifact's headers against the digest the generation recorded.
fn check_header_digest(include_dir: &Path, fingerprint_header: &Path) {
    let recorded = std::fs::read_to_string(fingerprint_header)
        .expect("the generated fingerprint header is present")
        .lines()
        .find_map(|line| {
            line.strip_prefix("#define MLN_ABI_HEADER_DIGEST \"")
                .and_then(|rest| rest.strip_suffix('"'))
                .map(str::to_owned)
        })
        .expect("the generated fingerprint header records a header digest");

    let mut paths = Vec::new();
    collect_headers(include_dir, &mut paths);
    paths.sort();
    let mut hasher = Sha256::new();
    for path in &paths {
        let relative = path
            .strip_prefix(include_dir)
            .expect("header paths sit under the include directory");
        hasher.update(relative.to_string_lossy().replace('\\', "/").as_bytes());
        hasher.update([0]);
        hasher.update(std::fs::read(path).expect("header is readable"));
        hasher.update([0]);
    }
    let actual: String = hasher
        .finalize()
        .iter()
        .map(|byte| format!("{byte:02x}"))
        .collect();

    assert!(
        actual == recorded,
        "the native artifact at {} ships public headers this binding was not generated \
         against ({actual} rather than {recorded}). Rebuild the native library from this \
         checkout, or regenerate with `mise run //bindings/typescript:generate`.",
        include_dir.display()
    );
}

fn collect_headers(directory: &Path, paths: &mut Vec<PathBuf>) {
    for entry in std::fs::read_dir(directory).expect("the include directory is readable") {
        let path = entry.expect("directory entries are readable").path();
        if path.is_dir() {
            collect_headers(&path, paths);
        } else if path.extension().is_some_and(|extension| extension == "h") {
            paths.push(path);
        }
    }
}
