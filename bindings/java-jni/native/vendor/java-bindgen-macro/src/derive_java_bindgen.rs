use java_bindgen_core::{
    consts::ffi_definitions_path,
    ffi_store::{FFIStore, JavaFFIMethod},
    project_info::ProjectInfo,
};
use proc_macro::TokenStream;
use quote::{ToTokens, TokenStreamExt, format_ident, quote};
use syn::{__private::TokenStream2, FnArg, Token, Type, punctuated::Punctuated, spanned::Spanned};

use crate::common::BindgenReturnType;
use crate::{
    common::{
        produce_java_args, produce_java_return, produce_rust_args_names, produce_rust_result_type,
    },
    types_conversion::rewrite_rust_type_to_jni,
    util::{self, CompileErrors, parse_attr_to_map, ts2},
};

// Extract lifetime form args
pub fn extract_jni_env_lifetime(
    inputs: &Punctuated<FnArg, Token![,]>,
    _errors: &mut CompileErrors,
) -> Option<TokenStream2> {
    for ele in inputs.iter() {
        if let FnArg::Typed(typed) = ele {
            let ty_str = typed.ty.to_token_stream().to_string().replace(' ', "");
            if !ty_str.contains("JNIEnv") {
                continue;
            }
            match *typed.ty.clone() {
                Type::Path(_path) => {}
                Type::Reference(refer) => {
                    if let Some(l) = util::lifetime_from_type(&refer.elem) {
                        return Some(l);
                    }
                }
                _ => {}
            };
        }
    }
    None
}

struct JavaFnSig {
    args: TokenStream2,
    env_indent: TokenStream2,
    #[allow(dead_code)]
    class_indent: TokenStream2,
    into_rust_ident: Vec<TokenStream2>,
    jni_env_lifetime: TokenStream2,
}

