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

/// The release that a published crate downloads, with the SHA-256 digest of
/// each archive in it. The release workflow writes this file, and git ignores
/// it.
const PIN_PATH: &str = "native_release.json";

fn main() -> Result<(), Box<dyn Error>> {
    // docs.rs builds without network access, and documentation links nothing.
    if env::var_os("DOCS_RS").is_some() {
        return Ok(());
    }
    println!("cargo:rerun-if-env-changed={LOCAL_BUILD_ENV}");
    println!("cargo:rerun-if-env-changed={RELEASE_URL_ENV}");
    let target_os = env::var("CARGO_CFG_TARGET_OS")?;
    let target_env = env::var("CARGO_CFG_TARGET_ENV").unwrap_or_default();
    let out_path = PathBuf::from(env::var("OUT_DIR")?);

    let platform = target_platform(&target_os, &target_env)?;
    // Checked ahead of the local build so two enabled backend features fail
    // however the library arrives.
    let backend = selected_backend()?;
    let preset = format!(
        "{platform}-{}",
        backend_suffix(
            backend.unwrap_or(default_backend(&target_os, &target_env)),
            &target_os
        )
    );
    let install_dir = match env::var_os(LOCAL_BUILD_ENV) {
        Some(local_build) => local_prefix(Path::new(&local_build), &platform, backend, &preset)?,
        None => download::install_prefix(&preset, &target_os)?,
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

    for archive in static_archives(&link_dir, &target_os)? {
        println!("cargo:rustc-link-lib=static={archive}");
    }
    // The `links` key passes these to the crates that depend on this one as
    // DEP_MAPLIBRE_NATIVE_C_PREFIX and DEP_MAPLIBRE_NATIVE_C_RENDER_BACKEND.
    println!("cargo:prefix={}", install_dir.display());
    println!("cargo:render-backend={}", descriptor.render_backend);
    // The archive merges in every dependency it can. What is left is the
    // platform and render backend's system libraries, which CMake records.
    for library in &descriptor.static_link_libraries {
        // Rust's self-contained musl sysroot includes these interfaces in libc
        // and provides no separate alias archives for them.
        if target_env == "musl" && matches!(library.as_str(), "m" | "dl" | "pthread" | "rt") {
            continue;
        }
        println!("cargo:rustc-link-lib={library}");
    }
    for framework in &descriptor.static_link_frameworks {
        println!("cargo:rustc-link-lib=framework={framework}");
    }
    Ok(())
}

/// Names the platform half of the preset serving the target, as the artifact
/// descriptor's `targetPlatform` spells it.
fn target_platform(os: &str, target_env: &str) -> Result<String, Box<dyn Error>> {
    let arch = match env::var("CARGO_CFG_TARGET_ARCH")?.as_str() {
        "x86_64" => "x64".to_owned(),
        "aarch64" => "arm64".to_owned(),
        other => other.to_owned(),
    };
    // The environment separates targets that share an OS: OpenHarmony and musl
    // both report `linux`, and the simulators and Mac Catalyst report their
    // device OS.
    Ok(match (os, target_env) {
        ("linux", "gnu" | "musl") => format!("linux-{target_env}-{arch}"),
        ("linux", "ohos") => format!("ohos-{arch}"),
        ("ios" | "tvos" | "visionos", "sim") => format!("{os}-simulator-{arch}"),
        ("ios", "macabi") => format!("ios-maccatalyst-{arch}"),
        ("windows", "msvc")
        | ("macos" | "ios" | "tvos" | "visionos" | "android" | "emscripten", _) => {
            format!("{os}-{arch}")
        }
        _ => {
            return Err(format!(
                "MapLibre Native builds for no platform matching {}",
                env::var("TARGET")?
            )
            .into());
        }
    })
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

/// Metal on Apple platforms, OpenGL on Android and OpenHarmony, WebGPU in the
/// browser, and Vulkan elsewhere.
fn default_backend(os: &str, target_env: &str) -> &'static str {
    match (os, target_env) {
        ("macos" | "ios" | "tvos" | "visionos", _) => "metal",
        ("android", _) | ("linux", "ohos") => "opengl",
        ("emscripten", _) => "webgpu",
        _ => "vulkan",
    }
}

/// The backend half of a preset name, which spells OpenGL by its context
/// provider.
fn backend_suffix<'a>(backend: &'a str, os: &str) -> &'a str {
    match (backend, os) {
        ("opengl", "windows") => "wgl",
        ("opengl", "emscripten") => "webgl",
        ("opengl", _) => "egl",
        _ => backend,
    }
}

