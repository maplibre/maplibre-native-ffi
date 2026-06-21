use std::env;
use std::error::Error;
use std::io;
use std::path::{Path, PathBuf};

const LIBRARY_NAME: &str = "maplibre-native-c";

fn main() -> Result<(), Box<dyn Error>> {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR")?);
    let repo_root = repo_root_from_manifest_dir(&manifest_dir)?;
    let header = repo_root.join("include/maplibre_native_c.h");

    println!("cargo:rerun-if-env-changed=PKG_CONFIG_PATH");
    print_rerun_if_pkg_config_file_changed();

    let library = pkg_config::Config::new().probe(LIBRARY_NAME).map_err(|error| {
        io::Error::other(format!(
            "could not find {LIBRARY_NAME} with pkg-config; run through mise or add the generated maplibre-native-c.pc directory to PKG_CONFIG_PATH: {error}"
        ))
    })?;

    println!("cargo:rerun-if-env-changed=LIBCLANG_PATH");
    println!("cargo:rerun-if-env-changed=BINDGEN_EXTRA_CLANG_ARGS");
    print_rerun_if_changed(&repo_root.join("include"));

    let mut bindings = bindgen::Builder::default()
        .header(header.display().to_string())
        .clang_arg("-xc")
        .clang_arg("-std=c23");
    for include_path in &library.include_paths {
        bindings = bindings.clang_arg(format!("-I{}", include_path.display()));
    }
    let bindings = bindings
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

fn print_rerun_if_pkg_config_file_changed() {
    let Some(paths) = env::var_os("PKG_CONFIG_PATH") else {
        return;
    };

    for path in env::split_paths(&paths) {
        let pc_file = path.join(format!("{LIBRARY_NAME}.pc"));
        if pc_file.is_file() {
            println!("cargo:rerun-if-changed={}", pc_file.display());
        }
    }
}
