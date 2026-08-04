use std::env;
use std::path::PathBuf;
use std::process::{Command, Stdio};

/// Builds what the browser render fixtures call into.
///
/// Everything here is scoped to this package's link targets, which is the scope
/// it has: nothing outside the fixtures uses it, and a host linking its own
/// module should not have to carry it. Cargo fingerprints this script's output
/// rather than the files it names, so each input is declared too, or an edit to
/// one would leave the previous module linked.
fn main() {
    println!("cargo:rerun-if-env-changed=CARGO_CFG_TARGET_OS");
    println!("cargo:rustc-check-cfg=cfg(mln_webgpu_backend)");
    if env::var("CARGO_CFG_TARGET_OS").as_deref() != Ok("emscripten") {
        return;
    }

    let emscripten =
        PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("cargo sets the manifest dir"))
            .join("emscripten");

    // The OffscreenCanvas registry the WebGL fixtures create their contexts on.
    // JavaScript rather than a compiled shim, so a Rust test binary needs no C
    // toolchain for the backend every browser has.
    let canvas_library = emscripten.join("test_support.js");
    println!("cargo:rerun-if-changed={}", canvas_library.display());
    println!(
        "cargo:rustc-link-arg=--js-library={}",
        canvas_library.display()
    );

    // The artifact compiles one renderer, and only a WebGPU one has a device for
    // the fixtures to borrow. The sys crate reports which from the installed
    // descriptor; see its build script.
    if env::var("DEP_MAPLIBRE_NATIVE_C_RENDER_BACKEND").as_deref() != Ok("webgpu") {
        return;
    }
    println!("cargo:rustc-cfg=mln_webgpu_backend");
    generate_webgpu_bindings();
}

/// Binds the WebGPU header this module links against.
///
/// Generated rather than taken from a crate, because the fixtures hand their
/// device to a session as an opaque handle: it has to come from the very
/// emdawnwebgpu instance the core links, so a crate carrying its own WebGPU
/// implementation would produce handles from somewhere else, and one carrying
/// its own copy of the header would pin a revision this port need not agree
/// with.
///
/// The port's header is what `--use-port=emdawnwebgpu` puts ahead of the
/// sysroot, whose `webgpu/webgpu.h` is Emscripten's older built-in one and a
/// different ABI. emcc is asked where that is rather than told, because the
/// path carries the port's own package layout.
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
        // What emcc passes and libclang, driven directly, does not: a wasm
        // target hides declarations by default, and bindgen generates nothing
        // for a hidden function. The port's header spells no visibility of its
        // own, so without this the bindings come out with every type and no
        // function at all, and no diagnostic to say why.
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

/// The include directory the emdawnwebgpu port contributes, taken from emcc's
/// own search list so this tracks the port rather than guessing its layout.
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
