use std::env;
use std::path::Path;

fn main() {
    println!("cargo:rerun-if-env-changed=MAPLIBRE_NATIVE_C_INSTALL_DIR");
    if env::var("CARGO_CFG_TARGET_FAMILY").as_deref() != Ok("unix") {
        return;
    }
    let install_dir = env::var_os("MAPLIBRE_NATIVE_C_INSTALL_DIR")
        .expect("MAPLIBRE_NATIVE_C_INSTALL_DIR is required");
    println!(
        "cargo:rustc-link-arg=-Wl,-rpath,{}/lib",
        Path::new(&install_dir).display()
    );
}
