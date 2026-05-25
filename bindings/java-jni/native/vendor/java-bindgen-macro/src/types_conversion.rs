use crate::util::{CompileErrors, ts2};
use quote::quote;
use syn::__private::TokenStream2;

// returns (path, type)
fn extract_rust_type(ty: &TokenStream2) -> (String, String) {
    let ty = ty.to_string().replace("& mut", "").replace(' ', "");

    let generic = ty.rfind("<").unwrap_or(ty.len());
    let Some(split_index) = ty.rfind("::") else {
        return ("".to_string(), ty);
    };

    if split_index > generic {
        return ("".to_string(), ty);
    }

    let (left, right) = ty.split_at(split_index + 2);
    (left.to_string(), right.to_string())
}

// Extract T from JList<T>
pub fn to_java_list(rust_type: String, errors: &mut CompileErrors) -> String {
    let default = "List<Object>".to_string();
    let Some(split_index) = rust_type.find('<') else {
        return default;
    };
    let (_, right) = rust_type.split_at(split_index + 1);

    let Some(split_index) = right.rfind('>') else {
        return default;
    };
    let (ty, _) = right.split_at(split_index);

    if ty == "u8" || ty == "i8" {
        return "List<Byte>".to_string();
    }
    if ty == "i16" {
        return "List<Short>".to_string();
    }
    if ty == "i32" {
        return "List<Integer>".to_string();
    }
    if ty == "i64" {
        return "List<Long>".to_string();
    }
    if ty == "f32" {
        return "List<Float>".to_string();
    }
    if ty == "f64" {
        return "List<Double>".to_string();
    }
    if ty == "char" {
        return "List<Character>".to_string();
    }
    if ty == "bool" {
        return "List<Boolean>".to_string();
    }

    let obj = rewrite_rust_to_java(&ts2(ty), errors).unwrap_or("Object".to_string());
    format!("List<{obj}>")
}

// Extract T from Option<T>
fn extract_from_option(rust_type: String, errors: &mut CompileErrors) -> String {
    let default = "void".to_string();
    let Some(split_index) = rust_type.find('<') else {
        return default;
    };
    let (_, right) = rust_type.split_at(split_index + 1);

    let Some(split_index) = right.rfind('>') else {
        return default;
    };
    let (ty, _) = right.split_at(split_index);
    rewrite_rust_to_java(&ts2(ty), errors).unwrap_or("void".to_string())
}

// rewrite [Rust] to [Java Type]
pub fn rewrite_rust_to_java(ty: &TokenStream2, errors: &mut CompileErrors) -> Option<String> {
    let (_, rust_type) = extract_rust_type(&ty);

    // ignored types
    if rust_type.starts_with("JNIEnv<") {
        return None;
    };
    if rust_type.starts_with("JClass<") {
        return None;
    };

    if rust_type.starts_with("JList<") {
        return Some(to_java_list(rust_type, errors));
    };

    if rust_type.starts_with("Option<") {
        return Some(extract_from_option(rust_type, errors));
    };

    // void
    if rust_type == "()" || rust_type == "" {
        return Some("void".to_string());
    };

    // jni primitives
    if rust_type == "jbyte" {
        return Some("byte".to_string());
    };
    if rust_type == "jchar" {
        return Some("char".to_string());
    };
    if rust_type == "jboolean" {
        return Some("boolean".to_string());
    };
    if rust_type == "jint" {
        return Some("int".to_string());
    };
    if rust_type == "jshort" {
        return Some("short".to_string());
    };
    if rust_type == "jlong" {
        return Some("long".to_string());
    };
    if rust_type == "jfloat" {
        return Some("float".to_string());
    };
    if rust_type == "jdouble" {
        return Some("double".to_string());
    };
    if rust_type.starts_with("JString<") {
        return Some("String".to_string());
    };
    if rust_type.starts_with("JObject<") {
        return Some("Object".to_string());
    };
    if rust_type.starts_with("JByteArray<") {
        return Some("byte[]".to_string());
    };

    // class primitive wrappers
    if rust_type == "JByte" {
        return Some("Byte".to_string());
    };
    if rust_type == "JShort" {
        return Some("Short".to_string());
    };
    if rust_type == "JInt" {
        return Some("Integer".to_string());
    };
    if rust_type == "JLong" {
        return Some("Long".to_string());
    };
    if rust_type == "JFloat" {
        return Some("Float".to_string());
    };
    if rust_type == "JDouble" {
        return Some("Double".to_string());
    };
    if rust_type == "JBoolean" {
        return Some("Boolean".to_string());
    };
    if rust_type == "JChar" {
        return Some("Character".to_string());
    };

    // rust primitives
    if rust_type == "u8" || rust_type == "i8" {
        return Some("byte".to_string());
    }
    if rust_type == "i16" {
        return Some("short".to_string());
    }
    if rust_type == "i32" {
        return Some("int".to_string());
    }
    if rust_type == "i64" {
        return Some("long".to_string());
    }
    if rust_type == "f32" {
        return Some("float".to_string());
    }
    if rust_type == "f64" {
        return Some("double".to_string());
    }
    if rust_type == "bool" {
        return Some("boolean".to_string());
    }
    if rust_type == "char" {
        return Some("char".to_string());
    }
    if rust_type == "Vec<u8>" {
        return Some("byte[]".to_string());
    }

    // objects

    if rust_type == "String" {
        return Some("String".to_string());
    };

    if rust_type == "Vec<u8>" {
        return Some("byte[]".to_string());
    };

    Some(rust_type)

    // debug
    // errors.add_spaned(
    //     ty.span(),
    //     format!("unsupported Type: {rust_type}."),
    // );
    // None
}

