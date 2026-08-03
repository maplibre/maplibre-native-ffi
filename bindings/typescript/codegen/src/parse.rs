//! Header extraction through libclang.
//!
//! The headers are parsed once per ABI class. Sizes, alignments, and field
//! offsets come from Clang for that target rather than from rules written here,
//! so a target whose layout differs from the assumption cannot go unnoticed.

use std::collections::BTreeMap;
use std::path::Path;

use clang::{Clang, Entity, EntityKind, Index, Type, TypeKind};

use crate::model::{Constant, Entrypoint, EnumType, Param, SlotKind, Symbol};

/// Handle typedefs. Each is `uint64_t`, so only the name distinguishes them.
pub const HANDLE_TYPEDEFS: &[&str] = &[
    "mln_runtime",
    "mln_map",
    "mln_map_projection",
    "mln_offline_region_snapshot",
    "mln_offline_region_list",
    "mln_json_snapshot",
    "mln_resource_request_handle",
    "mln_render_session",
    "mln_wake_source",
    "mln_style_id_list",
    "mln_feature_query_result",
    "mln_feature_extension_result",
];

/// One record's measurements for a single ABI class.
#[derive(Clone, Debug, Default)]
pub struct RecordLayout {
    pub size: u64,
    pub align: u64,
    pub is_union: bool,
    /// Field name to (offset, size), including fields of anonymous members.
    pub fields: BTreeMap<String, (u64, u64)>,
    /// Declaration order, so the emitted table reads like the header.
    pub order: Vec<String>,
    /// Field name to the type as written.
    pub spellings: BTreeMap<String, String>,
}

#[derive(Debug, Default)]
pub struct Parsed {
    pub records: BTreeMap<String, RecordLayout>,
    pub entrypoints: Vec<Entrypoint>,
    pub enums: Vec<EnumType>,
    pub constants: Vec<Constant>,
    pub symbols: Vec<Symbol>,
}

/// Locates Clang's builtin header directory.
///
/// libclang derives that directory from the running executable, which is this
/// generator rather than a Clang installation, so the parse has to be told where
/// `stdint.h` and `stdbool.h` live. `MLN_TS_CODEGEN_RESOURCE_DIR` overrides the
/// probe for an environment whose Clang is not on `PATH`.
fn resource_dir() -> Result<String, String> {
    if let Ok(directory) = std::env::var("MLN_TS_CODEGEN_RESOURCE_DIR") {
        return Ok(directory);
    }
    let mut attempts = Vec::new();
    for candidate in [
        std::env::var("CLANG").unwrap_or_default(),
        "clang".to_string(),
    ] {
        if candidate.is_empty() {
            continue;
        }
        match std::process::Command::new(&candidate)
            .arg("-print-resource-dir")
            .output()
        {
            Ok(output) if output.status.success() => {
                return Ok(String::from_utf8_lossy(&output.stdout).trim().to_string());
            }
            Ok(output) => attempts.push(format!("{candidate}: exited {}", output.status)),
            Err(error) => attempts.push(format!("{candidate}: {error}")),
        }
    }
    Err(format!(
        "no Clang to report a resource directory; set MLN_TS_CODEGEN_RESOURCE_DIR ({})",
        attempts.join("; ")
    ))
}

