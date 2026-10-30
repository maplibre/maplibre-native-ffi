//! Links the MapLibre Native C library: from a local build that the consumer
//! names, or else from the release that this crate was published against.

use std::env;
use std::error::Error;
use std::fs;
use std::io;
use std::io::Read as _;
use std::path::{Path, PathBuf};
use std::time::Duration;

use flate2::read::GzDecoder;
use serde::Deserialize;
use sha2::{Digest, Sha256};

const LIBRARY_NAME: &str = "maplibre-native-c";
const LOCAL_BUILD_ENV: &str = "MAPLIBRE_NATIVE_FFI_LOCAL_BUILD";
const RELEASE_URL_ENV: &str = "MAPLIBRE_NATIVE_FFI_RELEASE_URL";
const DEFAULT_RELEASE_URL: &str =
    "https://github.com/maplibre/maplibre-native-ffi/releases/download";
const DESCRIPTOR_PATH: &str = "share/maplibre-native-c/artifact.json";
const BACKENDS: [&str; 4] = ["metal", "opengl", "vulkan", "webgpu"];

/// The artifact index of the release that a published crate downloads. The
/// release workflow writes this file, and git ignores it.
const PIN_PATH: &str = "native_release.json";

fn main() -> Result<(), Box<dyn Error>> {
    println!("cargo:rustc-check-cfg=cfg(mln_platform_crate)");
    // docs.rs builds without network access, and documentation links nothing.
    if env::var_os("DOCS_RS").is_some() {
        return Ok(());
    }
    println!("cargo:rerun-if-env-changed={LOCAL_BUILD_ENV}");
    println!("cargo:rerun-if-env-changed={RELEASE_URL_ENV}");
    let target_os = env::var("CARGO_CFG_TARGET_OS")?;
    let target_env = env::var("CARGO_CFG_TARGET_ENV").unwrap_or_default();
    let out_path = PathBuf::from(env::var("OUT_DIR")?);
    // The crate root keeps the Cargo-built platform layer on the link line.
    if cargo_builds_platform(&target_os, &target_env) {
        println!("cargo:rustc-cfg=mln_platform_crate");
    }

    let target = env::var("TARGET")?;
    // Checked ahead of the local build so two enabled backend features fail
    // however the library arrives.
    let backend = selected_backend()?;
    let install_dir = match env::var_os(LOCAL_BUILD_ENV) {
        Some(local_build) => local_prefix(Path::new(&local_build), &target, backend)?,
        None => download::install_prefix(&target, backend)?,
    };
    let link_dir = native_library_dir(&install_dir);
    let descriptor = read_descriptor(&install_dir)?;

    if target_env == "musl" {
        let aliases = musl_aliases(&out_path)?;
        println!("cargo:rustc-link-search=native={}", aliases.display());
    }
    println!("cargo:rustc-link-search=native={}", link_dir.display());
    if target_os == "macos" {
        // ld64 resolves a library name to the shared library when both forms
        // sit in one directory, so this crate's own test binary needs that
        // directory on its run-time search path. The argument reaches only
        // this package's own targets, not the crates that depend on it.
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", link_dir.display());
    }
    print_rerun_if_changed(&link_dir);
    println!(
        "cargo:rerun-if-changed={}",
        install_dir.join(DESCRIPTOR_PATH).display()
    );

    println!(
        "cargo:rustc-link-lib=static={}",
        static_archive(&link_dir, &target_os, &target_env)?
    );
    // The `links` key passes these to the crates that depend on this one as
    // DEP_MAPLIBRE_NATIVE_C_PREFIX and DEP_MAPLIBRE_NATIVE_C_RENDER_BACKEND.
    println!("cargo:prefix={}", install_dir.display());
    println!("cargo:render-backend={}", descriptor.backend);
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

/// The one enabled backend feature, or none when the consumer takes the
/// platform default.
fn selected_backend() -> Result<Option<&'static str>, Box<dyn Error>> {
    let enabled: Vec<&'static str> = BACKENDS
        .into_iter()
        .filter(|backend| {
            env::var_os(format!("CARGO_FEATURE_{}", backend.to_uppercase())).is_some()
        })
        .collect();

    match enabled.as_slice() {
        [] => Ok(None),
        [backend] => Ok(Some(backend)),
        backends => Err(format!(
            "MapLibre Native builds one renderer per artifact, so only one maplibre-native-ffi-sys \
             backend feature can be enabled; got: {}",
            backends.join(", ")
        )
        .into()),
    }
}

