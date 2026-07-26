use std::env;

const TEST_BACKENDS_ENV: &str = "MAPLIBRE_NATIVE_NODE_TEST_BACKENDS";

fn main() {
    println!("cargo:rerun-if-env-changed={TEST_BACKENDS_ENV}");
    println!(
        "cargo:rustc-check-cfg=cfg(node_test_backend, values(\"metal\", \"egl\", \"wgl\", \"vulkan\"))"
    );

    if env::var_os("CARGO_FEATURE_TEST_SUPPORT").is_some() {
        configure_test_backend();
    }

    napi_build::setup();
}

fn configure_test_backend() {
    let selection = env::var(TEST_BACKENDS_ENV).unwrap_or_else(|_| {
        panic!("{TEST_BACKENDS_ENV} must select exactly one backend when test-support is enabled")
    });
    let backends: Vec<_> = selection
        .split(',')
        .map(str::trim)
        .filter(|backend| !backend.is_empty())
        .collect();
    assert!(
        backends.len() == 1,
        "{TEST_BACKENDS_ENV} must select exactly one backend, got {selection:?}"
    );

    let backend = backends[0];
    let target_os = env::var("CARGO_CFG_TARGET_OS").expect("Cargo must set CARGO_CFG_TARGET_OS");
    let supported_target = match backend {
        "metal" => target_os == "macos",
        "egl" => matches!(target_os.as_str(), "linux" | "macos"),
        "wgl" => target_os == "windows",
        "vulkan" => matches!(target_os.as_str(), "linux" | "macos" | "windows"),
        _ => panic!(
            "{TEST_BACKENDS_ENV} contains unknown backend {backend:?}; expected metal, egl, wgl, or vulkan"
        ),
    };
    assert!(
        supported_target,
        "test backend {backend:?} is unsupported on target OS {target_os:?}"
    );
    println!("cargo:rustc-cfg=node_test_backend=\"{backend}\"");
}
