use std::env;
use std::error::Error;
use std::io;

const LIBRARY_NAME: &str = "maplibre-native-c";

fn main() -> Result<(), Box<dyn Error>> {
    println!("cargo:rerun-if-env-changed=PKG_CONFIG_PATH");
    print_rerun_if_pkg_config_file_changed();

    let library = pkg_config::Config::new()
        .cargo_metadata(false)
        .probe(LIBRARY_NAME)
        .map_err(|error| {
            io::Error::other(format!(
                "could not find {LIBRARY_NAME} with pkg-config; run through mise or add the generated maplibre-native-c.pc directory to PKG_CONFIG_PATH: {error}"
            ))
        })?;

    for arg in library.ld_args {
        if !arg.is_empty() {
            println!("cargo:rustc-link-arg=-Wl,{}", arg.join(","));
        }
    }

    Ok(())
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
