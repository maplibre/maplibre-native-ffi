use std::env;
use std::path::PathBuf;
use std::process::{Command, Stdio};

/// Builds browser render fixture support.
fn main() {
    println!("cargo:rerun-if-env-changed=CARGO_CFG_TARGET_OS");
    println!("cargo:rustc-check-cfg=cfg(mln_webgpu_backend)");
    if env::var("CARGO_CFG_TARGET_OS").as_deref() != Ok("emscripten") {
        return;
    }

    let emscripten =
        PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("cargo sets the manifest dir"))
            .join("emscripten");

    // WebGL fixture context registry.
    let canvas_library = emscripten.join("test_support.js");
    println!("cargo:rerun-if-changed={}", canvas_library.display());
    println!(
        "cargo:rustc-link-arg=--js-library={}",
        canvas_library.display()
    );

    // Only WebGPU artifacts need WebGPU fixtures.
    if env::var("DEP_MAPLIBRE_NATIVE_C_RENDER_BACKEND").as_deref() != Ok("webgpu") {
        return;
    }
    println!("cargo:rustc-cfg=mln_webgpu_backend");
    generate_webgpu_bindings();
}

/// Binds the emdawnwebgpu header linked into this module.
fn generate_webgpu_bindings() {
    let emsdk = env::var("EMSDK")
        .expect("EMSDK is required to build the browser fixtures for wasm32-unknown-emscripten");
    let sysroot = PathBuf::from(&emsdk).join("upstream/emscripten/cache/sysroot");
    let port_include = webgpu_port_include_dir();
    let header = port_include.join("webgpu/webgpu.h");
    println!("cargo:rerun-if-env-changed=EMSDK");
    println!("cargo:rerun-if-changed={}", header.display());

    let bindings = bindgen::Builder::default()
        .header(header.display().to_string())
        .clang_arg("-xc")
        .clang_arg("--target=wasm32-unknown-emscripten")
        .clang_arg(format!("--sysroot={}", sysroot.display()))
        .clang_arg(format!("-I{}", port_include.display()))
        // Keep WebGPU functions visible to bindgen.
        .clang_arg("-fvisibility=default")
        .allowlist_function("^wgpu.*")
        .allowlist_type("^WGPU.*")
        .allowlist_var("^WGPU.*")
        .prepend_enum_name(false)
        .layout_tests(false)
        .generate()
        .expect("the emdawnwebgpu port's webgpu.h is bindable");

    let out_dir = PathBuf::from(env::var("OUT_DIR").expect("cargo sets the out dir"));
    bindings
        .write_to_file(out_dir.join("webgpu.rs"))
        .expect("writing the generated WebGPU bindings");
}

/// Finds the emdawnwebgpu include directory from emcc's search list.
fn webgpu_port_include_dir() -> PathBuf {
    let output = Command::new("emcc")
        .args(["--use-port=emdawnwebgpu", "-xc", "-E", "-v", "-"])
        .stdin(Stdio::null())
        .output()
        .expect("emcc runs the browser build and is on PATH for this target");
    let search_list = String::from_utf8_lossy(&output.stderr);
    search_list
        .lines()
        .map(str::trim)
        .find(|line| line.ends_with("emdawnwebgpu_pkg/webgpu/include"))
        .map(PathBuf::from)
        .expect("emcc lists the emdawnwebgpu port's include directory for --use-port")
}