// Rust fn arguments for inner function call
fn produce_fn_java_args_signature(
    inputs: &Punctuated<FnArg, Token![,]>,
    errors: &mut CompileErrors,
) -> JavaFnSig {
    let jni_env_lifetime = extract_jni_env_lifetime(inputs, errors).unwrap_or(quote! { <'l> });
    let mut into_rust_ident = vec![];
    let mut env_indent = quote! { env };
    let mut class_indent = quote! { _classs };
    let mut jni_env = quote! { mut #env_indent: JNIEnv #jni_env_lifetime };
    let mut jni_class = quote! { #class_indent: JClass #jni_env_lifetime };
    let mut args = quote! {};

    for (i, ele) in inputs.iter().enumerate() {
        match ele {
            FnArg::Receiver(r) => {
                errors.add_spaned(
                    r.span(),
                    "'self' parameter is not supported. Use functions for defining java bindings."
                        .into(),
                );
                args.append_all(quote! { self, #args });
            }
            FnArg::Typed(typed) => {
                // Rewrite [Rust] to [RustJNI]
                let ty = if let Some(ty) =
                    rewrite_rust_type_to_jni(&typed.ty.to_token_stream(), &jni_env_lifetime, errors)
                {
                    into_rust_ident.push(typed.pat.to_token_stream());
                    ty
                } else {
                    typed.ty.to_token_stream()
                };
                let type_string = ty.to_token_stream().to_string().replace(' ', "");

                let pat = &typed.pat;
                let pat_string = pat.to_token_stream().to_string().replace(' ', "");

                if type_string.contains("JNIEnv") {
                    if !type_string.contains("&mut") {
                        errors.add_spaned(
                            typed.span(),
                            "'JNIEnv' must be declared as mutable reference: &mut JNIEnv<'a>"
                                .into(),
                        );
                    }
                    if pat_string.trim() == "_" {
                        env_indent = format_ident!("arg{i}").to_token_stream()
                    } else {
                        env_indent = pat.to_token_stream()
                    }

                    let jni_env_type = ts2(&type_string.replace("&mut", ""));
                    jni_env = quote! { mut #env_indent : #jni_env_type };
                    continue;
                }

                if type_string.contains("JClass") {
                    if pat_string.trim() == "_" {
                        class_indent = format_ident!("arg{i}").to_token_stream()
                    } else {
                        class_indent = pat.to_token_stream()
                    }
                    jni_class = quote! { #class_indent : #ty };
                    continue;
                }
                args.append_all(quote! { , #pat : #ty  });
            }
        }
    }

    JavaFnSig {
        args: quote! { #jni_env, #jni_class #args },
        env_indent,
        class_indent,
        jni_env_lifetime,
        into_rust_ident,
    }
}

// Macro attributes
struct JavaBindgenAttr {
    pub package: String,
    pub class: Option<String>,
    pub method: Option<String>,
    pub returns: Option<String>,
}

impl JavaBindgenAttr {
    pub fn parse_attr(attr: TokenStream) -> JavaBindgenAttr {
        let map = parse_attr_to_map(attr);
        JavaBindgenAttr {
            package: map.get("package").unwrap_or(&"".to_string()).clone(),
            class: map.get("class").cloned(),
            method: map.get("method").cloned(),
            returns: map
                .get("return")
                .cloned()
                .or_else(|| map.get("returns").cloned()),
        }
    }
}

pub fn main(attr: TokenStream, item: TokenStream) -> TokenStream {
    let mut errors = CompileErrors::default();

    if let Ok(java_fn) = syn::parse::<syn::ItemFn>(item.clone()) {
        let source = TokenStream2::from(item.clone());
        let attribute = JavaBindgenAttr::parse_attr(attr.clone());
        let project_dir = std::path::Path::new(".");

        // Parse Cargo.toml file
        let cargo_toml = match util::parse_project_toml(project_dir) {
            Ok(toml) => toml,
            Err(err) => {
                let error = util::error(java_fn.span(), err.to_string());
                return quote! {
                    #source
                    #error
                }
                .into();
            }
        };
        // Create project info
        let project_info = ProjectInfo::from(&cargo_toml).set_package_name(&attribute.package);
        let rust_fn_name = java_fn.sig.ident.to_string();
        let return_type = produce_rust_result_type(&java_fn.sig.output, &mut errors);

        // Safe FFI Methods
        if let Some(mut store) = FFIStore::read_from_file(&ffi_definitions_path(project_dir)) {
            let args = produce_java_args(&java_fn.sig.inputs, &mut errors);
            let return_type = attribute
                .returns
                .clone()
                .unwrap_or_else(|| produce_java_return(return_type.as_token(), &mut errors));

            store.add_ffi_method(JavaFFIMethod {
                id: rust_fn_name.clone(),
                sig: format!(
                    "public static native {} {}({})",
                    &return_type,
                    &rust_fn_name,
                    args.join(",")
                ),
            });
            store.save();
        }

        // Rewrite rust function
        let java_method_name = attribute
            .method
            .clone()
            .unwrap_or_else(|| rust_fn_name.clone());
        let j_ffi_fn_name = if let Some(class_name) = attribute.class.as_ref() {
            let packages = project_info.get_packages_path();
            format_ident!(
                "Java_{}_{}_{}",
                packages.join("_"),
                class_name,
                java_method_name.replace('_', "_1")
            )
        } else {
            format_ident!("{}", project_info.get_java_method_name(&java_method_name))
        };
        let fn_name = java_fn.sig.ident.to_token_stream();
        let args_names = produce_rust_args_names(&java_fn.sig.inputs, &mut errors);

        let JavaFnSig {
            args,
            env_indent,
            class_indent: _,
            jni_env_lifetime,
            into_rust_ident,
        } = produce_fn_java_args_signature(&java_fn.sig.inputs, &mut errors);

        // Input types conversion
        let mut rewrites = quote! {};
        for indent in into_rust_ident {
            rewrites.append_all(quote! {

                let Ok(#indent) = #indent.into_rust(&mut #env_indent) else {
                    return Default::default()
                };

            });
        }

        // Return type conversion
        let jni_return_type =
            rewrite_rust_type_to_jni(return_type.as_token(), &jni_env_lifetime, &mut errors)
                .unwrap_or_else(|| {
                    // Return JObject if custom type specified
                    if attribute.returns.is_some() {
                        quote! { jni::objects::JObject #jni_env_lifetime }
                    } else {
                        return_type.as_token().clone()
                    }
                });

        let return_handler = match return_type {
            BindgenReturnType::JResult(_) => {
                quote! {
                    java_bindgen::exception::j_result_handler(r, &mut #env_indent)
                }
            }
            BindgenReturnType::Option(_) => {
                quote! {
                    java_bindgen::exception::option_handler(r, &mut #env_indent)
                }
            }
            BindgenReturnType::None(_) => {
                quote! {
                    Default::default()
                }
            }
        };

        return quote! {

            #errors

            #[allow(non_snake_case)]
            #source

            #[no_mangle]
            #[allow(unused_mut, non_snake_case, unused_variables)]
            pub extern "system" fn #j_ffi_fn_name #jni_env_lifetime(#args) -> #jni_return_type {

                #rewrites

                let r = #fn_name(#args_names);
                #return_handler
            }

        }
        .into();
    }

    item
}
