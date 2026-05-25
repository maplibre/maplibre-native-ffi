use java_bindgen_core::cargo_parser::{CargoToml, CargoTomlFile, parse_toml};
use proc_macro::TokenStream;
use quote::{ToTokens, TokenStreamExt, quote, quote_spanned};
use std::collections::HashMap;
use std::str::FromStr;
use syn::__private::TokenStream2;

pub fn parse_attr_to_map(attr: TokenStream) -> HashMap<String, String> {
    let mut map: HashMap<String, String> = HashMap::new();
    for entry in attr.to_string().split(',') {
        let key_value = entry.split('=').collect::<Vec<&str>>();
        let key = key_value.first().unwrap_or(&"");
        let value = key_value.get(1).unwrap_or(&"");
        map.insert(
            key.trim().to_string(),
            value.trim().replace('\"', "").to_string(),
        );
    }
    map
}

pub fn error(span: syn::__private::Span, message: String) -> TokenStream2 {
    quote_spanned!( span => compile_error!(#message); )
}

pub struct CompileErrors {
    quote: TokenStream2,
}

impl Default for CompileErrors {
    fn default() -> Self {
        Self {
            quote: Default::default(),
        }
    }
}

impl ToTokens for CompileErrors {
    fn to_tokens(&self, tokens: &mut TokenStream2) {
        tokens.append_all(self.quote.clone())
    }
}

impl From<CompileErrors> for TokenStream {
    fn from(value: CompileErrors) -> Self {
        value.quote.into()
    }
}

#[allow(dead_code)]
impl CompileErrors {
    pub fn add(&mut self, message: String) {
        let msg = format!("[java-bindgen]\n{message}\n");
        self.quote.append_all(quote!( compile_error!(#msg); ))
    }
    pub fn add_spaned(&mut self, span: syn::__private::Span, message: String) {
        let msg = format!("[java-bindgen]\n{message}\n");
        self.quote
            .append_all(quote_spanned!( span => compile_error!(#msg); ))
    }
}

pub fn ts2(tokens: &str) -> TokenStream2 {
    TokenStream2::from_str(tokens).unwrap_or_else(|_| {
        quote! {/* Error */}
    })
}

pub fn parse_project_toml(project_dir: &std::path::Path) -> Result<CargoToml, String> {
    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR")
        .map(std::path::PathBuf::from)
        .unwrap_or_else(|_| project_dir.to_path_buf());
    match parse_toml(&manifest_dir.join("Cargo.toml")) {
        Ok(CargoTomlFile {
            toml_parsed,
            toml_path,
            ..
        }) => {
            let example =
                "\n\nExample:\n[package.metadata.java-bindgen]\npackage = \"com.java.package\"\n\n";
            if toml_parsed.java_bindgen().is_none() {
                return Err(format!(
                    "Add java-bindgen metadata in your Cargo.toml file. {example}\nfile:{}",
                    toml_path.to_string_lossy()
                ));
            }

            let config = toml_parsed.java_bindgen().expect("To be checked");
            if config.package.is_none() {
                return Err(format!(
                    "Add: package = \"com.java.package\" in your Cargo.toml file.{example}"
                ));
            }

            Ok(toml_parsed)
        }
        Err(err) => Err(err.to_string()),
    }
}

pub fn lifetime_from_type(ty: &syn::Type) -> Option<TokenStream2> {
    if let syn::Type::Path(syn::TypePath { ref path, .. }) = ty {
        for segment in &path.segments {
            if let syn::PathArguments::AngleBracketed(ref args) = &segment.arguments {
                for arg in &args.args {
                    if let syn::GenericArgument::Lifetime(lifetime) = arg {
                        return Some(quote! {<#lifetime>});
                    }
                }
            }
        }
    }

    None
}