pub fn parse(include_dir: &Path, umbrella: &[&Path], triple: &str) -> Result<Parsed, String> {
    let clang = Clang::new().map_err(|error| format!("libclang unavailable: {error}"))?;
    let index = Index::new(&clang, false, false);
    let resource_dir = resource_dir()?;
    let mut parsed = Parsed::default();

    for header in umbrella {
        let unit = index
            .parser(header)
            .arguments(&[
                "-xc".to_string(),
                "-std=c23".to_string(),
                format!("-I{}", include_dir.display()),
                "-target".to_string(),
                triple.to_string(),
                "-resource-dir".to_string(),
                resource_dir.clone(),
            ])
            .detailed_preprocessing_record(true)
            .skip_function_bodies(true)
            .parse()
            .map_err(|error| format!("parsing {} for {triple}: {error}", header.display()))?;

        let diagnostics: Vec<_> = unit
            .get_diagnostics()
            .into_iter()
            .filter(|diagnostic| {
                matches!(
                    diagnostic.get_severity(),
                    clang::diagnostic::Severity::Error | clang::diagnostic::Severity::Fatal
                )
            })
            .map(|diagnostic| diagnostic.get_text())
            .collect();
        if !diagnostics.is_empty() {
            return Err(format!(
                "parsing {} for {triple} reported errors:\n{}",
                header.display(),
                diagnostics.join("\n")
            ));
        }

        for entity in unit.get_entity().get_children() {
            if !declared_in(&entity, include_dir) {
                continue;
            }
            match entity.get_kind() {
                EntityKind::FunctionDecl => collect_function(&entity, &mut parsed)?,
                EntityKind::StructDecl | EntityKind::UnionDecl => {
                    collect_record(&entity, &mut parsed)?;
                }
                EntityKind::EnumDecl => collect_enum(&entity, &mut parsed),
                EntityKind::MacroDefinition => collect_macro(&entity, &mut parsed),
                _ => {}
            }
        }
    }

    parsed
        .entrypoints
        .sort_by(|left, right| left.name.cmp(&right.name));
    parsed
        .entrypoints
        .dedup_by(|left, right| left.name == right.name);
    parsed
        .enums
        .sort_by(|left, right| left.name.cmp(&right.name));
    parsed.enums.dedup_by(|left, right| left.name == right.name);
    parsed
        .constants
        .sort_by(|left, right| left.name.cmp(&right.name));
    parsed
        .constants
        .dedup_by(|left, right| left.name == right.name);
    parsed
        .symbols
        .sort_by(|left, right| left.name.cmp(&right.name));
    parsed
        .symbols
        .dedup_by(|left, right| left.name == right.name);
    Ok(parsed)
}

fn declared_in(entity: &Entity, include_dir: &Path) -> bool {
    entity
        .get_location()
        .and_then(|location| location.get_file_location().file)
        .map(|file| file.get_path().starts_with(include_dir))
        .unwrap_or(false)
}

fn collect_function(entity: &Entity, parsed: &mut Parsed) -> Result<(), String> {
    let Some(name) = entity.get_name() else {
        return Ok(());
    };
    if !name.starts_with("mln_") {
        return Ok(());
    }
    let mut params = Vec::new();
    for (index, argument) in entity
        .get_arguments()
        .unwrap_or_default()
        .iter()
        .enumerate()
    {
        let argument_type = argument
            .get_type()
            .ok_or_else(|| format!("{name}: argument {index} has no type"))?;
        params.push(Param {
            name: argument
                .get_name()
                .unwrap_or_else(|| format!("argument_{index}")),
            spelling: argument_type.get_display_name(),
            slot: slot_of(&argument_type).ok_or_else(|| {
                format!(
                    "{name}: unsupported argument type {}",
                    argument_type.get_display_name()
                )
            })?,
        });
    }

    let result_type = entity
        .get_result_type()
        .ok_or_else(|| format!("{name}: no result type"))?;
    let result_slot = slot_of(&result_type);
    parsed.entrypoints.push(Entrypoint {
        name: name.clone(),
        params,
        result_record: if result_slot == Some(SlotKind::Struct) {
            Some(result_type.get_display_name())
        } else {
            None
        },
        result_spelling: result_type.get_display_name(),
        result: result_slot.ok_or_else(|| {
            format!(
                "{name}: unsupported result type {}",
                result_type.get_display_name()
            )
        })?,
    });
    parsed.symbols.push(Symbol { name });
    Ok(())
}

