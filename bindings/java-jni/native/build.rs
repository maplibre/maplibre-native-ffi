use std::{collections::BTreeMap, env, fs, path::PathBuf};

const UNSUPPORTED_METHODS: &[&str] = &[
    "mln_animation_options_default",
    "mln_bound_options_default",
    "mln_camera_fit_options_default",
    "mln_camera_options_default",
    "mln_custom_geometry_source_options_default",
    "mln_feature_extension_result_destroy",
    "mln_feature_extension_result_get",
    "mln_feature_query_result_count",
    "mln_feature_query_result_destroy",
    "mln_feature_query_result_get",
    "mln_free_camera_options_default",
    "mln_json_snapshot_destroy",
    "mln_json_snapshot_get",
    "mln_map_options_default",
    "mln_map_tile_options_default",
    "mln_map_viewport_options_default",
    "mln_metal_borrowed_texture_descriptor_default",
    "mln_metal_owned_texture_descriptor_default",
    "mln_metal_surface_descriptor_default",
    "mln_offline_region_list_count",
    "mln_offline_region_list_destroy",
    "mln_offline_region_list_get",
    "mln_offline_region_snapshot_destroy",
    "mln_offline_region_snapshot_get",
    "mln_premultiplied_rgba8_image_default",
    "mln_projection_mode_default",
    "mln_rendered_feature_query_options_default",
    "mln_rendered_query_geometry_box",
    "mln_rendered_query_geometry_line_string",
    "mln_rendered_query_geometry_point",
    "mln_runtime_options_default",
    "mln_source_feature_query_options_default",
    "mln_style_id_list_count",
    "mln_style_id_list_destroy",
    "mln_style_id_list_get",
    "mln_style_image_info_default",
    "mln_style_image_options_default",
    "mln_style_tile_source_options_default",
    "mln_texture_image_info_default",
    "mln_vulkan_borrowed_texture_descriptor_default",
    "mln_vulkan_owned_texture_descriptor_default",
    "mln_vulkan_surface_descriptor_default",
];

fn main() {
    let manifest_dir = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").expect("manifest dir"));
    let bridge_dir = manifest_dir
        .join("../src/main/java/org/maplibre/nativejni/internal/bridge")
        .canonicalize()
        .expect("bridge Java source directory");
    println!("cargo:rerun-if-changed={}", bridge_dir.display());

    let mut entries = Vec::new();
    for entry in fs::read_dir(&bridge_dir).expect("read bridge Java directory") {
        let path = entry.expect("bridge Java entry").path();
        if path.extension().and_then(|ext| ext.to_str()) != Some("java") {
            continue;
        }
        println!("cargo:rerun-if-changed={}", path.display());
        let class_name = path.file_stem().unwrap().to_string_lossy().to_string();
        let source = fs::read_to_string(&path).expect("read Java bridge source");
        let imports = java_imports(&source);
        for mut declaration in native_declarations(&source) {
            if class_name == "JniTestNative" && declaration.name == "unregisteredNativeForTesting" {
                continue;
            }
            declaration.signature = java_signature(&declaration, &imports);
            entries.push((class_name.clone(), declaration));
        }
    }
    entries.sort_by(|left, right| left.0.cmp(&right.0).then(left.1.name.cmp(&right.1.name)));

    let out_dir = PathBuf::from(env::var_os("OUT_DIR").expect("out dir"));
    fs::write(out_dir.join("bindgen_exports.rs"), render_exports(&entries))
        .expect("write generated java-bindgen exports");
}

#[derive(Debug)]
struct NativeDeclaration {
    return_type: String,
    name: String,
    parameters: Vec<String>,
    signature: String,
}

fn java_imports(source: &str) -> BTreeMap<String, String> {
    source
        .lines()
        .filter_map(|line| {
            let line = line.trim();
            let qualified = line
                .strip_prefix("import ")?
                .strip_suffix(';')?
                .trim()
                .to_string();
            let simple = qualified.rsplit('.').next()?.to_string();
            Some((simple, qualified))
        })
        .collect()
}

