use std::env;
use std::error::Error;
use std::fs;
use std::io;
use std::io::Read;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::Duration;

use flate2::read::GzDecoder;
use serde::Deserialize;
use sha2::{Digest, Sha256};

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
    // Windows has no rpath, so a dependent has to place the DLL itself. The
    // `links` key turns this into DEP_MAPLIBRE_NATIVE_C_RUNTIME_DIR for the
    // crates that need it.
    println!("cargo:runtime-dir={}", runtime_dir.display());
    println!("cargo:rerun-if-env-changed=LIBCLANG_PATH");
    println!("cargo:rerun-if-env-changed=BINDGEN_EXTRA_CLANG_ARGS");
    println!("cargo:rerun-if-env-changed=SDKROOT");
    print_rerun_if_changed(&include_dir);

    let mut builder = bindgen::Builder::default()
        .header(header.display().to_string())
        .clang_arg("-xc")
        .clang_arg("-std=c23")
        .clang_arg(format!("-I{}", include_dir.display()));

    // bindgen parses through libclang, which has none of the clang driver's
    // logic for locating an Apple SDK, and these headers include the SDK's
    // stdint.h. Ask the toolchain where that SDK is. An SDKROOT the caller set
    // already reaches libclang, so this fills in only when one is absent.
    if env::var_os("SDKROOT").is_none() {
        let target = env::var("TARGET")?;
        if let Some(sdk_path) = apple_sdk_name(&target_os, &target).and_then(apple_sdk_path) {
            builder = builder.clang_arg(format!("-isysroot{sdk_path}"));
        }
    }

    let bindings = builder
        .allowlist_function("^mln_.*")
        .allowlist_type("^mln_.*")
        .allowlist_var("^MLN_.*")
        // Every C handle is the same uint64_t, so a plain type alias would let
        // a map be passed where a runtime is expected. A transparent newtype
        // per handle keeps the distinction the opaque struct pointers used to
        // give us, at no ABI cost.
        .new_type_alias(concat!(
            "^mln_(runtime|map|map_projection|render_session|wake_source",
            "|resource_request_handle|offline_region_snapshot",
            "|offline_region_list|json_snapshot|style_id_list",
            "|style_string_list",
            "|feature_query_result|feature_extension_result)$"
        ))
        .prepend_enum_name(false)
        .layout_tests(true)
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()?;

    let out_path = PathBuf::from(env::var("OUT_DIR")?);
    bindings.write_to_file(out_path.join("bindings.rs"))?;

    Ok(())
}

/// Locates the native install prefix, downloading a published snapshot archive
/// when no local build was pointed at.
fn native_install_dir() -> Result<PathBuf, Box<dyn Error>> {
    // Ahead of the local prefix, not just the download: two backend features
    // name mutually exclusive artifacts however the library arrives, and a
    // prefix built for one of them would silently satisfy both requesters.
    let backend = download::selected_backend()?;
    if let Some(install_dir) = env::var_os("MAPLIBRE_NATIVE_C_INSTALL_DIR") {
        return Ok(PathBuf::from(install_dir));
    }
    download::install_prefix(backend)
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

/// The SDK that xcrun knows this target by, or None for a target that needs no
/// Apple SDK.
fn apple_sdk_name(target_os: &str, target: &str) -> Option<&'static str> {
    let simulator = target.ends_with("-sim");
    match target_os {
        "macos" => Some("macosx"),
        "ios" if simulator => Some("iphonesimulator"),
        "ios" => Some("iphoneos"),
        _ => None,
    }
}

