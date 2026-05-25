use java_bindgen_core::{
    consts::ffi_definitions_path,
    ffi_store::{FFIStore, JavaFFIClass},
    project_info::ProjectInfo,
};
use proc_macro::TokenStream;
use quote::quote;
use syn::__private::TokenStream2;
use syn::{Data, DeriveInput};

use crate::util::{self, CompileErrors};

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

        return impl_java_type(&project_info, &input, &errors).into();
    }

    TokenStream::default()
}

pub fn impl_java_type(
    project_info: &ProjectInfo,
    input: &DeriveInput,
    errors: &CompileErrors,
) -> TokenStream2 {
    let name = &input.ident;
    let (_, ty_generics, where_clause) = input.generics.split_for_impl();
    let class_path = crate::common::class_path(&project_info, name.to_string());
    quote! {

        #errors

        impl<'local> java_bindgen::interop::JTypeInfo<'local> for #name #ty_generics #where_clause {
            fn j_type() -> jni::signature::JavaType {
                jni::signature::JavaType::Object(#class_path.to_string())
            }

            fn j_return_type() -> jni::signature::ReturnType {
                jni::signature::ReturnType::Object
            }

            fn into_j_value(self, env: &mut jni::JNIEnv<'local>) -> java_bindgen::JResult<jni::objects::JValueOwned<'local>> {
                let obj = self.into_java(env)?;
                Ok(jni::objects::JValueOwned::Object(obj))
            }
        }

    }
}