fn native_declarations(source: &str) -> Vec<NativeDeclaration> {
    let mut declarations = Vec::new();
    let normalized = source.replace('\n', " ");
    for segment in normalized.split(';') {
        let Some(native_index) = segment.find("public static native ") else {
            continue;
        };
        let rest = segment[native_index + "public static native ".len()..].trim();
        let Some(open) = rest.find('(') else { continue };
        let Some(close) = rest.rfind(')') else {
            continue;
        };
        let head = rest[..open].trim();
        let params = rest[open + 1..close].trim();
        let Some((return_type, name)) = head.rsplit_once(char::is_whitespace) else {
            continue;
        };
        declarations.push(NativeDeclaration {
            return_type: return_type.trim().to_string(),
            name: name.trim().to_string(),
            parameters: parse_parameters(params),
            signature: String::new(),
        });
    }
    declarations
}

fn parse_parameters(parameters: &str) -> Vec<String> {
    if parameters.is_empty() {
        return Vec::new();
    }
    parameters
        .split(',')
        .map(|parameter| {
            parameter
                .trim()
                .rsplit_once(char::is_whitespace)
                .map(|(ty, _name)| ty.trim().to_string())
                .unwrap_or_else(|| parameter.trim().to_string())
        })
        .collect()
}

fn render_exports(entries: &[(String, NativeDeclaration)]) -> String {
    let mut out = String::from("// Generated by bindings/java-jni/native/build.rs.\n\n");

    for (class_name, declaration) in entries {
        out.push_str(&render_export(class_name, declaration));
        out.push('\n');
    }
    out.push_str(&render_registration(entries));
    out
}

fn render_export(class_name: &str, declaration: &NativeDeclaration) -> String {
    let wrapper_name = format!("jbg_{class_name}_{}", declaration.name);
    let args = declaration
        .parameters
        .iter()
        .enumerate()
        .map(|(index, ty)| format!("a{index}: {}", wrapper_type(ty)))
        .collect::<Vec<_>>()
        .join(", ");
    let call_args = declaration
        .parameters
        .iter()
        .enumerate()
        .map(|(index, ty)| call_argument(ty, &format!("a{index}")))
        .collect::<Vec<_>>();
    let target = target_function(class_name, &declaration.name);
    let call = if UNSUPPORTED_METHODS.contains(&declaration.name.as_str()) {
        "unsupported_status(env_for_call, class)".to_string()
    } else {
        let mut call = format!("{target}(env_for_call, class");
        if !call_args.is_empty() {
            call.push_str(", ");
            call.push_str(&call_args.join(", "));
        }
        call.push(')');
        call
    };

    let unused_args = (0..declaration.parameters.len())
        .map(|index| format!("    let _ = &a{index};\n"))
        .collect::<String>();

    format!(
        r#"#[java_bindgen(package = "org.maplibre.nativejni.internal.bridge")]
fn {wrapper_name}<'local>(
    env: &mut JNIEnv<'local>,
    {args}
) -> JResult<{return_type}> {{
    let env_for_call = unsafe {{ JNIEnv::from_raw(env.get_raw()).expect("valid JNIEnv") }};
    let class = JClass::from(JObject::null());
{unused_args}    {return_expression}
}}
"#,
        return_type = return_type(&declaration.return_type),
        return_expression = return_expression(&declaration.return_type, &call),
    )
}

