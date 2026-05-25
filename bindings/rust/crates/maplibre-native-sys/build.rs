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

    let dependency_library_dir = dependency_library_dir();
    set_repo_libclang_path(&repo_root, dependency_library_dir.as_deref());

    println!("cargo:rustc-link-search=native={}", build_dir.display());
    if let Some(dependency_library_dir) = dependency_library_dir {
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

fn set_repo_libclang_path(repo_root: &Path, dependency_library_dir: Option<&Path>) {
    if env::var("LIBCLANG_PATH").is_ok_and(|value| !value.is_empty()) {
        return;
    }

    for libclang_path in libclang_path_candidates(repo_root, dependency_library_dir) {
        if has_libclang(&libclang_path) {
            // SAFETY: this build script sets LIBCLANG_PATH before bindgen starts
            // and before any other threads are created in this process.
            unsafe {
                env::set_var("LIBCLANG_PATH", libclang_path);
            }
            return;
        }
    }
}

fn libclang_path_candidates(
    repo_root: &Path,
    dependency_library_dir: Option<&Path>,
) -> Vec<PathBuf> {
    let mut candidates = Vec::new();
    if let Some(dependency_library_dir) = dependency_library_dir {
        if cfg!(windows) {
            if let Some(parent) = dependency_library_dir.parent() {
                candidates.push(parent.join("bin"));
            }
        } else {
            candidates.push(dependency_library_dir.to_path_buf());
        }
    }
    candidates.push(if cfg!(windows) {
        repo_root.join(".pixi/envs/default/Library/bin")
    } else {
        repo_root.join(".pixi/envs/default/lib")
    });
    candidates
}

fn has_libclang(path: &Path) -> bool {
    if cfg!(windows) {
        return path.join("libclang.dll").is_file() || path.join("clang.dll").is_file();
    }
    path.join("libclang.so").is_file() || path.join("libclang.dylib").is_file()
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
