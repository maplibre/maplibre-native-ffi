use std::fs;
use std::path::{Path, PathBuf};

#[test]
// Rust regression: source-level audit for the architecture rule that raw C and
// FFI carrier types stay out of Rust's supported safe public surface.
fn public_surface_does_not_expose_raw_c_api() {
    let mut failures = Vec::new();

    for path in rust_source_files(&source_dir()) {
        if path.file_name().is_some_and(|name| name == "tests.rs") {
            continue;
        }
        let source = fs::read_to_string(&path).unwrap();
        for declaration in public_declarations(&source) {
            let raw_pointer_is_allowed = declaration.inside_allowed_pointer_impl;
            for pattern in [
                "maplibre_native_sys",
                "sys::",
                "mln_",
                "std::ffi::c_void",
                "core::ffi::c_void",
                "NonNull<",
                "extern \"C\"",
            ] {
                if declaration.text.contains(pattern) {
                    failures.push(format!(
                        "{}:{} exposes `{}`:\n{}",
                        path.display(),
                        declaration.line,
                        pattern,
                        declaration.text.trim()
                    ));
                }
            }

            if !raw_pointer_is_allowed
                && (declaration.text.contains("*mut ") || declaration.text.contains("*const "))
            {
                failures.push(format!(
                    "{}:{} exposes a raw pointer outside NativePointer interop:\n{}",
                    path.display(),
                    declaration.line,
                    declaration.text.trim()
                ));
            }
        }
    }

    assert!(
        failures.is_empty(),
        "safe public surface leaked raw C API:\n{}",
        failures.join("\n\n")
    );
}

#[test]
// Rust regression: Rust handles intentionally avoid public fields and native
// identity constructors so safe users cannot fabricate live owners or tokens.
fn owned_handles_cannot_be_fabricated_from_public_identity() {
    let source_dir = source_dir();
    let mut failures = Vec::new();
    let handle_types = [
        "RuntimeHandle",
        "MapHandle",
        "MapProjectionHandle",
        "RenderSessionHandle",
        "DetachedRenderSessionHandle",
        "MetalOwnedTextureFrameHandle",
        "VulkanOwnedTextureFrameHandle",
        "OpenGLOwnedTextureFrameHandle",
        "OfflineOperationHandle",
        "ResourceRequestHandle",
    ];

    for path in rust_source_files(&source_dir) {
        if path.file_name().is_some_and(|name| name == "tests.rs") {
            continue;
        }
        let source = fs::read_to_string(&path).unwrap();
        for handle_type in handle_types {
            private_handle_fields(&source, handle_type, &path, &mut failures);
            no_public_identity_constructors(&source, handle_type, &path, &mut failures);
        }
    }

    assert!(
        failures.is_empty(),
        "owned handle fabrication guard failed:\n{}",
        failures.join("\n\n")
    );
}

#[derive(Debug)]
struct PublicDeclaration {
    line: usize,
    text: String,
    inside_allowed_pointer_impl: bool,
}

fn source_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("src")
}

fn rust_source_files(dir: &Path) -> Vec<PathBuf> {
    let mut files = Vec::new();
    for entry in fs::read_dir(dir).unwrap() {
        let path = entry.unwrap().path();
        if path.is_dir() {
            files.extend(rust_source_files(&path));
        } else if path.extension().is_some_and(|extension| extension == "rs") {
            files.push(path);
        }
    }
    files
}