/// Checks the install prefix of a local build, which serves one target with its
/// own backend unless a feature selects another.
fn local_prefix(
    prefix: &Path,
    target: &str,
    backend: Option<&str>,
) -> Result<PathBuf, Box<dyn Error>> {
    // A relative path would resolve against this crate's own directory, which
    // the consumer neither chooses nor sees.
    if !prefix.is_absolute() {
        return Err(format!(
            "{LOCAL_BUILD_ENV} must be an absolute path; found {}. To set a path relative to \
             the workspace, use `relative = true` in the [env] table of .cargo/config.toml.",
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
    let descriptor = read_descriptor(prefix)?;
    if descriptor.platform.rust_target != target
        || backend.is_some_and(|backend| backend != descriptor.backend)
    {
        return Err(format!(
            "{LOCAL_BUILD_ENV} names an install prefix built for {} with the {} backend, which \
             cannot serve {}",
            descriptor.platform.rust_target,
            descriptor.backend,
            request(target, backend)
        )
        .into());
    }
    Ok(prefix.to_path_buf())
}

/// Describes a request for `target` with the `backend` selected for it.
fn request(target: &str, backend: Option<&str>) -> String {
    match backend {
        Some(backend) => format!("{target} with the {backend} backend"),
        None => format!("{target} with its default backend"),
    }
}

/// The platform that an artifact is built for, as its descriptor and a
/// release's artifact index both describe it.
#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
struct Platform {
    id: String,
    rust_target: String,
    default_backend: String,
}

#[derive(Deserialize)]
struct ArtifactDescriptor {
    platform: Platform,
    backend: String,
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
/// process-wide unwinder that its C++ frames and Rust frames share.
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

/// Whether Cargo builds the Rust platform layer for the target, as the
/// manifest's target-specific dependency on `maplibre-native-ffi-platform`
/// names.
fn cargo_builds_platform(target_os: &str, target_env: &str) -> bool {
    match target_os {
        "linux" => target_env != "ohos",
        "freebsd" | "windows" | "emscripten" | "android" => true,
        _ => false,
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

/// Acquires a native install prefix from the pinned release.
mod download {
    use super::*;

    /// The pinned release's artifact index.
    #[derive(Deserialize)]
    struct Pin {
        tag: String,
        platforms: Vec<Platform>,
        artifacts: Vec<Artifact>,
    }

    /// An archive of the release, with its digest.
    #[derive(Deserialize)]
    struct Artifact {
        platform: String,
        backend: String,
        archive: String,
        sha256: String,
    }

    /// Downloads and unpacks the pinned release's archive for `target` and
    /// `backend`, or the target's default backend, unless an earlier build
    /// already did.
    pub(super) fn install_prefix(
        target: &str,
        backend: Option<&str>,
    ) -> Result<PathBuf, Box<dyn Error>> {
        let pin_path =
            Path::new(&env::var_os("CARGO_MANIFEST_DIR").ok_or("CARGO_MANIFEST_DIR is unset")?)
                .join(PIN_PATH);
        if !pin_path.is_file() {
            return Err(format!(
                "Only a published maplibre-native-ffi-sys crate downloads the native library. \
                 Name a local build in {LOCAL_BUILD_ENV}."
            )
            .into());
        }
        println!("cargo:rerun-if-changed={}", pin_path.display());
        let pin: Pin = serde_json::from_str(&fs::read_to_string(&pin_path)?)
            .map_err(|error| format!("invalid {}: {error}", pin_path.display()))?;
        let platform = pin
            .platforms
            .iter()
            .find(|platform| platform.rust_target == target);
        let artifact = platform
            .and_then(|platform| {
                let backend = backend.unwrap_or(&platform.default_backend);
                pin.artifacts.iter().find(|artifact| {
                    artifact.platform == platform.id && artifact.backend == backend
                })
            })
            .ok_or_else(|| {
                format!(
                    "{} publishes no archive for {}",
                    pin.tag,
                    request(target, backend)
                )
            })?;
        let archive_name = &artifact.archive;
        let digest = &artifact.sha256;

        // The digest names the unpacked prefix, and an unpacked prefix is
        // complete because it is renamed into place.
        let cache_dir = cache_dir()?;
        let prefix = cache_dir.join(digest);
        if prefix.is_dir() {
            return Ok(prefix);
        }

        let release_url =
            env::var(RELEASE_URL_ENV).unwrap_or_else(|_| DEFAULT_RELEASE_URL.to_owned());
        let url = format!(
            "{}/{}/{archive_name}",
            release_url.trim_end_matches('/'),
            pin.tag
        );
        println!("cargo:warning=downloading {url}");
        let mut archive = Vec::new();
        agent()
            .get(&url)
            .call()
            .map_err(|error| format!("failed to download {url}: {error}"))?
            .into_body()
            .into_reader()
            .read_to_end(&mut archive)
            .map_err(|error| format!("failed to download {url}: {error}"))?;
        if hex(&Sha256::digest(&archive)) != *digest {
            return Err(format!(
                "{archive_name} does not match its digest pinned for {}",
                pin.tag
            )
            .into());
        }

        let scratch = cache_dir.join(format!("{digest}.{}", std::process::id()));
        let _ = fs::remove_dir_all(&scratch);
        let result = tar::Archive::new(GzDecoder::new(archive.as_slice()))
            .unpack(&scratch)
            .map_err(|error| format!("failed to unpack {archive_name}: {error}").into())
            .and_then(|()| {
                // The archive holds a single root named after the archive.
                // Losing the rename means another build unpacked the same
                // digest first, which leaves the same tree in place.
                let root = archive_name.strip_suffix(".tar.gz").unwrap_or(archive_name);
                match fs::rename(scratch.join(root), &prefix) {
                    Err(error) if !prefix.is_dir() => {
                        Err(format!("failed to install {}: {error}", prefix.display()).into())
                    }
                    _ => Ok(()),
                }
            });
        let _ = fs::remove_dir_all(&scratch);
        result.map(|()| prefix)
    }

    /// Bounds every request, because ureq leaves both timeouts unset and a host
    /// that stops answering would hang the build. The global bound covers the
    /// body, so it allows for a slow link pulling a 30 MB archive.
    fn agent() -> ureq::Agent {
        ureq::Agent::config_builder()
            .timeout_connect(Some(Duration::from_secs(30)))
            .timeout_global(Some(Duration::from_secs(600)))
            .build()
            .into()
    }

    fn cache_dir() -> Result<PathBuf, Box<dyn Error>> {
        // Resolves the host's cache directory even when cross-compiling.
        // Archives live outside the target directory so `cargo clean` does not
        // force a re-download.
        let base = if cfg!(windows) {
            env::var_os("LOCALAPPDATA").map(PathBuf::from)
        } else if cfg!(target_os = "macos") {
            env::var_os("HOME").map(|home| PathBuf::from(home).join("Library/Caches"))
        } else {
            env::var_os("XDG_CACHE_HOME")
                .map(PathBuf::from)
                .or_else(|| env::var_os("HOME").map(|home| PathBuf::from(home).join(".cache")))
        };
        let base = base.ok_or("could not resolve a cache directory for the native library")?;
        let cache_dir = base.join("maplibre-native-ffi/native");
        fs::create_dir_all(&cache_dir)?;
        Ok(cache_dir)
    }

    fn hex(bytes: &[u8]) -> String {
        bytes.iter().map(|byte| format!("{byte:02x}")).collect()
    }
}
