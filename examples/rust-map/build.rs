use std::env;
use std::path::Path;
use std::process::Command;

fn main() {
    println!(
        "cargo:rustc-check-cfg=cfg(maplibre_render_backend, values(\"metal\", \"opengl\", \"vulkan\"))"
    );
    println!("cargo:rerun-if-env-changed=MAPLIBRE_NATIVE_FFI_LOCAL_BUILD");
    let install_dir = env::var_os("MAPLIBRE_NATIVE_FFI_LOCAL_BUILD")
        .expect("MAPLIBRE_NATIVE_FFI_LOCAL_BUILD is required");
    if env::var("CARGO_CFG_TARGET_FAMILY").as_deref() == Ok("unix") {
        println!(
            "cargo:rustc-link-arg=-Wl,-rpath,{}/lib",
            Path::new(&install_dir).display()
        );
    }
    // The sys crate reports the backend of the library it linked.
    let backend = env::var("DEP_MAPLIBRE_NATIVE_C_RENDER_BACKEND")
        .expect("maplibre-native-ffi-sys reports its render backend");
    println!("cargo:rustc-cfg=maplibre_render_backend=\"{backend}\"");

    if backend != "vulkan" {
        return;
    }

    let out_dir = env::var_os("OUT_DIR").expect("Cargo must provide OUT_DIR");
    for shader in ["fullscreen.vert", "sample.frag"] {
        let source = Path::new("src/vulkan_texture_compositor/shaders").join(shader);
        let output = Path::new(&out_dir).join(format!("{shader}.spv"));
        println!("cargo:rerun-if-changed={}", source.display());
        let status = Command::new("glslangValidator")
            .args(["-V"])
            .arg(&source)
            .args(["-o"])
            .arg(&output)
            .status()
            .expect("glslangValidator is required for the Vulkan example");
        assert!(status.success(), "failed to compile {}", source.display());
    }
}