fn public_declarations(source: &str) -> Vec<PublicDeclaration> {
    let lines: Vec<_> = source.lines().collect();
    let mut declarations = Vec::new();
    let mut index = 0;
    let mut current_impl = None::<&'static str>;
    let mut impl_depth = 0usize;

    while index < lines.len() {
        let line = lines[index];
        let trimmed = line.trim_start();
        let mut started_allowed_impl = false;

        if current_impl.is_none() {
            if trimmed.starts_with("impl NativePointer") {
                current_impl = Some("NativePointer");
            } else if trimmed.starts_with("impl<'frame> FrameNativePointer") {
                current_impl = Some("FrameNativePointer");
            }
            if current_impl.is_some() {
                impl_depth = brace_delta(line, 0);
                started_allowed_impl = true;
            }
        }

        let mut next_index = index + 1;
        if starts_public_declaration(trimmed) {
            let start = index;
            let mut end = index;
            let mut text = String::new();
            let mut depth = 0usize;

            while end < lines.len() {
                let current = lines[end];
                text.push_str(current);
                text.push('\n');
                depth = brace_delta(current, depth);

                let current_trimmed = current.trim_end();
                if current_trimmed.ends_with(';') || (end > start && depth == 0) {
                    break;
                }
                if end == start && current_trimmed.ends_with('{') {
                    break;
                }
                end += 1;
            }

            declarations.push(PublicDeclaration {
                line: start + 1,
                text,
                inside_allowed_pointer_impl: matches!(
                    current_impl,
                    Some("NativePointer" | "FrameNativePointer")
                ),
            });
            next_index = end + 1;
        }

        if current_impl.is_some() && !started_allowed_impl {
            for current in &lines[index..next_index] {
                impl_depth = brace_delta(current, impl_depth);
            }
            if impl_depth == 0 {
                current_impl = None;
            }
        }

        index = next_index;
    }

    declarations
}

fn starts_public_declaration(trimmed: &str) -> bool {
    trimmed.starts_with("pub ")
        || trimmed.starts_with("pub unsafe ")
        || trimmed.starts_with("pub const ")
}

fn brace_delta(line: &str, depth: usize) -> usize {
    let opens = line.bytes().filter(|byte| *byte == b'{').count();
    let closes = line.bytes().filter(|byte| *byte == b'}').count();
    depth.saturating_add(opens).saturating_sub(closes)
}

fn private_handle_fields(source: &str, handle_type: &str, path: &Path, failures: &mut Vec<String>) {
    let needle = format!("pub struct {handle_type}");
    let Some(start) = source.find(&needle) else {
        return;
    };
    let rest = &source[start..];
    let Some(open) = rest.find('{') else {
        return;
    };
    let Some(close) = rest[open + 1..].find('}') else {
        return;
    };
    let body = &rest[open + 1..open + 1 + close];

    for line in body.lines() {
        if line.trim_start().starts_with("pub ") {
            failures.push(format!(
                "{} exposes a public field on owned handle `{}`:\n{}",
                path.display(),
                handle_type,
                line.trim()
            ));
        }
    }
}

fn no_public_identity_constructors(
    source: &str,
    handle_type: &str,
    path: &Path,
    failures: &mut Vec<String>,
) {
    let allowed_lifecycle_constructors = ["RuntimeHandle", "MapHandle"];
    let impl_needle = format!("impl {handle_type}");
    let mut rest = source;

    while let Some(start) = rest.find(&impl_needle) {
        rest = &rest[start + impl_needle.len()..];
        let Some(open) = rest.find('{') else {
            continue;
        };
        let Some(close) = rest[open + 1..].find("\n}") else {
            continue;
        };
        let body = &rest[open + 1..open + 1 + close];

        for line in body.lines() {
            let trimmed = line.trim_start();
            let exposes_public_new = trimmed.starts_with("pub fn new")
                || trimmed.starts_with("pub const fn new")
                || trimmed.starts_with("pub unsafe fn new");
            let exposes_public_identity_constructor = trimmed.starts_with("pub fn from")
                || trimmed.starts_with("pub unsafe fn from")
                || trimmed.starts_with("pub fn with_raw")
                || trimmed.starts_with("pub unsafe fn with_raw");

            if exposes_public_identity_constructor
                || (exposes_public_new && !allowed_lifecycle_constructors.contains(&handle_type))
            {
                failures.push(format!(
                    "{} exposes public owned-handle construction for `{}`:\n{}",
                    path.display(),
                    handle_type,
                    trimmed
                ));
            }
        }

        rest = &rest[open + 1 + close..];
    }
}