fn render_registration(entries: &[(String, NativeDeclaration)]) -> String {
    let mut by_class = BTreeMap::<&str, Vec<&NativeDeclaration>>::new();
    for (class_name, declaration) in entries {
        by_class
            .entry(class_name.as_str())
            .or_default()
            .push(declaration);
    }

    let mut out = String::from(
        r#"pub fn register_generated_natives(vm: &JavaVM) -> jni::errors::Result<()> {
    let mut env = vm.get_env()?;
"#,
    );
    for (class_name, declarations) in by_class {
        out.push_str(&format!(
            "    register_generated_class(&mut env, \"org/maplibre/nativejni/internal/bridge/{class_name}\", vec![\n"
        ));
        for declaration in declarations {
            out.push_str(&format!(
                "        jni::NativeMethod {{ name: \"{method}\".into(), sig: \"{signature}\".into(), fn_ptr: {symbol} as *mut std::ffi::c_void }},\n",
                method = declaration.name,
                signature = declaration.signature,
                symbol = jni_symbol_name(class_name, &declaration.name),
            ));
        }
        out.push_str("    ])?;\n");
    }
    out.push_str(
        r#"    Ok(())
}

fn register_generated_class(
    env: &mut JNIEnv<'_>,
    class_name: &str,
    methods: Vec<jni::NativeMethod>,
) -> jni::errors::Result<()> {
    let class = env.find_class(class_name)?;
    env.register_native_methods(class, &methods)
}
"#,
    );
    out
}

fn jni_symbol_name(class_name: &str, method_name: &str) -> String {
    let wrapper_name = format!("jbg_{class_name}_{method_name}");
    format!(
        "Java_org_maplibre_nativejni_internal_bridge_MaplibreNativeJni_{}",
        escape_jni_identifier(&wrapper_name)
    )
}

fn escape_jni_identifier(identifier: &str) -> String {
    identifier.replace('_', "_1")
}

fn java_signature(declaration: &NativeDeclaration, imports: &BTreeMap<String, String>) -> String {
    let params = declaration
        .parameters
        .iter()
        .map(|ty| java_type_signature(ty, imports))
        .collect::<String>();
    format!(
        "({}){}",
        params,
        java_type_signature(&declaration.return_type, imports)
    )
}

fn java_type_signature(java_type: &str, imports: &BTreeMap<String, String>) -> String {
    if let Some(component) = java_type.strip_suffix("[]") {
        return format!("[{}", java_type_signature(component, imports));
    }
    match java_type {
        "void" => "V".to_string(),
        "boolean" => "Z".to_string(),
        "byte" => "B".to_string(),
        "char" => "C".to_string(),
        "short" => "S".to_string(),
        "int" => "I".to_string(),
        "long" => "J".to_string(),
        "float" => "F".to_string(),
        "double" => "D".to_string(),
        "String" => "Ljava/lang/String;".to_string(),
        "Object" => "Ljava/lang/Object;".to_string(),
        "Integer" => "Ljava/lang/Integer;".to_string(),
        "Runnable" => "Ljava/lang/Runnable;".to_string(),
        ty => {
            let qualified = imports.get(ty).map(String::as_str).unwrap_or(ty);
            format!("L{};", java_binary_name(qualified))
        }
    }
}

fn java_binary_name(qualified: &str) -> String {
    let segments = qualified.split('.').collect::<Vec<_>>();
    let Some(class_index) = segments
        .iter()
        .position(|segment| segment.starts_with(char::is_uppercase))
    else {
        return qualified.replace('.', "/");
    };

    let mut binary = segments[..class_index].join("/");
    if !binary.is_empty() {
        binary.push('/');
    }
    binary.push_str(&segments[class_index..].join("$"));
    binary
}

