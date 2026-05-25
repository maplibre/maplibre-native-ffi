use crate::util::{self, CompileErrors};
use java_bindgen_core::project_info::ProjectInfo;
use proc_macro::TokenStream;
use quote::quote;

pub fn main(item: TokenStream) -> TokenStream {
    if let Ok(input) = syn::parse::<syn::DeriveInput>(item.clone()) {
        let project_dir = std::path::Path::new(".");
        let mut errors = CompileErrors::default();

        // Struct Guard
        let syn::Data::Struct(_) = input.data else {
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
        let struct_name = &input.ident;
        let class_path =
            crate::common::class_path(&project_info, project_info.get_java_class_name());

        return quote! {
            impl #struct_name {
                fn init<'local>(env: &mut jni::JNIEnv<'local>) -> java_bindgen::logger::JLoggerCore<'local> {
                    java_bindgen::logger::JLoggerCore::new(env, #class_path).unwrap_or_default()
                }
            }
        }.into();
    }
    TokenStream::default()
}
