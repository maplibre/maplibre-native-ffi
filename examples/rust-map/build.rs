use std::env;
use std::fs;
use std::path::Path;
use std::process::Command;

fn main() {
    println!(
        "cargo:rustc-check-cfg=cfg(maplibre_render_backend, values(\"metal\", \"opengl\", \"vulkan\"))"
    );
    println!("cargo:rerun-if-env-changed=MAPLIBRE_NATIVE_C_INSTALL_DIR");
    let install_dir = env::var_os("MAPLIBRE_NATIVE_C_INSTALL_DIR")
        .expect("MAPLIBRE_NATIVE_C_INSTALL_DIR is required");
    if env::var("CARGO_CFG_TARGET_FAMILY").as_deref() == Ok("unix") {
        println!(
            "cargo:rustc-link-arg=-Wl,-rpath,{}/lib",
            Path::new(&install_dir).display()
        );
    }
    let descriptor_path = Path::new(&install_dir).join("share/maplibre-native-c/artifact.json");
    let descriptor = fs::read_to_string(&descriptor_path)
        .unwrap_or_else(|error| panic!("failed to read {}: {error}", descriptor_path.display()));
    let backend = ["metal", "opengl", "vulkan"]
        .into_iter()
        .find(|backend| descriptor.contains(&format!("\"renderBackend\": \"{backend}\"")))
        .expect("native artifact descriptor has an unknown render backend");
    println!("cargo:rerun-if-changed={}", descriptor_path.display());
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