// Extract T from Option<T>
fn extract_jni_from_option(
    rust_type: String,
    lifetime: &TokenStream2,
    errors: &mut CompileErrors,
) -> Option<TokenStream2> {
    let Some(split_index) = rust_type.find('<') else {
        return None;
    };
    let (_, right) = rust_type.split_at(split_index + 1);
    let Some(split_index) = right.rfind('>') else {
        return None;
    };
    let (ty, _) = right.split_at(split_index);
    rewrite_rust_type_to_jni(&ts2(ty), lifetime, errors)
}

const OBJECT_TYPES: &[&str] = &[
    "()", "JByte", "JShort", "JInt", "JLong", "JFloat", "JDouble", "JBoolean", "JChar",
];

// Rewrite [Rust Type] to [JNI Rust]
pub fn rewrite_rust_type_to_jni(
    ty: &TokenStream2,
    lifetime: &TokenStream2,
    errors: &mut CompileErrors,
) -> Option<TokenStream2> {
    let (_, rust_type) = extract_rust_type(&ty);

    // Ignored types
    if rust_type.starts_with("JNIEnv<") {
        return None;
    };
    if rust_type.starts_with("JClass<") {
        return None;
    };

    // Option<T>
    if rust_type.starts_with("Option<") {
        return extract_jni_from_option(rust_type, lifetime, errors);
    };

    // JNI Types

    if rust_type.starts_with("JString<") {
        return Some(quote! { jni::objects::JString #lifetime });
    };
    if rust_type.starts_with("JByteArray<") {
        return Some(quote! { jni::objects::JByteArray #lifetime });
    };

    if OBJECT_TYPES.contains(&rust_type.as_str()) {
        return Some(quote! { jni::objects::JObject #lifetime });
    };

    // Primitives
    if rust_type == "jbyte" || rust_type == "u8" || rust_type == "i8" {
        return Some(quote! { jni::sys::jbyte });
    };
    if rust_type == "jboolean" || rust_type == "bool" {
        return Some(quote! { jni::sys::jboolean });
    };
    if rust_type == "jshort" || rust_type == "i16" {
        return Some(quote! { jni::sys::jshort });
    };
    if rust_type == "jint" || rust_type == "i32" {
        return Some(quote! { jni::sys::jint });
    };
    if rust_type == "jlong" || rust_type == "i64" {
        return Some(quote! { jni::sys::jlong });
    };
    if rust_type == "jfloat" || rust_type == "f32" {
        return Some(quote! { jni::sys::jfloat });
    };
    if rust_type == "jdouble" || rust_type == "f64" {
        return Some(quote! { jni::sys::jdouble });
    };
    if rust_type == "jchar" || rust_type == "char" {
        return Some(quote! { jni::sys::jchar });
    };

    // Typed Objects
    if rust_type == "String" {
        return Some(quote! { jni::objects::JString #lifetime });
    };
    if rust_type == "Vec<u8>" {
        return Some(quote! { jni::objects::JByteArray #lifetime  });
    };

    Some(quote! { jni::objects::JObject #lifetime })
}

#[cfg(test)]
pub mod tests {
    use super::{extract_rust_type, rewrite_rust_to_java};
    use crate::{
        types_conversion::rewrite_rust_type_to_jni,
        util::{CompileErrors, ts2},
    };

    #[test]
    fn should_extract_type() {
        let ty = extract_rust_type(&ts2("String"));
        assert_eq!(("".to_string(), "String".to_string()), ty);

        let ty = extract_rust_type(&ts2("std::string::String"));
        assert_eq!(("std::string::".to_string(), "String".to_string()), ty);

        let ty = extract_rust_type(&ts2("JList<std::string::String>"));
        assert_eq!(
            ("".to_string(), "JList<std::string::String>".to_string()),
            ty
        );
    }

    #[test]
    fn should_rewrite_to_java() {
        let errors = &mut CompileErrors::default();
        let ty = rewrite_rust_to_java(&ts2("Option<String>"), errors);
        assert_eq!(Some("String".to_string()), ty);

        let ty = rewrite_rust_to_java(&ts2("Option<std::string::String>"), errors);
        assert_eq!(Some("String".to_string()), ty);

        let ty = rewrite_rust_to_java(&ts2("JList<std::string::String>"), errors);
        assert_eq!(Some("List<String>".to_string()), ty);

        let ty = rewrite_rust_to_java(&ts2("Option<JList<std::string::String>>"), errors);
        assert_eq!(Some("List<String>".to_string()), ty);

        let ty = rewrite_rust_to_java(&ts2("jni::sys::jint"), errors);
        assert_eq!(Some("int".to_string()), ty);

        let ty = rewrite_rust_to_java(&ts2("java_bindgen::interop::JLong"), errors);
        assert_eq!(Some("Long".to_string()), ty);
    }

    #[test]
    fn should_rewrite_to_jni() {
        let errors = &mut CompileErrors::default();
        let lifetime = ts2("<'local>");
        let ty = rewrite_rust_type_to_jni(&ts2("JNIEnv<'l>"), &lifetime, errors)
            .map(|ts| ts.to_string());
        assert_eq!(None, ty);

        let ty = rewrite_rust_type_to_jni(&ts2("&mut JNIEnv<'l>"), &lifetime, errors)
            .map(|ts| ts.to_string());
        assert_eq!(None, ty);

        let ty =
            rewrite_rust_type_to_jni(&ts2("JDouble"), &lifetime, errors).map(|ts| ts.to_string());
        assert_eq!(Some("jni :: objects :: JObject <'local >"), ty.as_deref());

        let ty = rewrite_rust_type_to_jni(&ts2("MyCustomClassStruct"), &lifetime, errors)
            .map(|ts| ts.to_string());
        assert_eq!(Some("jni :: objects :: JObject <'local >"), ty.as_deref());
    }
}