fn target_function(_class_name: &str, method_name: &str) -> String {
    match method_name {
        "panicStatus" => "test_panic_status".to_string(),
        "createManyLocalStrings" => "test_create_many_local_strings".to_string(),
        "invokeOnAttachedNativeThread" => "test_invoke_on_attached_native_thread".to_string(),
        "cVersion" => "c_version".to_string(),
        "supportedRenderBackendMask" => "supported_render_backend_mask".to_string(),
        "networkStatusGet" => "network_status_get".to_string(),
        "networkStatusSet" => "network_status_set".to_string(),
        "threadLastErrorMessage" => "thread_last_error_message".to_string(),
        "mln_runtime_offline_region_create_start" => "offline_region_create_start".to_string(),
        "mln_runtime_offline_region_create_take_result" => {
            "offline_region_create_take_result".to_string()
        }
        "mln_runtime_offline_region_delete_start" => "offline_region_delete_start".to_string(),
        "mln_runtime_offline_region_get_start" => "offline_region_get_start".to_string(),
        "mln_runtime_offline_region_get_status_start" => {
            "offline_region_get_status_start".to_string()
        }
        "mln_runtime_offline_region_get_status_take_result" => {
            "offline_region_get_status_take_result".to_string()
        }
        "mln_runtime_offline_region_get_take_result" => {
            "offline_region_get_take_result".to_string()
        }
        "mln_runtime_offline_region_invalidate_start" => {
            "offline_region_invalidate_start".to_string()
        }
        "mln_runtime_offline_region_set_download_state_start" => {
            "offline_region_set_download_state_start".to_string()
        }
        "mln_runtime_offline_region_set_observed_start" => {
            "offline_region_set_observed_start".to_string()
        }
        "mln_runtime_offline_region_update_metadata_start" => {
            "offline_region_update_metadata_start".to_string()
        }
        "mln_runtime_offline_region_update_metadata_take_result" => {
            "offline_region_update_metadata_take_result".to_string()
        }
        "mln_runtime_offline_regions_list_start" => "offline_regions_list_start".to_string(),
        "mln_runtime_offline_regions_list_take_result" => {
            "offline_regions_list_take_result".to_string()
        }
        "mln_runtime_offline_regions_merge_database_start" => {
            "offline_regions_merge_database_start".to_string()
        }
        "mln_runtime_offline_regions_merge_database_take_result" => {
            "offline_regions_merge_database_take_result".to_string()
        }
        "mln_map_projection_create" => "projection_create".to_string(),
        "mln_map_projection_destroy" => "projection_destroy".to_string(),
        "mln_map_projection_get_camera" => "projection_get_camera".to_string(),
        "mln_map_projection_lat_lng_for_pixel" => "projection_lat_lng_for_pixel".to_string(),
        "mln_map_projection_pixel_for_lat_lng" => "projection_pixel_for_lat_lng".to_string(),
        "mln_map_projection_set_camera" => "projection_set_camera".to_string(),
        "mln_map_projection_set_visible_coordinates" => {
            "projection_set_visible_coordinates".to_string()
        }
        "mln_map_projection_set_visible_geometry" => "projection_set_visible_geometry".to_string(),
        name => name.strip_prefix("mln_").unwrap_or(name).to_string(),
    }
}

fn wrapper_type(java_type: &str) -> &'static str {
    match java_type {
        "int" => "i32",
        "long" => "i64",
        "double" => "f64",
        "boolean" => "bool",
        _ => "JObject<'local>",
    }
}

fn call_argument(java_type: &str, name: &str) -> String {
    match java_type {
        "boolean" => format!("jboolean::from({name})"),
        "int" | "long" | "double" => name.to_string(),
        "String" => format!("JString::from({name})"),
        "long[]" => format!("JLongArray::from({name})"),
        "int[]" => format!("JIntArray::from({name})"),
        "boolean[]" => format!("JBooleanArray::from({name})"),
        "byte[]" => format!("JByteArray::from({name})"),
        "double[]" => format!("JDoubleArray::from({name})"),
        ty if ty.ends_with("[]") => format!("JObjectArray::from({name})"),
        _ => name.to_string(),
    }
}

fn return_type(java_type: &str) -> &'static str {
    match java_type {
        "int" => "i32",
        "long" => "i64",
        "boolean" => "bool",
        "void" => "()",
        "String" => "JObject<'local>",
        _ => "JObject<'local>",
    }
}

fn return_expression(java_type: &str, call: &str) -> String {
    match java_type {
        "void" => format!("{{ {call}; Ok(()) }}"),
        "boolean" => format!("Ok({call} != 0)"),
        "String" => format!("Ok(unsafe {{ JObject::from_raw({call}) }})"),
        _ => format!("Ok({call})"),
    }
}
