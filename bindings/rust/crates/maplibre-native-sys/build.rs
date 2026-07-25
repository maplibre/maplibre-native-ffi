use std::env;
use std::error::Error;
use std::io;
use std::path::{Path, PathBuf};

const LIBRARY_NAME: &str = "maplibre-native-c";

fn main() -> Result<(), Box<dyn Error>> {
    println!("cargo:rerun-if-env-changed=MAPLIBRE_NATIVE_C_INSTALL_DIR");
    println!("cargo:rerun-if-env-changed=CARGO_CFG_TARGET_FAMILY");
    println!("cargo:rerun-if-env-changed=CARGO_CFG_TARGET_OS");
    let install_dir = native_install_dir()?;
    let include_dir = install_dir.join("include");
    let link_dir = native_library_dir(&install_dir);
    let target_os = env::var("CARGO_CFG_TARGET_OS")?;
    let target_family = env::var("CARGO_CFG_TARGET_FAMILY")?;
    let runtime_dir = native_runtime_dir(&install_dir, &target_os);
    let header = include_dir.join("maplibre_native_c.h");

    require_dir(&include_dir, "native include directory")?;
    require_dir(&link_dir, "native link directory")?;
    require_dir(&runtime_dir, "native runtime library directory")?;

    println!("cargo:rustc-link-search=native={}", link_dir.display());
    println!("cargo:rustc-link-lib={LIBRARY_NAME}");
    if target_family.split(',').any(|family| family == "unix") {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", runtime_dir.display());
    }
    println!("cargo:rerun-if-env-changed=LIBCLANG_PATH");
    println!("cargo:rerun-if-env-changed=BINDGEN_EXTRA_CLANG_ARGS");
    print_rerun_if_changed(&include_dir);

    let bindings = bindgen::Builder::default()
        .header(header.display().to_string())
        .clang_arg("-xc")
        .clang_arg("-std=c23")
        .clang_arg(format!("-I{}", include_dir.display()))
        .allowlist_function("^mln_.*")
        .allowlist_type("^mln_.*")
        .allowlist_var("^MLN_.*")
        .prepend_enum_name(false)
        .layout_tests(true)
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()?;

    let out_path = PathBuf::from(env::var("OUT_DIR")?);
    bindings.write_to_file(out_path.join("bindings.rs"))?;

    Ok(())
}

fn native_install_dir() -> Result<PathBuf, Box<dyn Error>> {
    let install_dir = env::var_os("MAPLIBRE_NATIVE_C_INSTALL_DIR").ok_or_else(|| {
        io::Error::new(
            io::ErrorKind::NotFound,
            "MAPLIBRE_NATIVE_C_INSTALL_DIR is required",
        )
    })?;
    Ok(PathBuf::from(install_dir))
}

fn native_runtime_dir(install_dir: &Path, target_os: &str) -> PathBuf {
    if target_os == "windows" {
        install_dir.join("bin")
    } else {
        native_library_dir(install_dir)
    }
}

fn native_library_dir(install_dir: &Path) -> PathBuf {
    for dirname in ["lib", "lib64"] {
        let candidate = install_dir.join(dirname);
        if candidate.is_dir() {
            return candidate;
        }
    }
    install_dir.join("lib")
}

fn require_dir(path: &Path, label: &str) -> Result<(), Box<dyn Error>> {
    if path.is_dir() {
        return Ok(());
    }
    Err(io::Error::new(
        io::ErrorKind::NotFound,
        format!(
            "missing {label}: {}; run `mise run build` first",
            path.display()
        ),
    )
    .into())
}

fn print_rerun_if_changed(path: &Path) {
    if path.is_file() {
        println!("cargo:rerun-if-changed={}", path.display());
        return;
    }

    let Ok(entries) = std::fs::read_dir(path) else {
        return;
    };
    for entry in entries.flatten() {
        print_rerun_if_changed(&entry.path());
    }
}
