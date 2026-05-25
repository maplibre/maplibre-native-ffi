use java_bindgen_core::{
    consts::ffi_definitions_path,
    ffi_store::{FFIStore, JavaFFIClass},
    project_info::ProjectInfo,
};
use proc_macro::TokenStream;
use quote::{ToTokens, TokenStreamExt, format_ident, quote};
use syn::__private::TokenStream2;
use syn::{Data, DeriveInput};

use crate::{
    common,
    util::{self, CompileErrors},
};

pub fn main(item: TokenStream) -> TokenStream {
    if let Ok(input) = syn::parse::<DeriveInput>(item.clone()) {
        let project_dir = std::path::Path::new(".");
        let mut errors = CompileErrors::default();

        // Struct Guard
        let Data::Struct(ref struct_info) = input.data else {
            errors.add("Only struct is allowed.".to_string());
            return errors.into();
        };

        // Parse Cargo.toml file
        let cargo_toml = match util::parse_project_toml(project_dir) {
            Ok(toml) => toml,
            Err(err) => {
                return util::error(input.ident.span(), err.to_string()).into();
            }
        };

        // Create project info
        let project_info = ProjectInfo::from(&cargo_toml);
        let fields = crate::common::get_struct_fileds(&struct_info.fields, &mut errors);
        let Some(java_fields) = crate::common::produce_java_class_ffi_types(&fields, &mut errors)
        else {
            return errors.into();
        };

        if let Some(mut store) = FFIStore::read_from_file(&ffi_definitions_path(project_dir)) {
            store.add_ffi_class(JavaFFIClass {
                id: input.ident.to_string(),
                fields: java_fields,
            });
            store.save();
        }

        return impl_into_java(&project_info, &input, &fields, &errors).into();
    }

    TokenStream::default()
}

pub fn impl_into_java(
    project_info: &ProjectInfo,
    input: &DeriveInput,
    fields: &Vec<(syn::Ident, syn::Type)>,
    errors: &CompileErrors,
) -> TokenStream2 {
    let name = &input.ident;

    // rust to java type covertion
    let mut type_signature = quote! {};
    let mut args_conversion = quote! {};
    let mut args_list = quote! {};
    for (i, (name, ty)) in fields.into_iter().enumerate() {
        let arg_name = format_ident!("a{i}");

        // ',' sepparated Types
        if !type_signature.is_empty() {
            type_signature.append_all(quote! {, })
        }
        type_signature.append_all(ty.to_token_stream());

        // Type convertions
        args_conversion.append_all(quote! {
            let #arg_name = self.#name.into_j_value(env)?;
        });

        // ',' sepparated Args names
        if !args_list.is_empty() {
            args_list.append_all(quote! {, })
        }
        args_list.append_all(quote! {#arg_name.borrow()});
    }

    let class_path = common::class_path(&project_info, name.to_string());
    let (_, ty_generics, where_clause) = input.generics.split_for_impl();

    quote! {

        #errors

        impl <'local> java_bindgen::r2j::IntoJavaType<'local, jni::objects::JObject<'local>> for #name #ty_generics #where_clause {
            fn into_java(self, env: &mut jni::JNIEnv<'local>) -> java_bindgen::JResult<jni::objects::JObject<'local>> {
                let sig = signature_by_type!(#type_signature => JVoid);

                #args_conversion

                let class = env.find_class(#class_path).j_catch(env)?;
                env.new_object(class, sig.to_string(), &[#args_list]).j_catch(env)
            }
        }

    }
}
