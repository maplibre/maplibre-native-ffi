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

    // The ArkTS runtime keeps its Node-API implementation in a shared library,
    // where Node, Bun, and Deno export those symbols from the executable that
    // loads the addon. Without this the library dlopens far enough to be found
    // and then fails to relocate, which the runtime reports as a module with no
    // exports.
    if std::env::var("CARGO_CFG_TARGET_ENV").as_deref() == Ok("ohos") {
        println!("cargo:rustc-link-lib=dylib=ace_napi.z");
    }
    napi_build::setup();

    let include_dir = PathBuf::from(
        std::env::var("DEP_MAPLIBRE_NATIVE_C_INCLUDE_DIR")
            .expect("the sys crate reports the native include directory"),
    );
    let host_support = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../host-support");
    let generated = host_support.join("generated");

    check_header_digest(&include_dir, &generated.join("fingerprint.h"));

    println!("cargo:rerun-if-changed={}", host_support.display());
    let mut build = cc::Build::new();
    build
        .file(host_support.join("src/mln_abi.c"))
        .file(generated.join("layout_assert.c"))
        .include(host_support.join("include"))
        .include(&generated)
        .include(&include_dir)
        .warnings(true);
    // The public headers are C23, and a cross toolchain can be older than the
    // host's: the OpenHarmony SDK's Clang knows this standard only by its
    // working name. Both spellings give the shim what it needs.
    build.std(c_standard(&build));
    build.compile("mln_abi");

    // The sys crate's link directives are emitted before this crate's static
    // library, and the linker drops a shared library nothing has needed yet, so
    // the native library is named again here, after the objects that call it.
    let install_dir = include_dir
        .parent()
        .expect("the include directory sits inside an install prefix");
    let link_dir = install_dir.join("lib");
    println!("cargo:rustc-link-search=native={}", link_dir.display());
    println!("cargo:rustc-link-lib=dylib=maplibre-native-c");
    // Two rpaths, because the addon runs from two places. The build prefix
    // serves a developer running against the tree; the payload-relative one is
    // what a staged or installed package finds, where the native library sits
    // in the package's own lib directory.
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();
    if target_os == "macos" || target_os == "ios" {
        let runtime_dir = std::env::var("DEP_MAPLIBRE_NATIVE_C_RUNTIME_DIR")
            .unwrap_or_else(|_| link_dir.display().to_string());
        println!("cargo:rustc-link-arg=-Wl,-rpath,{runtime_dir}");
        println!("cargo:rustc-link-arg=-Wl,-rpath,@loader_path/lib");
    } else if cfg!(unix) {
        let runtime_dir = std::env::var("DEP_MAPLIBRE_NATIVE_C_RUNTIME_DIR")
            .unwrap_or_else(|_| link_dir.display().to_string());
        println!("cargo:rustc-link-arg=-Wl,-rpath,{runtime_dir}");
        println!("cargo:rustc-link-arg=-Wl,-rpath,$ORIGIN/lib");
    }
}

/// Reports the newest C standard this build's compiler accepts.
fn c_standard(build: &cc::Build) -> &'static str {
    let compiler = build
        .try_get_compiler()
        .expect("a C compiler for the target");
    for candidate in ["c23", "c2x"] {
        let probe = std::process::Command::new(compiler.path())
            .args(compiler.args())
            .arg(format!("-std={candidate}"))
            .args(["-fsyntax-only", "-xc", "-"])
            .stdin(std::process::Stdio::null())
            .stdout(std::process::Stdio::null())
            .stderr(std::process::Stdio::null())
            .status();
        if probe.is_ok_and(|status| status.success()) {
            return candidate;
        }
    }
    panic!("no C compiler accepting -std=c23 or -std=c2x");
}

/// Compares the artifact's headers against the digest the generation recorded.
fn check_header_digest(include_dir: &Path, fingerprint_header: &Path) {
    // The macro's value can sit on the next line, because the formatter wraps a
    // long define, so this reads the first quoted string after the name rather
    // than assuming one line.
    let header = std::fs::read_to_string(fingerprint_header)
        .expect("the generated fingerprint header is present");
    let recorded = header
        .split_once("MLN_ABI_HEADER_DIGEST")
        .and_then(|(_, rest)| rest.split_once('"'))
        .and_then(|(_, rest)| rest.split_once('"'))
        .map(|(value, _)| value.to_owned())
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
