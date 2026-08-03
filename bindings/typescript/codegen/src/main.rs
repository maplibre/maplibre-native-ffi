//! Generates the TypeScript binding's raw layer and its normalized C dispatch.
//!
//! Run through `mise run //bindings/typescript:generate`. Every output is
//! checked in, so CI regenerates and diffs rather than building the generator
//! into a consumer's install.
//!
//! Usage: `mln-ts-codegen <include-dir> <binding-dir>`.

mod emit;
mod model;
mod parse;

use std::path::{Path, PathBuf};
use std::process::ExitCode;

use sha2::{Digest, Sha256};

/// The ABI classes the binding supports.
///
/// The native class is measured on an LP64 target. Windows is LLP64, but these
/// headers use fixed-width types and pointers alone, so the two agree; the
/// generated assertions compile on every target and would fail if that stopped
/// being true.
const NATIVE_TRIPLE: &str = "x86_64-unknown-linux-gnu";
/// Emscripten shares the wasm32 C ABI, and this triple needs no SDK sysroot to
/// parse, so the layout query stays runnable wherever Clang is.
const WASM_TRIPLE: &str = "wasm32-unknown-unknown";

fn main() -> ExitCode {
    match run() {
        Ok(outputs) => {
            for output in outputs {
                println!("wrote {}", output.display());
            }
            ExitCode::SUCCESS
        }
        Err(message) => {
            eprintln!("mln-ts-codegen: {message}");
            ExitCode::FAILURE
        }
    }
}

fn run() -> Result<Vec<PathBuf>, String> {
    let mut arguments = std::env::args().skip(1);
    let include_dir = PathBuf::from(
        arguments
            .next()
            .ok_or("usage: mln-ts-codegen <include-dir> <binding-dir>")?,
    );
    let binding_dir = PathBuf::from(
        arguments
            .next()
            .ok_or("usage: mln-ts-codegen <include-dir> <binding-dir>")?,
    );

    let umbrella = include_dir.join("maplibre_native_c.h");
    let adapter = include_dir.join("maplibre_native_c/callback_adapter.h");
    let headers = [umbrella.as_path(), adapter.as_path()];

    let native = parse::parse(&include_dir, &headers, NATIVE_TRIPLE)?;
    let wasm = parse::parse(&include_dir, &headers, WASM_TRIPLE)?;
    check_agreement(&native, &wasm)?;

    let header_digest = digest_headers(&include_dir)?;
    let schema = emit::canonical_schema(&native, &wasm, &header_digest);
    let fingerprint = emit::fingerprint(&schema);

    let raw = binding_dir.join("api/src/raw");
    let generated = binding_dir.join("host-support/generated");
    let mut outputs = Vec::new();
    for (path, contents) in [
        (raw.join("entrypoints.ts"), emit::entrypoints_ts(&native)),
        (raw.join("layouts.ts"), emit::layouts_ts(&native, &wasm)),
        (raw.join("enums.ts"), emit::enums_ts(&native)),
        (raw.join("constants.ts"), emit::constants_ts(&native)),
        (
            raw.join("fingerprint.ts"),
            emit::fingerprint_ts(&fingerprint, &header_digest),
        ),
        (generated.join("dispatch.inc"), emit::dispatch_inc(&native)?),
        (generated.join("symbols.inc"), emit::symbols_inc(&native)),
        (
            generated.join("entrypoint_names.inc"),
            emit::entrypoint_names_inc(&native),
        ),
        (
            generated.join("layout_assert.c"),
            emit::layout_assert_c(&native, &wasm),
        ),
        (
            generated.join("result_is_status.inc"),
            emit::result_is_status_inc(&native),
        ),
        (
            generated.join("fingerprint.h"),
            emit::fingerprint_h(&fingerprint, &header_digest, native.entrypoints.len()),
        ),
    ] {
        write_if_changed(&path, &contents)?;
        outputs.push(path);
    }
    Ok(outputs)
}

/// The two parses must describe the same API; only measurements may differ.
fn check_agreement(native: &parse::Parsed, wasm: &parse::Parsed) -> Result<(), String> {
    if native.entrypoints.len() != wasm.entrypoints.len() {
        return Err(format!(
            "the two ABI parses disagree about the entrypoint set: {} native, {} wasm32",
            native.entrypoints.len(),
            wasm.entrypoints.len()
        ));
    }
    for (left, right) in native.entrypoints.iter().zip(&wasm.entrypoints) {
        if left.name != right.name {
            return Err(format!(
                "the two ABI parses disagree about entrypoint order: {} against {}",
                left.name, right.name
            ));
        }
    }
    for name in native.records.keys() {
        if !wasm.records.contains_key(name) {
            return Err(format!("{name} is missing from the wasm32 parse"));
        }
    }
    Ok(())
}

/// Digests the public header bytes the generation ran against.
///
/// The dispatch table is checked in, so it describes one exact library ABI. The
/// addon build compares this against the headers shipped with the native
/// artifact it links and refuses a mismatch.
fn digest_headers(include_dir: &Path) -> Result<String, String> {
    let mut paths = Vec::new();
    collect_headers(include_dir, &mut paths)?;
    paths.sort();
    let mut hasher = Sha256::new();
    for path in paths {
        let relative = path
            .strip_prefix(include_dir)
            .map_err(|error| format!("{}: {error}", path.display()))?;
        hasher.update(relative.to_string_lossy().replace('\\', "/").as_bytes());
        hasher.update([0]);
        hasher.update(
            std::fs::read(&path).map_err(|error| format!("reading {}: {error}", path.display()))?,
        );
        hasher.update([0]);
    }
    Ok(hasher
        .finalize()
        .iter()
        .map(|byte| format!("{byte:02x}"))
        .collect())
}

fn collect_headers(directory: &Path, paths: &mut Vec<PathBuf>) -> Result<(), String> {
    let entries = std::fs::read_dir(directory)
        .map_err(|error| format!("reading {}: {error}", directory.display()))?;
    for entry in entries {
        let entry = entry.map_err(|error| format!("reading {}: {error}", directory.display()))?;
        let path = entry.path();
        if path.is_dir() {
            collect_headers(&path, paths)?;
        } else if path.extension().is_some_and(|extension| extension == "h") {
            paths.push(path);
        }
    }
    Ok(())
}

fn write_if_changed(path: &Path, contents: &str) -> Result<(), String> {
    if let Some(parent) = path.parent() {
        std::fs::create_dir_all(parent)
            .map_err(|error| format!("creating {}: {error}", parent.display()))?;
    }
    if std::fs::read_to_string(path).is_ok_and(|existing| existing == contents) {
        return Ok(());
    }
    std::fs::write(path, contents).map_err(|error| format!("writing {}: {error}", path.display()))
}
