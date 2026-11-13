//! The TypeScript binding's browser module: its Node-API addon
//! (bindings/typescript/native), with mln-rs's C library and a Node-API of its
//! own (`napi`).
//!
//! The binding's browser entry loads the package and calls `mlnTsAddon()`,
//! whose exports it calls as it calls the Node-API addon's on other hosts.
//! Everything runs on the thread that loaded the module, which the library
//! never blocks. The module starts its tile workers' Web Workers from its own
//! script, which loads this package (js/tile_worker.js), and loads mln-rs's
//! font-face shaper module from beside the package.

mod napi;

use std::cell::OnceCell;

// The C library, whose exports the addon calls: Rust links a crate only when
// code names it.
use maplibre_native_c as _;

use wasm_bindgen::prelude::*;

unsafe extern "C" {
    /// The addon's registration, in bindings/typescript/native.
    fn napi_register_module_v1(env: napi::napi_env, exports: napi::napi_value) -> napi::napi_value;
    /// The suite's native probe, in test/support/native_probe.c.
    #[cfg(feature = "test")]
    fn mln_ts_test_native_probe(
        env: napi::napi_env,
        info: napi::napi_callback_info,
    ) -> napi::napi_value;
}

#[wasm_bindgen(module = "/js/package.js")]
extern "C" {
    #[wasm_bindgen(js_name = tileWorkerSpawner)]
    fn tile_worker_spawner(url: &str) -> js_sys::Function;
}

thread_local! {
    static ADDON: OnceCell<JsValue> = const { OnceCell::new() };
}

/// The addon's exports, which the first call registers. Later calls return
/// the same object.
#[wasm_bindgen(js_name = mlnTsAddon)]
pub fn addon() -> Result<JsValue, JsValue> {
    if let Some(exports) = ADDON.with(|addon| addon.get().cloned()) {
        return Ok(exports);
    }
    napi::install();
    // Before any runtime: a spawn function takes the place of mln-rs's
    // default script, which loads mln-rs's own package rather than this one.
    let script = wasm_bindgen::link_to!(module = "/js/tile_worker.js");
    mln_web_host::tile_workers::configure(None, Some(tile_worker_spawner(&script)));
    let exports = napi::register(napi_register_module_v1, add_test_exports)?;
    ADDON.with(|addon| addon.set(exports.clone()).ok());
    Ok(exports)
}

/// Adds `testGraphics.nativeProbe(listener)` to the exports of the suite's
/// module.
#[cfg(feature = "test")]
fn add_test_exports(env: napi::napi_env, exports: napi::napi_value) {
    // SAFETY: Node-API calls on the registration's own values.
    unsafe {
        let mut test = std::ptr::null_mut();
        let mut probe = std::ptr::null_mut();
        napi::napi_create_object(env, &raw mut test);
        napi::napi_create_function(
            env,
            c"nativeProbe".as_ptr(),
            usize::MAX,
            Some(mln_ts_test_native_probe),
            std::ptr::null_mut(),
            &raw mut probe,
        );
        napi::napi_set_named_property(env, test, c"nativeProbe".as_ptr(), probe);
        napi::napi_set_named_property(env, exports, c"testGraphics".as_ptr(), test);
    }
}

#[cfg(not(feature = "test"))]
fn add_test_exports(_env: napi::napi_env, _exports: napi::napi_value) {}