fn collect_record(entity: &Entity, parsed: &mut Parsed) -> Result<(), String> {
    let Some(name) = entity.get_name() else {
        return Ok(());
    };
    if !name.starts_with("mln_") || entity.get_definition() != Some(*entity) {
        return Ok(());
    }
    let record_type = entity
        .get_type()
        .ok_or_else(|| format!("{name}: struct without a type"))?;
    let mut layout = RecordLayout {
        size: byte_size(&record_type, &name)?,
        align: record_type
            .get_alignof()
            .map_err(|error| format!("{name}: alignof failed: {error:?}"))? as u64,
        is_union: entity.get_kind() == EntityKind::UnionDecl,
        ..RecordLayout::default()
    };
    collect_fields(entity, &record_type, &name, &mut layout)?;
    parsed.records.insert(name, layout);
    Ok(())
}

fn collect_fields(
    entity: &Entity,
    record_type: &Type,
    record_name: &str,
    layout: &mut RecordLayout,
) -> Result<(), String> {
    for field in entity.get_children() {
        if field.get_kind() != EntityKind::FieldDecl {
            continue;
        }
        let field_type = field
            .get_type()
            .ok_or_else(|| format!("{record_name}: field without a type"))?;
        match field.get_name() {
            Some(field_name) => {
                let offset_bits = record_type.get_offsetof(&field_name).map_err(|error| {
                    format!("{record_name}.{field_name}: offsetof failed: {error:?}")
                })?;
                layout.fields.insert(
                    field_name.clone(),
                    (offset_bits as u64 / 8, byte_size(&field_type, record_name)?),
                );
                layout
                    .spellings
                    .insert(field_name.clone(), field_type.get_display_name());
                layout.order.push(field_name);
            }
            // An anonymous member contributes its own fields, whose offsets the
            // parent record still answers for.
            None => {
                if let Some(declaration) = field_type.get_declaration() {
                    collect_fields(&declaration, record_type, record_name, layout)?;
                }
            }
        }
    }
    Ok(())
}

fn byte_size(ty: &Type, context: &str) -> Result<u64, String> {
    ty.get_sizeof().map(|size| size as u64).map_err(|error| {
        format!(
            "{context}: sizeof {} failed: {error:?}",
            ty.get_display_name()
        )
    })
}

fn collect_enum(entity: &Entity, parsed: &mut Parsed) {
    let Some(name) = entity.get_name() else {
        return;
    };
    if !name.starts_with("mln_") {
        return;
    }
    let signed = entity
        .get_enum_underlying_type()
        .map(|underlying| {
            matches!(
                underlying.get_canonical_type().get_kind(),
                TypeKind::CharS
                    | TypeKind::SChar
                    | TypeKind::Short
                    | TypeKind::Int
                    | TypeKind::Long
                    | TypeKind::LongLong
            )
        })
        .unwrap_or(true);
    let mut members = Vec::new();
    for member in entity.get_children() {
        if member.get_kind() != EntityKind::EnumConstantDecl {
            continue;
        }
        let Some(member_name) = member.get_name() else {
            continue;
        };
        if let Some((signed_value, unsigned_value)) = member.get_enum_constant_value() {
            members.push((
                member_name,
                if signed {
                    signed_value
                } else {
                    unsigned_value as i64
                },
            ));
        }
    }
    parsed.enums.push(EnumType { name, members });
}

fn collect_macro(entity: &Entity, parsed: &mut Parsed) {
    let Some(name) = entity.get_name() else {
        return;
    };
    if !name.starts_with("MLN_") || entity.is_function_like_macro() {
        return;
    }
    let Some(range) = entity.get_range() else {
        return;
    };
    let tokens: Vec<String> = range
        .tokenize()
        .into_iter()
        .map(|token| token.get_spelling())
        .collect();
    if tokens.len() < 2 {
        return;
    }
    let body = tokens[1..].join(" ");
    if let Some(value) = evaluate_integer_macro(&body) {
        parsed.constants.push(Constant { name, value });
    }
}

