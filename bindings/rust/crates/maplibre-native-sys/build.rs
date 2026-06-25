use std::env;
use std::error::Error;
use std::io;
use std::path::{Path, PathBuf};

use serde::Deserialize;

const LIBRARY_NAME: &str = "maplibre-native-c";

#[derive(Deserialize)]
struct Artifact {
    include_dirs: Vec<PathBuf>,
    import_library_path: PathBuf,
    rpaths: Vec<PathBuf>,
    supports_linker_rpath: bool,
}

fn main() -> Result<(), Box<dyn Error>> {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR")?);
    let repo_root = repo_root_from_manifest_dir(&manifest_dir)?;
    let header = repo_root.join("include/maplibre_native_c.h");

    println!("cargo:rerun-if-env-changed=MLN_FFI_BUILD_DIR");
    let artifact = load_artifact()?;
    let import_library_dir = artifact.import_library_path.parent().ok_or_else(|| {
        io::Error::new(
            io::ErrorKind::InvalidData,
            format!(
                "native metadata import_library_path has no parent directory: {}",
                artifact.import_library_path.display()
            ),
        )
    })?;
    println!(
        "cargo:rustc-link-search=native={}",
        import_library_dir.display()
    );
    println!("cargo:rustc-link-lib={LIBRARY_NAME}");
    if artifact.supports_linker_rpath {
        for rpath in &artifact.rpaths {
            println!("cargo:rustc-link-arg=-Wl,-rpath,{}", rpath.display());
        }
    }

    println!("cargo:rerun-if-env-changed=LIBCLANG_PATH");
    println!("cargo:rerun-if-env-changed=BINDGEN_EXTRA_CLANG_ARGS");
    print_rerun_if_changed(&repo_root.join("include"));

    let mut bindings = bindgen::Builder::default()
        .header(header.display().to_string())
        .clang_arg("-xc")
        .clang_arg("-std=c23");
    for include_path in &artifact.include_dirs {
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

fn load_artifact() -> Result<Artifact, Box<dyn Error>> {
    let build_dir = env::var_os("MLN_FFI_BUILD_DIR").ok_or_else(|| {
        io::Error::new(
            io::ErrorKind::NotFound,
            "MLN_FFI_BUILD_DIR is required; run Rust binding builds through mise",
        )
    })?;
    let metadata_path = PathBuf::from(build_dir).join(format!("{LIBRARY_NAME}.dev.json"));
    println!("cargo:rerun-if-changed={}", metadata_path.display());

    let metadata = std::fs::read_to_string(&metadata_path).map_err(|error| {
        io::Error::new(
            error.kind(),
            format!(
                "failed to read native artifact metadata at {}; run `mise run build` first: {error}",
                metadata_path.display()
            ),
        )
    })?;
    let artifact = serde_json::from_str::<Artifact>(&metadata).map_err(|error| {
        io::Error::new(
            io::ErrorKind::InvalidData,
            format!(
                "invalid native artifact metadata at {}: {error}",
                metadata_path.display()
            ),
        )
    })?;

    Ok(artifact)
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