/// Resolves a local build, which is either an install prefix or a CMake build
/// root holding an install for each preset.
fn local_prefix(
    directory: &Path,
    platform: &str,
    backend: Option<&str>,
    preset: &str,
) -> Result<PathBuf, Box<dyn Error>> {
    // A relative path would resolve against this crate's own directory, which
    // the consumer neither chooses nor sees.
    if !directory.is_absolute() {
        return Err(format!(
            "{LOCAL_BUILD_ENV} must be an absolute path; found {}. To set a path relative to \
             the workspace, use `relative = true` in the [env] table of .cargo/config.toml.",
            directory.display()
        )
        .into());
    }
    if directory.join(DESCRIPTOR_PATH).is_file() {
        // A prefix serves one platform, so a build for several targets names a
        // build root instead. Only a selected backend can conflict; without
        // one, the prefix's own backend serves.
        let descriptor = read_descriptor(directory)?;
        if descriptor.target_platform != platform
            || backend.is_some_and(|backend| backend != descriptor.render_backend)
        {
            return Err(format!(
                "{LOCAL_BUILD_ENV} names an install prefix built for {} {}, which cannot serve \
                 {platform}{}",
                descriptor.target_platform,
                descriptor.render_backend,
                backend
                    .map(|backend| format!(" {backend}"))
                    .unwrap_or_default()
            )
            .into());
        }
        return Ok(directory.to_path_buf());
    }
    let prefix = directory.join(preset).join("install");
    // Registered before the existence check so that building the preset later
    // reruns this script.
    println!(
        "cargo:rerun-if-changed={}",
        prefix.join(DESCRIPTOR_PATH).display()
    );
    if !prefix.join(DESCRIPTOR_PATH).is_file() {
        return Err(format!(
            "{LOCAL_BUILD_ENV} names {}, which is neither an install prefix nor a build root \
             holding {preset}",
            directory.display()
        )
        .into());
    }
    Ok(prefix)
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
struct ArtifactDescriptor {
    render_backend: String,
    target_platform: String,
    static_link_libraries: Vec<String>,
    static_link_frameworks: Vec<String>,
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

/// The complete static archive, then whatever ships beside it, such as the Rust
/// platform layer the C archive calls into.
///
/// Every format but COFF lets the file extension separate the archive from the
/// shared library. COFF cannot, because the shared build's import library
/// already holds that name, so there the archive carries a `-static` suffix and
/// the import library is the one to leave out.
fn static_archives(link_dir: &Path, target_os: &str) -> Result<Vec<String>, Box<dyn Error>> {
    let windows = target_os == "windows";
    let (extension, prefix) = if windows { ("lib", "") } else { ("a", "lib") };
    let primary = if windows {
        format!("{LIBRARY_NAME}-static")
    } else {
        LIBRARY_NAME.to_owned()
    };

    let mut companions: Vec<String> = fs::read_dir(link_dir)
        .map_err(|error| format!("failed to read {}: {error}", link_dir.display()))?
        .flatten()
        .filter_map(|entry| {
            let path = entry.path();
            if path.extension()? != extension {
                return None;
            }
            let name = path.file_stem()?.to_str()?.strip_prefix(prefix)?.to_owned();
            // On COFF this is the shared library's import library, which would
            // put the consumer back on the DLL.
            (name != primary && !(windows && name == LIBRARY_NAME)).then_some(name)
        })
        .collect();
    companions.sort();

    if !link_dir
        .join(format!("{prefix}{primary}.{extension}"))
        .is_file()
    {
        return Err(format!(
            "missing the complete static archive {prefix}{primary}.{extension} in {}",
            link_dir.display()
        )
        .into());
    }

    // The archive references the companions, so it comes first. Emscripten
    // takes the platform layer from Cargo, so linking a copy here would collide.
    let mut archives = vec![primary];
    if target_os != "emscripten" {
        archives.extend(companions);
    }
    Ok(archives)
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

    use std::collections::BTreeMap;

    #[derive(Deserialize)]
    struct Pin {
        tag: String,
        sha256: BTreeMap<String, String>,
    }

    /// Downloads and unpacks `preset` from the pinned release, unless an
    /// earlier build already did.
    pub(super) fn install_prefix(preset: &str, target_os: &str) -> Result<PathBuf, Box<dyn Error>> {
        let pin_path =
            Path::new(&env::var_os("CARGO_MANIFEST_DIR").ok_or("CARGO_MANIFEST_DIR is unset")?)
                .join(PIN_PATH);
        println!("cargo:rerun-if-changed={}", pin_path.display());
        if !pin_path.is_file() {
            return Err(format!(
                "Only a published maplibre-native-ffi-sys crate downloads the native library. \
                 Name a local build in {LOCAL_BUILD_ENV}."
            )
            .into());
        }
        // The browser links the Rust platform layer from Cargo, and that crate
        // is not published.
        if target_os == "emscripten" {
            return Err(format!(
                "The published crates do not support Emscripten. Depend on a checkout of \
                 maplibre-native-ffi and name a local build in {LOCAL_BUILD_ENV}."
            )
            .into());
        }
        let pin: Pin = serde_json::from_str(&fs::read_to_string(&pin_path)?)
            .map_err(|error| format!("invalid {}: {error}", pin_path.display()))?;
        let archive_name = format!("{LIBRARY_NAME}-{preset}.tar.gz");
        let digest = pin
            .sha256
            .get(&archive_name)
            .ok_or_else(|| format!("{} publishes no {archive_name}", pin.tag))?;

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
                // The archive holds a single `maplibre-native-c-<preset>/`
                // root. Losing the rename means another build unpacked the same
                // digest first, which leaves the same tree in place.
                match fs::rename(scratch.join(format!("{LIBRARY_NAME}-{preset}")), &prefix) {
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