/// Reads the integer value out of the small set of macro spellings these headers
/// use: `((uint64_t)0)`, `UINT32_MAX`, and plain integer literals.
fn evaluate_integer_macro(body: &str) -> Option<String> {
    let cleaned: String = body
        .chars()
        .filter(|character| !matches!(character, '(' | ')' | ' '))
        .collect();
    let cleaned = cleaned
        .trim_start_matches("uint64_t")
        .trim_start_matches("uint32_t")
        .trim_start_matches("int64_t")
        .trim_start_matches("int32_t")
        .trim_end_matches(['u', 'U', 'l', 'L']);
    match cleaned {
        "UINT32_MAX" => Some("4294967295".to_string()),
        "UINT64_MAX" => Some("18446744073709551615".to_string()),
        "UINT8_MAX" => Some("255".to_string()),
        other => other.parse::<i128>().ok().map(|value| value.to_string()),
    }
}

/// Classifies a C type into the normalized call ABI's slot kinds.
///
/// The written type is inspected before its canonical form, because `size_t` and
/// the handle typedefs canonicalize to integers whose meaning the ABI needs to
/// keep apart.
pub fn slot_of(ty: &Type) -> Option<SlotKind> {
    if let Some(declaration) = ty.get_declaration()
        && let Some(name) = declaration.get_name()
    {
        if HANDLE_TYPEDEFS.contains(&name.as_str()) {
            return Some(SlotKind::Handle);
        }
        if name == "size_t" {
            return Some(SlotKind::Usize);
        }
    }
    let display = ty.get_display_name();
    let bare = display.trim_start_matches("const ").trim();
    if bare == "size_t" {
        return Some(SlotKind::Usize);
    }
    if HANDLE_TYPEDEFS.contains(&bare) {
        return Some(SlotKind::Handle);
    }

    let canonical = ty.get_canonical_type();
    match canonical.get_kind() {
        TypeKind::Void => Some(SlotKind::Void),
        TypeKind::Bool => Some(SlotKind::Bool),
        TypeKind::Float => Some(SlotKind::F32),
        TypeKind::Double => Some(SlotKind::F64),
        TypeKind::Pointer | TypeKind::IncompleteArray | TypeKind::ConstantArray => {
            Some(SlotKind::Ptr)
        }
        TypeKind::Record => Some(SlotKind::Struct),
        TypeKind::Enum => integer_slot(&canonical),
        TypeKind::CharS
        | TypeKind::CharU
        | TypeKind::SChar
        | TypeKind::UChar
        | TypeKind::Short
        | TypeKind::UShort
        | TypeKind::Int
        | TypeKind::UInt
        | TypeKind::Long
        | TypeKind::ULong
        | TypeKind::LongLong
        | TypeKind::ULongLong => integer_slot(&canonical),
        _ => None,
    }
}

/// Integer slots follow width and signedness rather than the C spelling, because
/// `uint64_t` is `unsigned long` on one target and `unsigned long long` on
/// another.
fn integer_slot(canonical: &Type) -> Option<SlotKind> {
    let signed = match canonical.get_kind() {
        TypeKind::Enum => canonical
            .get_declaration()
            .and_then(|declaration| declaration.get_enum_underlying_type())
            .map(|underlying| is_signed(&underlying.get_canonical_type()))
            .unwrap_or(true),
        _ => is_signed(canonical),
    };
    match canonical.get_sizeof().ok()? {
        1 | 2 | 4 => Some(if signed { SlotKind::I32 } else { SlotKind::U32 }),
        8 => Some(if signed { SlotKind::I64 } else { SlotKind::U64 }),
        _ => None,
    }
}

fn is_signed(canonical: &Type) -> bool {
    matches!(
        canonical.get_kind(),
        TypeKind::CharS
            | TypeKind::SChar
            | TypeKind::Short
            | TypeKind::Int
            | TypeKind::Long
            | TypeKind::LongLong
    )
}
