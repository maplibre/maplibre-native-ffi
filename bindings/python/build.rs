use std::env;
use std::error::Error;
use std::io;
use std::path::Path;
use std::process::Command;

const LIBRARY_NAME: &str = "maplibre-native-c";

fn main() -> Result<(), Box<dyn Error>> {
    println!("cargo:rerun-if-env-changed=MLN_FFI_NATIVE_INSTALL_DIR");
    println!("cargo:rerun-if-env-changed=MLN_FFI_HOST_LIBRARY_DIRS");
    println!("cargo:rerun-if-env-changed=PKG_CONFIG_PATH");

    let install_dir = native_install_dir()?;
    let host_library_dirs = host_library_dirs()?;
    let pkg_config_dir = install_dir.join("share/pkgconfig");
    require_dir(&pkg_config_dir, "native pkg-config directory")?;
    println!(
        "cargo:rerun-if-changed={}",
        pkg_config_dir.join(format!("{LIBRARY_NAME}.pc")).display()
    );

    let pkg_config_path = match env::var_os("PKG_CONFIG_PATH") {
        Some(existing) if !existing.is_empty() => {
            format!(
                "{}:{}",
                pkg_config_dir.display(),
                existing.to_string_lossy()
            )
        }
        _ => pkg_config_dir.display().to_string(),
    };

    for flag in pkg_config_flags("--libs", &pkg_config_path)? {
        emit_link_flag(&flag);
    }
    if cfg!(unix) {
        for host_library_dir in &host_library_dirs {
            println!(
                "cargo:rustc-link-arg=-Wl,-rpath,{}",
                host_library_dir.display()
            );
        }
    }
    for flag in pkg_config_flags("--cflags", &pkg_config_path)? {
        if let Some(include_dir) = flag.strip_prefix("-I") {
            println!("cargo:include={include_dir}");
        }
    }

    Ok(())
}

fn host_library_dirs() -> Result<Vec<std::path::PathBuf>, Box<dyn Error>> {
    let library_dirs = env::var_os("MLN_FFI_HOST_LIBRARY_DIRS").ok_or_else(|| {
        io::Error::new(
            io::ErrorKind::NotFound,
            "MLN_FFI_HOST_LIBRARY_DIRS is required; run Python binding builds through mise",
        )
    })?;
    let separator = if cfg!(windows) { ';' } else { ':' };
    Ok(library_dirs
        .to_string_lossy()
        .split(separator)
        .filter(|path| !path.is_empty())
        .map(std::path::PathBuf::from)
        .collect())
}

fn native_install_dir() -> Result<std::path::PathBuf, Box<dyn Error>> {
    let install_dir = env::var_os("MLN_FFI_NATIVE_INSTALL_DIR").ok_or_else(|| {
        io::Error::new(
            io::ErrorKind::NotFound,
            "MLN_FFI_NATIVE_INSTALL_DIR is required; run Python binding builds through mise",
        )
    })?;
    Ok(install_dir.into())
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

fn pkg_config_flags(arg: &str, pkg_config_path: &str) -> Result<Vec<String>, Box<dyn Error>> {
    let output = Command::new("pkg-config")
        .arg(arg)
        .arg(LIBRARY_NAME)
        .env("PKG_CONFIG_PATH", pkg_config_path)
        .output()
        .map_err(|error| {
            io::Error::new(
                error.kind(),
                format!("failed to run pkg-config for {LIBRARY_NAME}: {error}"),
            )
        })?;
    if !output.status.success() {
        return Err(io::Error::other(format!(
            "pkg-config {arg} {LIBRARY_NAME} failed: {}",
            String::from_utf8_lossy(&output.stderr).trim()
        ))
        .into());
    }
    Ok(String::from_utf8(output.stdout)?
        .split_whitespace()
        .map(str::to_owned)
        .collect())
}

fn emit_link_flag(flag: &str) {
    if let Some(path) = flag.strip_prefix("-L") {
        println!("cargo:rustc-link-search=native={path}");
    } else if let Some(name) = flag.strip_prefix("-l") {
        println!("cargo:rustc-link-lib={name}");
    } else if let Some(rpath) = flag.strip_prefix("-Wl,-rpath,") {
        println!("cargo:rustc-link-arg=-Wl,-rpath,{rpath}");
    } else {
        println!("cargo:rustc-link-arg={flag}");
    }
}
