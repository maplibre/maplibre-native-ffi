use std::env;
use std::error::Error;
use std::io;
use std::path::PathBuf;

use serde::Deserialize;

const LIBRARY_NAME: &str = "maplibre-native-c";

#[derive(Deserialize)]
struct Artifact {
    import_library_path: PathBuf,
    #[serde(default)]
    library_dirs: Vec<PathBuf>,
    #[serde(default)]
    link_libraries: Vec<String>,
    #[serde(default)]
    rpaths: Vec<PathBuf>,
    #[serde(default)]
    frameworks: Vec<String>,
}

fn main() -> Result<(), Box<dyn Error>> {
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
    for library_dir in artifact.library_dirs {
        println!("cargo:rustc-link-search=native={}", library_dir.display());
    }

    let link_libraries = if artifact.link_libraries.is_empty() {
        vec![LIBRARY_NAME.to_string()]
    } else {
        artifact.link_libraries
    };
    for link_library in link_libraries {
        println!("cargo:rustc-link-lib={link_library}");
    }
    for framework in artifact.frameworks {
        println!("cargo:rustc-link-lib=framework={framework}");
    }
    for rpath in artifact.rpaths {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", rpath.display());
    }

    Ok(())
}

fn load_artifact() -> Result<Artifact, Box<dyn Error>> {
    let build_dir = env::var_os("MLN_FFI_BUILD_DIR").ok_or_else(|| {
        io::Error::new(
            io::ErrorKind::NotFound,
            "MLN_FFI_BUILD_DIR is required; run Python binding builds through mise",
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
