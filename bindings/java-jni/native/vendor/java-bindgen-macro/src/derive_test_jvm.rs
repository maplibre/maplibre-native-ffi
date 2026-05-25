use crate::util::CompileErrors;
use proc_macro::TokenStream;
use quote::quote;
use syn::{__private::TokenStream2, spanned::Spanned};

pub fn main(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let mut errors = CompileErrors::default();

    if let Ok(java_fn) = syn::parse::<syn::ItemFn>(item.clone()) {
        let fn_name = &java_fn.sig.ident;
        let source = TokenStream2::from(item.clone());

        if java_fn.sig.inputs.len() != 3 {
            let msg = "Invalid function signature:";
            let example = format!(
                r#"

Example:
fn {fn_name}<'a>(
    test_env: &mut JNIEnv<'a>,
    env: JNIEnv<'a>,
    class: JClass
) -> JResult<()> {{
    // your code
    Ok(())
}}
            "#
            );

            errors.add_spaned(java_fn.sig.inputs.span(), format!("{msg}{example}"));
            return quote! {
                #errors
                #source
            }
            .into();
        }

        // todo validate test fn signature

        let expect = crate::util::ts2(&format!("\"JVM failed! {fn_name}\""));
        return quote! {

            #errors

            #[test]
            pub fn #fn_name() {

                #source

                java_bindgen::test_utils::run_in_jvm(#fn_name).expect(#expect);
            }
        }
        .into();
    }

    item
}
