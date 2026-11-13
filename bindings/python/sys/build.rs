//! Links the MapLibre Native C library from the install prefix that
//! `MAPLIBRE_NATIVE_FFI_LOCAL_BUILD` names.

use std::env;
use std::error::Error;
use std::fs;
use std::io;
use std::path::{Path, PathBuf};

use serde::Deserialize;

const LIBRARY_NAME: &str = "maplibre-native-c";
const LOCAL_BUILD_ENV: &str = "MAPLIBRE_NATIVE_FFI_LOCAL_BUILD";
const DESCRIPTOR_PATH: &str = "share/maplibre-native-c/artifact.json";

fn main() -> Result<(), Box<dyn Error>> {
    println!("cargo:rerun-if-env-changed={LOCAL_BUILD_ENV}");
    let target_os = env::var("CARGO_CFG_TARGET_OS")?;
    let target_env = env::var("CARGO_CFG_TARGET_ENV").unwrap_or_default();
    let out_path = PathBuf::from(env::var("OUT_DIR")?);

    let target = env::var("TARGET")?;
    let install_dir = local_prefix(&target)?;
    let link_dir = native_library_dir(&install_dir);
    let descriptor = read_descriptor(&install_dir)?;

    if target_env == "musl" {
        let aliases = musl_aliases(&out_path)?;
        println!("cargo:rustc-link-search=native={}", aliases.display());
    }
    println!("cargo:rustc-link-search=native={}", link_dir.display());
    print_rerun_if_changed(&link_dir);
    println!(
        "cargo:rerun-if-changed={}",
        install_dir.join(DESCRIPTOR_PATH).display()
    );

    println!(
        "cargo:rustc-link-lib=static={}",
        static_archive(&link_dir, &target_os, &target_env)?
    );
    // The archive merges in every dependency it can. What is left is the
    // platform and render backend's system libraries, which CMake records.
    for library in &descriptor.link.libraries {
        // Rust's self-contained musl sysroot includes these interfaces in libc
        // and provides no separate alias archives for them.
        if target_env == "musl" && matches!(library.as_str(), "m" | "dl" | "pthread" | "rt") {
            continue;
        }
        println!("cargo:rustc-link-lib={library}");
    }
    for framework in &descriptor.link.frameworks {
        println!("cargo:rustc-link-lib=framework={framework}");
    }
    Ok(())
}

fn local_prefix(target: &str) -> Result<PathBuf, Box<dyn Error>> {
    let prefix = env::var_os(LOCAL_BUILD_ENV)
        .map(PathBuf::from)
        .ok_or_else(|| format!("{LOCAL_BUILD_ENV} must name a native install prefix"))?;
    // A relative path would resolve against this crate's own directory, which
    // the caller neither chooses nor sees.
    if !prefix.is_absolute() {
        return Err(format!(
            "{LOCAL_BUILD_ENV} must be an absolute path; found {}",
            prefix.display()
        )
        .into());
    }
    if !prefix.join(DESCRIPTOR_PATH).is_file() {
        return Err(format!(
            "{LOCAL_BUILD_ENV} names {}, which is not an install prefix because it lacks \
             {DESCRIPTOR_PATH}",
            prefix.display()
        )
        .into());
    }
    let descriptor = read_descriptor(&prefix)?;
    if descriptor.platform.rust_target != target {
        return Err(format!(
            "{LOCAL_BUILD_ENV} names an install prefix built for {}, which cannot serve {target}",
            descriptor.platform.rust_target,
        )
        .into());
    }
    Ok(prefix)
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
struct Platform {
    rust_target: String,
}

#[derive(Deserialize)]
struct ArtifactDescriptor {
    platform: Platform,
    link: Link,
}

#[derive(Deserialize)]
struct Link {
    libraries: Vec<String>,
    frameworks: Vec<String>,
}

fn read_descriptor(prefix: &Path) -> Result<ArtifactDescriptor, Box<dyn Error>> {
    let path = prefix.join(DESCRIPTOR_PATH);
    let contents = fs::read_to_string(&path)
        .map_err(|error| format!("failed to read {}: {error}", path.display()))?;
    serde_json::from_str(&contents)
        .map_err(|error| format!("invalid artifact descriptor {}: {error}", path.display()).into())
}

/// Supplies musl's traditional library names without a second implementation.
/// The complete native archive provides the unified libc interfaces and the
/// process-wide unwinder.
fn musl_aliases(out_path: &Path) -> Result<PathBuf, io::Error> {
    let directory = out_path.join("musl-aliases");
    fs::create_dir_all(&directory)?;
    for library in ["dl", "m", "pthread", "rt", "unwind"] {
        fs::write(directory.join(format!("lib{library}.a")), b"!<arch>\n")?;
    }
    Ok(directory)
}

/// The complete static archive's library name.
///
/// MSVC's shared build uses `.lib` for its import library too, so its static
/// archive carries a `-static` suffix. MinGW uses `.dll.a` for imports and the
/// usual `.a` for the static archive.
fn static_archive(
    link_dir: &Path,
    target_os: &str,
    target_env: &str,
) -> Result<String, Box<dyn Error>> {
    let (name, file) = if target_os == "windows" && target_env == "msvc" {
        let name = format!("{LIBRARY_NAME}-static");
        let file = format!("{name}.lib");
        (name, file)
    } else {
        (LIBRARY_NAME.to_owned(), format!("lib{LIBRARY_NAME}.a"))
    };
    if !link_dir.join(&file).is_file() {
        return Err(format!(
            "missing the complete static archive {file} in {}",
            link_dir.display()
        )
        .into());
    }
    Ok(name)
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