/// Where the installed Xcode command line tools keep that SDK. Reports None on
/// a host without them, leaving libclang to its own header search.
fn apple_sdk_path(sdk_name: &str) -> Option<String> {
    let output = Command::new("xcrun")
        .args(["--sdk", sdk_name, "--show-sdk-path"])
        .output()
        .ok()?;
    if !output.status.success() {
        return None;
    }
    let path = String::from_utf8(output.stdout).ok()?;
    let path = path.trim().to_owned();
    if path.is_empty() { None } else { Some(path) }
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

/// Acquires a native install prefix from the published snapshot release.
///
/// The snapshot release is floating: its asset URLs never change, so a cache
/// keyed on the URL would serve stale bytes forever. `SHA256SUMS` changing is
/// the exact signal that the artifacts moved, so its digest is the cache key.
///
/// Documented in `docs/src/content/docs/development/binding-specification.md`
/// under "Native Artifact Acquisition"; the Dart build hook implements the same
/// contract.
mod download {
    use super::*;

    use std::collections::{BTreeMap, BTreeSet};

    const RELEASE_BASE_URL: &str =
        "https://github.com/maplibre/maplibre-native-ffi/releases/download";
    const SNAPSHOT_TAG: &str = "unstable-native-snapshot";
    const BACKENDS: [&str; 4] = ["metal", "opengl", "vulkan", "webgpu"];

    /// A published `<os>-<arch>-<backend>` artifact.
    struct Preset {
        /// Full preset name, as used in the asset file name.
        name: String,
        /// Platform half of the preset, matching the descriptor's
        /// `targetPlatform`.
        platform: String,
        /// Render backend selector, matching the descriptor's `renderBackend`.
        /// Spelled `opengl` even where the preset suffix names the context
        /// provider instead.
        backend: &'static str,
    }

    /// One published platform and the backends built for it.
    struct PlatformTarget {
        os: &'static str,
        arch: &'static str,
        /// `CARGO_CFG_TARGET_ENV`, which is what separates targets that share
        /// an OS: OpenHarmony and musl both report `linux`, and the iOS
        /// simulator reports `ios`.
        env: &'static str,
        platform: &'static str,
        default_backend: &'static str,
        /// Backend selector paired with the preset suffix it maps to. Preset
        /// names spell the OpenGL backend by its context provider.
        backends: &'static [(&'static str, &'static str)],
    }

    const OPENGL_EGL: &[(&str, &str)] = &[("vulkan", "vulkan"), ("opengl", "egl")];
    const OPENGL_WGL: &[(&str, &str)] = &[("vulkan", "vulkan"), ("opengl", "wgl")];
    const APPLE_DESKTOP: &[(&str, &str)] =
        &[("metal", "metal"), ("vulkan", "vulkan"), ("opengl", "egl")];
    const APPLE_MOBILE: &[(&str, &str)] = &[("metal", "metal")];

    /// The presets `.github/workflows/snapshots.yml` publishes a shared library
    /// for. The OpenHarmony presets build from source and ship no archive, so
    /// they are absent here; so are musl and windows-gnu, which no preset
    /// targets. Device iOS is absent too: `ios-arm64-metal` ships only a static
    /// archive, which needs Apple framework link metadata this does not emit.
    const PLATFORM_TARGETS: &[PlatformTarget] = &[
        PlatformTarget {
            os: "linux",
            arch: "x86_64",
            env: "gnu",
            platform: "linux-x64",
            default_backend: "vulkan",
            backends: OPENGL_EGL,
        },
        PlatformTarget {
            os: "linux",
            arch: "aarch64",
            env: "gnu",
            platform: "linux-arm64",
            default_backend: "vulkan",
            backends: OPENGL_EGL,
        },
        PlatformTarget {
            os: "macos",
            arch: "aarch64",
            env: "",
            platform: "macos-arm64",
            default_backend: "metal",
            backends: APPLE_DESKTOP,
        },
        PlatformTarget {
            os: "windows",
            arch: "x86_64",
            env: "msvc",
            platform: "windows-x64",
            default_backend: "vulkan",
            backends: OPENGL_WGL,
        },
        PlatformTarget {
            os: "windows",
            arch: "aarch64",
            env: "msvc",
            platform: "windows-arm64",
            default_backend: "vulkan",
            backends: OPENGL_WGL,
        },
        PlatformTarget {
            os: "android",
            arch: "aarch64",
            env: "",
            platform: "android-arm64",
            default_backend: "opengl",
            backends: OPENGL_EGL,
        },
        PlatformTarget {
            os: "android",
            arch: "x86_64",
            env: "",
            platform: "android-x64",
            default_backend: "opengl",
            backends: OPENGL_EGL,
        },
        PlatformTarget {
            os: "ios",
            arch: "aarch64",
            env: "sim",
            platform: "ios-simulator-arm64",
            default_backend: "metal",
            backends: APPLE_MOBILE,
        },
    ];

    #[derive(Deserialize)]
    #[serde(rename_all = "camelCase")]
    struct ArtifactDescriptor {
        /// Absent from archives published before the field was added.
        #[serde(default)]
        git_sha: Option<String>,
        render_backend: String,
        target_platform: String,
    }

    pub(super) fn install_prefix(backend: Option<&'static str>) -> Result<PathBuf, Box<dyn Error>> {
        let preset = resolve_preset(backend)?;
        let cache_dir = cache_dir()?.join(&preset.name);

        // Only an unreachable release falls back to the cache. A checksum or
        // extraction failure is a real defect and stays fatal, so a corrupt
        // download can never be papered over with a stale artifact.
        //
        // A publish replaces `SHA256SUMS` and the archives as separate assets,
        // so a build starting mid-publish can pair one generation's checksum
        // with the other's bytes. That reads as a mismatch without either file
        // being corrupt, so the whole acquisition is retried once against a
        // freshly fetched checksum file; a mismatch that survives is fatal.
        //
        // The retry gives up every cached answer with it — the offline fallback
        // and the cache hit alike. Once bytes have failed verification the
        // release is no longer trustworthy for this build, and answering that
        // from disk, whether on a later timeout or on a digest already present,
        // would hide an unverified download behind an older artifact.
        let mut after_mismatch = false;
        let prefix = loop {
            match fetch_checksums()
                .and_then(|checksums| acquire(&preset, &cache_dir, &checksums, after_mismatch))
            {
                Ok(prefix) => break prefix,
                Err(Unreachable(error)) if after_mismatch => return Err(error),
                Err(Unreachable(error)) => {
                    break reuse_cached(&preset, &cache_dir, &error)?;
                }
                Err(Mismatch(error)) if !after_mismatch => {
                    after_mismatch = true;
                    println!(
                        "cargo:warning={error}; the {SNAPSHOT_TAG} release may have been \
                         republished mid-download. Retrying once."
                    );
                }
                Err(Mismatch(error)) | Err(Fatal(error)) => return Err(error),
            }
        };

        let descriptor = read_descriptor(&prefix)?;
        verify_descriptor(&preset, &descriptor, &prefix)?;
        warn_on_header_skew(&prefix, &descriptor);
        Ok(prefix)
    }

    /// Separates a release we could not reach from a defect in what it served.
    /// A publish race can list an archive in `SHA256SUMS` before uploading it,
    /// which should fall back to the cache; a checksum mismatch should not.
    enum Failure {
        Unreachable(Box<dyn Error>),
        /// A checksum that did not match its archive, which a publish crossing
        /// generations produces without either file being corrupt.
        Mismatch(Box<dyn Error>),
        Fatal(Box<dyn Error>),
    }
    use Failure::{Fatal, Mismatch, Unreachable};

    impl From<io::Error> for Failure {
        fn from(error: io::Error) -> Self {
            Fatal(error.into())
        }
    }

    /// Downloads and extracts the archive unless the digest is already cached.
    ///
    /// `after_mismatch` suppresses the cache hit. A checksum file that crossed
    /// back to an older generation resolves to a digest already on disk, which
    /// would answer a failed verification from the cache without proving any
    /// fresh bytes.
    fn acquire(
        preset: &Preset,
        cache_dir: &Path,
        checksums: &str,
        after_mismatch: bool,
    ) -> Result<PathBuf, Failure> {
        let prefix = cache_dir.join(hex(&Sha256::digest(checksums.as_bytes())));
        if prefix.is_dir() && !after_mismatch {
            return Ok(prefix);
        }

        let archive_name = format!("{LIBRARY_NAME}-{}.tar.gz", preset.name);
        let expected = checksum_for(checksums, &archive_name).map_err(Fatal)?;
        let url = format!("{RELEASE_BASE_URL}/{SNAPSHOT_TAG}/{archive_name}");
        println!("cargo:warning=downloading {url}");
        let response = agent()
            .get(&url)
            .call()
            .map_err(|error| Unreachable(error.into()))?;

        // The archive holds a single `maplibre-native-c-<preset>/` root, so
        // extracting into a scratch directory yields the prefix as a child.
        let scratch = cache_dir.join(format!(".extract-{}", std::process::id()));
        let _ = fs::remove_dir_all(&scratch);
        fs::create_dir_all(&scratch)?;
        let result = extract(response, &scratch, &expected, &archive_name);
        if let Err(failure) = result {
            let _ = fs::remove_dir_all(&scratch);
            return Err(failure);
        }

        // Losing the rename means another build extracted the same digest
        // first, which is the same tree by construction.
        let extracted = scratch.join(format!("{LIBRARY_NAME}-{}", preset.name));
        let renamed = fs::rename(&extracted, &prefix);
        let _ = fs::remove_dir_all(&scratch);
        if let Err(error) = renamed
            && !prefix.is_dir()
        {
            return Err(Fatal(
                format!("failed to install {}: {error}", prefix.display()).into(),
            ));
        }
        Ok(prefix)
    }

    /// Streams the archive through the digest and the decoder at once, so a
    /// 25 MB download never has to be held in memory. The digest is checked
    /// before the extracted tree is moved into place, so a corrupt download
    /// cannot leave a usable cache entry behind.
    fn extract(
        response: ureq::http::Response<ureq::Body>,
        scratch: &Path,
        expected: &str,
        archive_name: &str,
    ) -> Result<(), Failure> {
        let reader = response.into_body().into_reader();
        let mut digest = DigestReader {
            inner: reader,
            hasher: Sha256::new(),
        };
        tar::Archive::new(GzDecoder::new(&mut digest)).unpack(scratch)?;
        // `unpack` stops at the end of the tar stream, which can leave trailing
        // padding unread and its bytes out of the digest.
        io::copy(&mut digest, &mut io::sink())?;

        let actual = hex(&digest.hasher.finalize());
        if actual != expected {
            return Err(Mismatch(
                format!("checksum mismatch for {archive_name}: expected {expected}, got {actual}")
                    .into(),
            ));
        }
        Ok(())
    }

    /// Falls back to a previously downloaded prefix when the release is
    /// unreachable, so offline builds keep working.
    fn reuse_cached(
        preset: &Preset,
        cache_dir: &Path,
        error: &dyn std::fmt::Display,
    ) -> Result<PathBuf, Box<dyn Error>> {
        let newest = fs::read_dir(cache_dir)
            .into_iter()
            .flatten()
            .flatten()
            .map(|entry| entry.path())
            .filter(|path| path.join("include").is_dir())
            .max_by_key(|path| {
                fs::metadata(path)
                    .and_then(|metadata| metadata.modified())
                    .ok()
            });

        match newest {
            Some(prefix) => {
                println!(
                    "cargo:warning=could not reach the native snapshot release ({error}); \
                     reusing the cached {} artifact, which may be out of date",
                    preset.name
                );
                Ok(prefix)
            }
            None => Err(format!(
                "could not reach the native snapshot release ({error}) and no cached {} \
                 artifact is available; set MAPLIBRE_NATIVE_C_INSTALL_DIR to a local install \
                 prefix to build without network access",
                preset.name
            )
            .into()),
        }
    }

    fn fetch_checksums() -> Result<String, Failure> {
        let url = format!("{RELEASE_BASE_URL}/{SNAPSHOT_TAG}/SHA256SUMS");
        let mut response = agent()
            .get(&url)
            .call()
            .map_err(|error| Unreachable(error.into()))?;
        response
            .body_mut()
            .read_to_string()
            .map_err(|error| Unreachable(error.into()))
    }

    /// Bounds every request, because ureq leaves both timeouts unset by
    /// default. A host that accepts the connection and then stops answering
    /// would otherwise hang the build instead of reaching the cache fallback.
    /// The global bound covers the body too, so it allows for a slow link
    /// pulling a 30 MB archive.
    fn agent() -> ureq::Agent {
        ureq::Agent::config_builder()
            .timeout_connect(Some(Duration::from_secs(30)))
            .timeout_global(Some(Duration::from_secs(600)))
            .build()
            .into()
    }

    fn checksum_for(checksums: &str, archive_name: &str) -> Result<String, Box<dyn Error>> {
        checksums
            .lines()
            .filter_map(|line| line.split_once("  "))
            .find(|(_, name)| *name == archive_name)
            .map(|(checksum, _)| checksum.to_owned())
            .ok_or_else(|| {
                format!("the native snapshot release has no {archive_name} entry").into()
            })
    }

    fn resolve_preset(backend: Option<&'static str>) -> Result<Preset, Box<dyn Error>> {
        let os = env::var("CARGO_CFG_TARGET_OS")?;
        let arch = env::var("CARGO_CFG_TARGET_ARCH")?;
        // Cargo omits the variable entirely when the cfg value is empty.
        let target_env = env::var("CARGO_CFG_TARGET_ENV").unwrap_or_default();
        let triple = env::var("TARGET")?;

        let target = PLATFORM_TARGETS
            .iter()
            .find(|target| target.os == os && target.arch == arch && target.env == target_env)
            .ok_or_else(|| {
                format!(
                    "no native artifact is published for {triple}; build the native library from \
                     source and point MAPLIBRE_NATIVE_C_INSTALL_DIR at its install prefix"
                )
            })?;

        let backend = backend.unwrap_or(target.default_backend);
        let suffix = target
            .backends
            .iter()
            .find(|(name, _)| *name == backend)
            .map(|(_, suffix)| *suffix)
            .ok_or_else(|| {
                let available: Vec<&str> = target.backends.iter().map(|(name, _)| *name).collect();
                format!(
                    "the {backend} backend is not built for {}; enable one of: {}",
                    target.platform,
                    available.join(", ")
                )
            })?;

        Ok(Preset {
            name: format!("{}-{suffix}", target.platform),
            platform: target.platform.to_owned(),
            backend,
        })
    }

    /// The one enabled backend feature, or none when the caller takes the
    /// platform default.
    pub(super) fn selected_backend() -> Result<Option<&'static str>, Box<dyn Error>> {
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

    fn read_descriptor(prefix: &Path) -> Result<ArtifactDescriptor, Box<dyn Error>> {
        let path = prefix.join("share/maplibre-native-c/artifact.json");
        let contents = fs::read_to_string(&path)
            .map_err(|error| format!("failed to read {}: {error}", path.display()))?;
        serde_json::from_str(&contents).map_err(|error| {
            format!("invalid artifact descriptor {}: {error}", path.display()).into()
        })
    }

    fn verify_descriptor(
        preset: &Preset,
        descriptor: &ArtifactDescriptor,
        prefix: &Path,
    ) -> Result<(), Box<dyn Error>> {
        if descriptor.target_platform != preset.platform
            || descriptor.render_backend != preset.backend
        {
            return Err(format!(
                "{} holds a {}/{} artifact but {} was requested; remove that directory and rebuild",
                prefix.display(),
                descriptor.target_platform,
                descriptor.render_backend,
                preset.name
            )
            .into());
        }
        Ok(())
    }

    /// Compares the checkout's public headers against the downloaded prefix's.
    ///
    /// A git dependency pinned at one commit gets whatever the floating release
    /// currently holds, which is built from another. Comparing commits would
    /// warn constantly, because the publish is gated on input digests and the
    /// artifact's commit lags by design. Differing headers are the condition
    /// that actually matters.
    fn warn_on_header_skew(prefix: &Path, descriptor: &ArtifactDescriptor) {
        let Some(repo_root) = repo_root() else {
            return;
        };
        let Some(checkout) = public_headers(&repo_root.join("include")) else {
            return;
        };
        let Some(artifact) = public_headers(&prefix.join("include")) else {
            return;
        };
        if checkout == artifact {
            return;
        }

        let differing: Vec<&str> = checkout
            .keys()
            .chain(artifact.keys())
            .filter(|name| checkout.get(*name) != artifact.get(*name))
            .map(String::as_str)
            .collect::<BTreeSet<_>>()
            .into_iter()
            .collect();

        let built_from = descriptor
            .git_sha
            .as_deref()
            .map(|sha| format!(" (built from {sha})"))
            .unwrap_or_default();
        println!(
            "cargo:warning=the downloaded native artifact{built_from} does not match this \
             checkout's C headers: {}. The snapshot release publishes on its own schedule, so it \
             can lag this commit; build the native library from source if the difference matters.",
            differing.join(", ")
        );
    }

    /// Digests the public C headers, keyed by their path under `include/`.
    /// Render backend dependencies install their own headers alongside ours, so
    /// this covers only the umbrella header and its domain directory.
    fn public_headers(include_dir: &Path) -> Option<BTreeMap<String, String>> {
        let umbrella = include_dir.join("maplibre_native_c.h");
        if !umbrella.is_file() {
            return None;
        }
        let mut headers = BTreeMap::new();
        headers.insert(
            "maplibre_native_c.h".to_owned(),
            hex(&Sha256::digest(fs::read(&umbrella).ok()?)),
        );
        collect_headers(
            &include_dir.join("maplibre_native_c"),
            "maplibre_native_c",
            &mut headers,
        );
        Some(headers)
    }

    fn collect_headers(dir: &Path, prefix: &str, headers: &mut BTreeMap<String, String>) {
        let Ok(entries) = fs::read_dir(dir) else {
            return;
        };
        for entry in entries.flatten() {
            let path = entry.path();
            let Some(name) = path.file_name().and_then(|name| name.to_str()) else {
                continue;
            };
            let key = format!("{prefix}/{name}");
            if path.is_dir() {
                collect_headers(&path, &key, headers);
            } else if path.extension().is_some_and(|extension| extension == "h")
                && let Ok(contents) = fs::read(&path)
            {
                headers.insert(key, hex(&Sha256::digest(contents)));
            }
        }
    }

    /// The crate sits at `bindings/rust/crates/maplibre-native-ffi-sys`, and both
    /// git and path dependencies carry the whole checkout.
    fn repo_root() -> Option<PathBuf> {
        let manifest_dir = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR")?);
        let root = manifest_dir.ancestors().nth(4)?.to_path_buf();
        root.join("include/maplibre_native_c.h")
            .is_file()
            .then_some(root)
    }

    fn cache_dir() -> Result<PathBuf, Box<dyn Error>> {
        // Build scripts run on the host, so this resolves the host's cache
        // directory even when cross-compiling. Keeping the archives outside the
        // target directory means `cargo clean` does not force a re-download.
        let base = if cfg!(windows) {
            env::var_os("LOCALAPPDATA").map(PathBuf::from)
        } else if cfg!(target_os = "macos") {
            env::var_os("HOME").map(|home| PathBuf::from(home).join("Library/Caches"))
        } else {
            env::var_os("XDG_CACHE_HOME")
                .map(PathBuf::from)
                .or_else(|| env::var_os("HOME").map(|home| PathBuf::from(home).join(".cache")))
        };
        let base = base.ok_or("could not resolve a cache directory for the native artifact")?;
        let cache_dir = base.join("maplibre-native-ffi/native");
        fs::create_dir_all(&cache_dir)?;
        Ok(cache_dir)
    }

    fn hex(bytes: &[u8]) -> String {
        bytes.iter().map(|byte| format!("{byte:02x}")).collect()
    }

    /// Digests bytes as they are read, so the download feeds the checksum and
    /// the decompressor in one pass.
    struct DigestReader<R> {
        inner: R,
        hasher: Sha256,
    }

    impl<R: Read> Read for DigestReader<R> {
        fn read(&mut self, buffer: &mut [u8]) -> io::Result<usize> {
            let read = self.inner.read(buffer)?;
            self.hasher.update(&buffer[..read]);
            Ok(read)
        }
    }
}
