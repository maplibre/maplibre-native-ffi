use java_bindgen_core::{
    consts::ffi_definitions_path,
    ffi_store::{FFIStore, JavaFFIClass},
};
use proc_macro::TokenStream;
use quote::{ToTokens, TokenStreamExt, quote};
use syn::__private::TokenStream2;
use syn::{Data, DeriveInput};

use crate::util::{CompileErrors, ts2};

pub fn main(item: TokenStream) -> TokenStream {
    if let Ok(input) = syn::parse::<DeriveInput>(item.clone()) {
        let project_dir = std::path::Path::new(".");
        let mut errors = CompileErrors::default();
        let name = &input.ident;

        // Struct Guard
        let Data::Struct(ref struct_info) = input.data else {
            errors.add("Only struct is allowed.".to_string());
            return errors.into();
        };

        // Create project info
        let fields = crate::common::get_struct_fileds(&struct_info.fields, &mut errors);
        let Some(java_fields) = crate::common::produce_java_class_ffi_types(&fields, &mut errors)
        else {
            return errors.into();
        };

        if let Some(mut store) = FFIStore::read_from_file(&ffi_definitions_path(project_dir)) {
            store.add_ffi_class(JavaFFIClass {
                id: name.to_string(),
                fields: java_fields,
            });
            store.save();
        }

        return impl_into_rust(&input, &fields, &errors).into();
    }

    item
}

fn is_bool_type(ty: &syn::Type) -> bool {
    let ty_string = ty.to_token_stream().to_string().replace(" ", "");
    if ty_string == "bool" || ty_string == "Option<bool>" {
        return true;
    }
    false
}

pub fn impl_into_rust(
    input: &DeriveInput,
    fields: &Vec<(syn::Ident, syn::Type)>,
    errors: &CompileErrors,
) -> TokenStream2 {
    let name = &input.ident;

    // Call Java Getters
    let mut fields_getters: TokenStream2 = quote! {};
    for (name, ty) in fields.into_iter() {
        let getter_fn_name = name.to_string();
        let prefix = if is_bool_type(&ty) { "is" } else { "get" };

        // Capital first
        let getter_name = format!(
            "\"{prefix}{}{}\"",
            &getter_fn_name[0..1].to_uppercase(),
            &getter_fn_name[1..]
        );
        let getter_name = ts2(&getter_name);
        fields_getters.append_all(quote! {
            #name: self.call_getter(#getter_name, env)?,
        })
    }

    quote! {

        #errors

        impl<'local> java_bindgen::j2r::IntoRustType<'local, #name> for jni::objects::JObject<'local> {
            fn into_rust(self, env: &mut jni::JNIEnv<'local>) -> java_bindgen::JResult<#name> {
                Ok(#name {
                    #fields_getters
                })
            }
        }

        impl<'local> java_bindgen::prelude::IntoRustType<'local, #name> for jni::objects::JValueGen<jni::objects::JObject<'local>> {
            fn into_rust(self, env: &mut jni::JNIEnv<'local>) -> JResult<#name> {
                let obj = self.l()?;
                obj.into_rust(env)
            }
        }

    }
}
