//! Compiles the binding's Node-API C for the module, against the C headers,
//! Node-API's headers, and the freestanding C library in libc/.

use std::env;
use std::path::PathBuf;

fn main() {
    let web =
        PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").expect("Cargo sets CARGO_MANIFEST_DIR"));
    let typescript = web
        .parent()
        .expect("web/ is in bindings/typescript")
        .to_path_buf();
    let root = typescript
        .parent()
        .and_then(|bindings| bindings.parent())
        .expect("bindings/ is in the repository");
    let native = typescript.join("native");
    let support = typescript.join("test/support");
    let node_api = env::var_os("MLN_FFI_NODE_API_HEADERS").map_or_else(
        || typescript.join("node_modules/node-api-headers/include"),
        PathBuf::from,
    );
    assert!(
        node_api.join("node_api.h").exists(),
        "node_api.h not found in {}; install the TypeScript binding's packages first",
        node_api.display()
    );

    let mut sources = vec![
        native.join("generated_addon.c"),
        native.join("generated_values.c"),
        native.join("realm.c"),
        native.join("runtime.c"),
        web.join("libc/libc.c"),
    ];
    if env::var_os("CARGO_FEATURE_TEST").is_some() {
        sources.push(support.join("native_probe.c"));
    }

    let mut build = cc::Build::new();
    build
        .files(&sources)
        .include(web.join("libc/include"))
        .include(root.join("include"))
        .include(&native)
        .include(&support)
        .include(&node_api)
        .std("c11")
        // The module defines Node-API itself, rather than importing it from a
        // host as node_api.h's WebAssembly default declares.
        .define("NAPI_EXTERN", "")
        .define("NAPI_VERSION", "8")
        .warnings(true)
        .extra_warnings(true)
        .flag("-Wno-unused-parameter");
    build.compile("maplibre_native_ffi_node_api");

    for source in &sources {
        println!("cargo::rerun-if-changed={}", source.display());
    }
    for directory in [
        web.join("libc/include"),
        root.join("include"),
        native,
        support,
    ] {
        println!("cargo::rerun-if-changed={}", directory.display());
    }
    println!("cargo::rerun-if-env-changed=MLN_FFI_NODE_API_HEADERS");
}
