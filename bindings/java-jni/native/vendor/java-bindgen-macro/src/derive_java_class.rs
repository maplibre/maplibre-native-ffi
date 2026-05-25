use java_bindgen_core::{
    consts::ffi_definitions_path,
    ffi_store::{FFIStore, JavaFFIClass},
    project_info::ProjectInfo,
};
use proc_macro::TokenStream;
use quote::quote;
use syn::{Data, DeriveInput};

use crate::util::CompileErrors;

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
        let cargo_toml = match crate::util::parse_project_toml(project_dir) {
            Ok(toml) => toml,
            Err(err) => {
                return crate::util::error(input.ident.span(), err.to_string()).into();
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

        let into_java =
            crate::dervie_into_java::impl_into_java(&project_info, &input, &fields, &errors);
        let into_rust = crate::derive_into_rust::impl_into_rust(&input, &fields, &errors);
        let java_type = crate::dervie_java_type::impl_java_type(&project_info, &input, &errors);

        return quote! {
            #into_java
            #into_rust
            #java_type

        }
        .into();
    }

    TokenStream::default()
}
