use std::env;
use std::error::Error;
use std::path::{Path, PathBuf};

fn main() -> Result<(), Box<dyn Error>> {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR")?);
    let repo_root = repo_root_from_manifest_dir(&manifest_dir)?;
    let header = repo_root.join("include/maplibre_native_c.h");
    let include_dir = repo_root.join("include");
    let build_dir =
        PathBuf::from(env::var_os("MLN_FFI_BUILD_DIR").ok_or("MLN_FFI_BUILD_DIR is required")?);

    println!("cargo:rerun-if-env-changed=MLN_FFI_BUILD_DIR");
    println!("cargo:rerun-if-env-changed=MLN_FFI_DEPENDENCY_LIBRARY_DIR");
    println!("cargo:rerun-if-env-changed=LIBCLANG_PATH");
    println!("cargo:rerun-if-env-changed=BINDGEN_EXTRA_CLANG_ARGS");
    print_rerun_if_changed(&repo_root.join("include"));

    println!("cargo:rustc-link-search=native={}", build_dir.display());
    if let Some(dependency_library_dir) = dependency_library_dir() {
        println!(
            "cargo:rustc-link-search=native={}",
            dependency_library_dir.display()
        );
    }
    println!("cargo:rustc-link-lib=dylib=maplibre-native-c");

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

fn dependency_library_dir() -> Option<PathBuf> {
    env::var_os("MLN_FFI_DEPENDENCY_LIBRARY_DIR").map(PathBuf::from)
}

fn repo_root_from_manifest_dir(manifest_dir: &Path) -> Result<PathBuf, Box<dyn Error>> {
    manifest_dir
        .ancestors()
        .find(|ancestor| ancestor.join("include/maplibre_native_c.h").is_file())
        .map(Path::to_path_buf)
        .ok_or_else(|| {
            format!(
                "could not locate repository root containing include/maplibre_native_c.h from {}",
                manifest_dir.display()
            )
            .into()
        })
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
