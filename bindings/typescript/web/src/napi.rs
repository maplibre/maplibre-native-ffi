//! The Node-API functions that the binding's C calls, over wasm-bindgen.
//!
//! The module has one environment and one thread, which nothing else enters,
//! so the state is thread-local and every call runs to completion.
//!
//! - A `napi_value` is a slot in a stack of JavaScript values. A handle scope
//!   records the stack's height, and closing it pops what the scope pushed.
//!   Each call of a native function, finalizer, or threadsafe function's
//!   callback runs in a scope of its own, as in Node.js.
//! - A `napi_ref` is a slot in a table of references, which holds its value
//!   strongly while its count is above zero and through a `WeakRef` at zero.
//! - A threadsafe function queues its calls and runs them in a microtask on
//!   the same thread; no other thread exists to call it from.
//! - An external is a frozen object of its own, whose finalizer runs from a
//!   `FinalizationRegistry`.
//! - A buffer's memory isn't the module's: `napi_get_typedarray_info()` gives
//!   out a copy of the bytes, and `napi_create_arraybuffer()` a block that
//!   `mln_napi_sync_arraybuffer()` writes into the buffer. Either lives until
//!   the handle scope that it was made in closes.
//! - The environment lives as long as the page, so it never runs its cleanup
//!   hooks or its instance data's finalizer.
//!
//! The functions have the C ABI of `js_native_api.h` and `node_api.h`.

#![allow(non_camel_case_types, clippy::missing_safety_doc)]

use std::alloc::Layout;
use std::cell::RefCell;
use std::collections::VecDeque;
use std::ffi::{CStr, c_char, c_void};

use wasm_bindgen::prelude::*;
use wasm_bindgen::{JsCast, JsValue};

#[wasm_bindgen(module = "/js/napi.js")]
extern "C" {
    #[wasm_bindgen(js_name = napiInstall)]
    fn install_entry_points(invoke: &JsValue, finalize: &JsValue, drain: &JsValue);
    #[wasm_bindgen(js_name = napiFunction)]
    fn make_function(name: &str, callback: u32, data: u32) -> js_sys::Function;
    #[wasm_bindgen(js_name = napiThrowOnReturn)]
    fn throw_on_return(error: &JsValue);
    #[wasm_bindgen(js_name = napiExternal)]
    fn make_external(data: u32, callback: u32, hint: u32) -> JsValue;
    #[wasm_bindgen(js_name = napiExternalData)]
    fn external_data(value: &JsValue) -> Option<u32>;
    #[wasm_bindgen(js_name = napiIsExternal)]
    fn is_external(value: &JsValue) -> bool;
    #[wasm_bindgen(js_name = napiWeakRef)]
    fn weak_ref(value: &JsValue) -> JsValue;
    #[wasm_bindgen(js_name = napiDeref)]
    fn deref(reference: &JsValue) -> JsValue;
    #[wasm_bindgen(js_name = napiSchedule)]
    fn schedule(function: u32);
    #[wasm_bindgen(js_name = napiToObject)]
    fn to_object(value: &JsValue) -> js_sys::Object;
    #[wasm_bindgen(js_name = napiReportError)]
    fn report_error(error: &JsValue);
    #[wasm_bindgen(js_name = napiIsTypedArray)]
    fn is_typed_array(value: &JsValue) -> bool;
    #[wasm_bindgen(js_name = napiTypedArrayType)]
    fn typed_array_type(value: &JsValue) -> i32;
    #[wasm_bindgen(js_name = napiTypedArray, catch)]
    fn make_typed_array(
        kind: i32,
        buffer: &JsValue,
        offset: f64,
        length: f64,
    ) -> Result<JsValue, JsValue>;
    #[wasm_bindgen(js_name = napiError)]
    fn make_error(kind: u32, message: &str, code: Option<String>) -> JsValue;
    #[wasm_bindgen(js_name = napiAsI64)]
    fn as_i64(value: &JsValue) -> i64;
    #[wasm_bindgen(js_name = napiAsU64)]
    fn as_u64(value: &JsValue) -> u64;
    #[wasm_bindgen(js_name = napiIsLosslessI64)]
    fn is_lossless_i64(value: &JsValue) -> bool;
    #[wasm_bindgen(js_name = napiIsLosslessU64)]
    fn is_lossless_u64(value: &JsValue) -> bool;
}

// Node-API's C types: pointers to opaque structs, which this module numbers.
pub type napi_env = *mut c_void;
pub type napi_value = *mut c_void;
pub type napi_ref = *mut c_void;
pub type napi_handle_scope = *mut c_void;
pub type napi_callback_info = *mut c_void;
pub type napi_threadsafe_function = *mut c_void;
pub type napi_callback = Option<unsafe extern "C" fn(napi_env, napi_callback_info) -> napi_value>;
pub type napi_finalize = Option<unsafe extern "C" fn(napi_env, *mut c_void, *mut c_void)>;
pub type napi_threadsafe_function_call_js =
    Option<unsafe extern "C" fn(napi_env, napi_value, *mut c_void, *mut c_void)>;
pub type napi_cleanup_hook = Option<unsafe extern "C" fn(*mut c_void)>;

#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
#[allow(dead_code)]
pub enum napi_status {
    napi_ok,
    napi_invalid_arg,
    napi_object_expected,
    napi_string_expected,
    napi_name_expected,
    napi_function_expected,
    napi_number_expected,
    napi_boolean_expected,
    napi_array_expected,
    napi_generic_failure,
    napi_pending_exception,
    napi_cancelled,
    napi_escape_called_twice,
    napi_handle_scope_mismatch,
    napi_callback_scope_mismatch,
    napi_queue_full,
    napi_closing,
    napi_bigint_expected,
}
use napi_status::*;

#[repr(C)]
#[derive(Clone, Copy)]
#[allow(dead_code)]
pub enum napi_valuetype {
    napi_undefined,
    napi_null,
    napi_boolean,
    napi_number,
    napi_string,
    napi_symbol,
    napi_object,
    napi_function,
    napi_external,
    napi_bigint,
}

const NAPI_AUTO_LENGTH: usize = usize::MAX;
const NAPI_TSFN_ABORT: i32 = 1;

/// The environment's one address, which every call receives and none reads.
static ENV_TAG: u8 = 0;

fn env() -> napi_env {
    (&raw const ENV_TAG).cast_mut().cast()
}

/// A block of the module's memory that stands for a buffer's bytes.
struct Block {
    pointer: *mut u8,
    layout: Layout,
    /// The scope that frees it as it closes: the stack's scope count when the
    /// block was made.
    depth: usize,
    /// The buffer that `mln_napi_sync_arraybuffer()` writes it into, or none
    /// for a copy of a typed array's bytes.
    buffer: Option<JsValue>,
}

impl Block {
    fn new(size: usize, depth: usize, buffer: Option<JsValue>) -> Option<Block> {
        let layout = Layout::from_size_align(size.max(1), 16).ok()?;
        // SAFETY: the layout has a size.
        let pointer = unsafe { std::alloc::alloc_zeroed(layout) };
        (!pointer.is_null()).then_some(Block {
            pointer,
            layout,
            depth,
            buffer,
        })
    }
}

impl Drop for Block {
    fn drop(&mut self) {
        // SAFETY: allocated with this layout.
        unsafe { std::alloc::dealloc(self.pointer, self.layout) };
    }
}

enum Reference {
    Strong(JsValue),
    Weak(JsValue),
}

struct Env {
    values: Vec<JsValue>,
    scopes: Vec<usize>,
    references: Vec<Option<(Reference, u32)>>,
    free_references: Vec<usize>,
    pending: Option<JsValue>,
    instance_data: *mut c_void,
    blocks: Vec<Block>,
}

thread_local! {
    static ENV: RefCell<Env> = const {
        RefCell::new(Env {
            values: Vec::new(),
            scopes: Vec::new(),
            references: Vec::new(),
            free_references: Vec::new(),
            pending: None,
            instance_data: std::ptr::null_mut(),
            blocks: Vec::new(),
        })
    };
}

fn with<T>(f: impl FnOnce(&mut Env) -> T) -> T {
    ENV.with(|cell| f(&mut cell.borrow_mut()))
}

/// Pushes `value` in the current scope and returns its handle.
fn push(value: JsValue) -> napi_value {
    with(|env| {
        env.values.push(value);
        env.values.len() as napi_value
    })
}

/// The value of a handle, which must be of an open scope.
fn value(handle: napi_value) -> Option<JsValue> {
    let index = (handle as usize).checked_sub(1)?;
    with(|env| env.values.get(index).cloned())
}

/// Writes `value`'s handle to `result`.
unsafe fn give(result: *mut napi_value, value: JsValue) -> napi_status {
    if result.is_null() {
        return napi_invalid_arg;
    }
    // SAFETY: the caller's pointer to a result.
    unsafe { *result = push(value) };
    napi_ok
}

fn set_pending(error: JsValue) -> napi_status {
    with(|env| env.pending = Some(error));
    napi_pending_exception
}

fn is_pending() -> bool {
    with(|env| env.pending.is_some())
}

fn open_scope() -> usize {
    with(|env| {
        let height = env.values.len();
        env.scopes.push(height);
        env.scopes.len()
    })
}

/// Closes the innermost scope, which must be `depth`, with what it holds.
fn close_scope(depth: usize) -> bool {
    let blocks = with(|env| {
        if env.scopes.len() != depth {
            return None;
        }
        let height = env.scopes.pop().unwrap_or_default();
        let values = env.values.split_off(height);
        let kept = env
            .blocks
            .iter()
            .position(|block| block.depth >= depth)
            .unwrap_or(env.blocks.len());
        Some((values, env.blocks.split_off(kept)))
    });
    // Dropped outside the borrow: a value's drop calls into JavaScript.
    blocks.is_some()
}

/// Runs `f` in a scope of its own, and reports an exception that it leaves
/// pending, which nothing else would catch.
fn in_scope(f: impl FnOnce()) {
    let depth = open_scope();
    f();
    report_pending();
    close_scope(depth);
}

fn report_pending() {
    if let Some(error) = with(|env| env.pending.take()) {
        report_error(&error);
    }
}

unsafe fn text<'a>(text: *const c_char, length: usize) -> Option<std::borrow::Cow<'a, str>> {
    if text.is_null() {
        return (length == 0 || length == NAPI_AUTO_LENGTH).then(|| "".into());
    }
    // SAFETY: the caller's text of `length` bytes, or NUL-terminated.
    let bytes = unsafe {
        if length == NAPI_AUTO_LENGTH {
            CStr::from_ptr(text).to_bytes()
        } else {
            std::slice::from_raw_parts(text.cast::<u8>(), length)
        }
    };
    Some(String::from_utf8_lossy(bytes))
}

fn is_object_like(value: &JsValue) -> bool {
    !value.is_undefined() && !value.is_null()
}

macro_rules! arg {
    ($handle:expr) => {
        match value($handle) {
            Some(value) => value,
            None => return napi_invalid_arg,
        }
    };
}

macro_rules! out {
    ($pointer:expr, $value:expr) => {{
        if $pointer.is_null() {
            return napi_invalid_arg;
        }
        // SAFETY: the caller's pointer to a result.
        unsafe { *$pointer = $value };
        napi_ok
    }};
}

/// The callback info of one call of a native function.
struct CallbackInfo {
    this: JsValue,
    arguments: js_sys::Array,
    data: *mut c_void,
}

fn invoke(callback: u32, data: u32, this: JsValue, arguments: js_sys::Array) -> JsValue {
    // SAFETY: napi_create_function() took the callback as a function pointer.
    let callback: unsafe extern "C" fn(napi_env, napi_callback_info) -> napi_value =
        unsafe { std::mem::transmute(callback as usize) };
    let mut info = CallbackInfo {
        this,
        arguments,
        data: data as *mut c_void,
    };
    let depth = open_scope();
    // SAFETY: the C callback, with this call's info.
    let result = unsafe { callback(env(), (&raw mut info).cast()) };
    let returned = if result.is_null() {
        JsValue::UNDEFINED
    } else {
        value(result).unwrap_or(JsValue::UNDEFINED)
    };
    if let Some(error) = with(|env| env.pending.take()) {
        throw_on_return(&error);
    }
    close_scope(depth);
    returned
}

fn finalize(callback: u32, data: u32, hint: u32) {
    // SAFETY: napi_create_external() took the finalizer as a function pointer.
    let callback: unsafe extern "C" fn(napi_env, *mut c_void, *mut c_void) =
        unsafe { std::mem::transmute(callback as usize) };
    // SAFETY: the C finalizer, with what the external carried.
    in_scope(|| unsafe { callback(env(), data as *mut c_void, hint as *mut c_void) });
}

/// Installs the entry points that the JavaScript half calls. They live as
/// long as the module.
pub fn install() {
    thread_local! {
        static INSTALLED: std::cell::Cell<bool> = const { std::cell::Cell::new(false) };
    }
    if INSTALLED.replace(true) {
        return;
    }
    let invoke = Closure::<dyn Fn(u32, u32, JsValue, js_sys::Array) -> JsValue>::new(invoke);
    let finalize = Closure::<dyn Fn(u32, u32, u32)>::new(finalize);
    let drain = Closure::<dyn Fn(u32)>::new(drain);
    install_entry_points(invoke.as_ref(), finalize.as_ref(), drain.as_ref());
    invoke.forget();
    finalize.forget();
    drain.forget();
}

/// Calls a module's registration with a new exports object, in a scope of its
/// own, and returns what it returned, or throws what it left pending.
pub fn register(
    init: unsafe extern "C" fn(napi_env, napi_value) -> napi_value,
    extend: impl FnOnce(napi_env, napi_value),
) -> Result<JsValue, JsValue> {
    let depth = open_scope();
    let exports = push(js_sys::Object::new().into());
    // SAFETY: the registration's C ABI.
    let result = unsafe { init(env(), exports) };
    if !result.is_null() {
        extend(env(), result);
    }
    let pending = with(|env| env.pending.take());
    let value = value(result);
    close_scope(depth);
    match (pending, value) {
        (Some(error), _) => Err(error),
        (None, Some(value)) => Ok(value),
        (None, None) => Err(JsValue::from_str(
            "the addon's registration returned nothing",
        )),
    }
}

// Handle scopes.

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_open_handle_scope(
    _env: napi_env,
    result: *mut napi_handle_scope,
) -> napi_status {
    if result.is_null() {
        return napi_invalid_arg;
    }
    out!(result, open_scope() as napi_handle_scope)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_close_handle_scope(
    _env: napi_env,
    scope: napi_handle_scope,
) -> napi_status {
    if close_scope(scope as usize) {
        napi_ok
    } else {
        napi_handle_scope_mismatch
    }
}

// Values.

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_undefined(
    _env: napi_env,
    result: *mut napi_value,
) -> napi_status {
    unsafe { give(result, JsValue::UNDEFINED) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_global(_env: napi_env, result: *mut napi_value) -> napi_status {
    unsafe { give(result, js_sys::global().into()) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_boolean(
    _env: napi_env,
    value: bool,
    result: *mut napi_value,
) -> napi_status {
    unsafe { give(result, JsValue::from_bool(value)) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_object(
    _env: napi_env,
    result: *mut napi_value,
) -> napi_status {
    unsafe { give(result, js_sys::Object::new().into()) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_array_with_length(
    _env: napi_env,
    length: usize,
    result: *mut napi_value,
) -> napi_status {
    unsafe { give(result, js_sys::Array::new_with_length(length as u32).into()) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_double(
    _env: napi_env,
    value: f64,
    result: *mut napi_value,
) -> napi_status {
    unsafe { give(result, JsValue::from_f64(value)) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_uint32(
    _env: napi_env,
    value: u32,
    result: *mut napi_value,
) -> napi_status {
    unsafe { give(result, JsValue::from(value)) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_bigint_int64(
    _env: napi_env,
    value: i64,
    result: *mut napi_value,
) -> napi_status {
    unsafe { give(result, JsValue::from(value)) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_bigint_uint64(
    _env: napi_env,
    value: u64,
    result: *mut napi_value,
) -> napi_status {
    unsafe { give(result, JsValue::from(value)) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_string_utf8(
    _env: napi_env,
    string: *const c_char,
    length: usize,
    result: *mut napi_value,
) -> napi_status {
    match unsafe { text(string, length) } {
        Some(string) => unsafe { give(result, JsValue::from_str(&string)) },
        None => napi_invalid_arg,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_typeof(
    _env: napi_env,
    value: napi_value,
    result: *mut napi_valuetype,
) -> napi_status {
    let value = arg!(value);
    let kind = if value.is_undefined() {
        napi_valuetype::napi_undefined
    } else if value.is_null() {
        napi_valuetype::napi_null
    } else if value.as_bool().is_some() {
        napi_valuetype::napi_boolean
    } else if value.as_f64().is_some() {
        napi_valuetype::napi_number
    } else if value.is_string() {
        napi_valuetype::napi_string
    } else if value.is_symbol() {
        napi_valuetype::napi_symbol
    } else if value.is_bigint() {
        napi_valuetype::napi_bigint
    } else if value.is_function() {
        napi_valuetype::napi_function
    } else if is_external(&value) {
        napi_valuetype::napi_external
    } else {
        napi_valuetype::napi_object
    };
    out!(result, kind)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_value_bool(
    _env: napi_env,
    value: napi_value,
    result: *mut bool,
) -> napi_status {
    match arg!(value).as_bool() {
        Some(boolean) => out!(result, boolean),
        None => napi_boolean_expected,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_value_double(
    _env: napi_env,
    value: napi_value,
    result: *mut f64,
) -> napi_status {
    match arg!(value).as_f64() {
        Some(number) => out!(result, number),
        None => napi_number_expected,
    }
}

/// A number as ECMAScript's ToUint32 converts it, as Node.js does.
fn to_uint32(number: f64) -> u32 {
    if !number.is_finite() {
        return 0;
    }
    number.trunc().rem_euclid(4_294_967_296.0) as u32
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_value_int32(
    _env: napi_env,
    value: napi_value,
    result: *mut i32,
) -> napi_status {
    match arg!(value).as_f64() {
        Some(number) => out!(result, to_uint32(number) as i32),
        None => napi_number_expected,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_value_uint32(
    _env: napi_env,
    value: napi_value,
    result: *mut u32,
) -> napi_status {
    match arg!(value).as_f64() {
        Some(number) => out!(result, to_uint32(number)),
        None => napi_number_expected,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_value_bigint_int64(
    _env: napi_env,
    value: napi_value,
    result: *mut i64,
    lossless: *mut bool,
) -> napi_status {
    let value = arg!(value);
    if !value.is_bigint() {
        return napi_bigint_expected;
    }
    if lossless.is_null() {
        return napi_invalid_arg;
    }
    // SAFETY: the caller's pointer.
    unsafe { *lossless = is_lossless_i64(&value) };
    out!(result, as_i64(&value))
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_value_bigint_uint64(
    _env: napi_env,
    value: napi_value,
    result: *mut u64,
    lossless: *mut bool,
) -> napi_status {
    let value = arg!(value);
    if !value.is_bigint() {
        return napi_bigint_expected;
    }
    if lossless.is_null() {
        return napi_invalid_arg;
    }
    // SAFETY: the caller's pointer.
    unsafe { *lossless = is_lossless_u64(&value) };
    out!(result, as_u64(&value))
}

/// Copies the UTF-8 of a string as Node.js does: the length alone without a
/// buffer, and otherwise as many whole characters as fit before the NUL.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_value_string_utf8(
    _env: napi_env,
    value: napi_value,
    buffer: *mut c_char,
    size: usize,
    result: *mut usize,
) -> napi_status {
    let Some(string) = arg!(value).as_string() else {
        return napi_string_expected;
    };
    if buffer.is_null() {
        return out!(result, string.len());
    }
    let mut length = string.len().min(size.saturating_sub(1));
    while !string.is_char_boundary(length) {
        length -= 1;
    }
    if size > 0 {
        // SAFETY: the caller's buffer of `size` bytes.
        unsafe {
            std::ptr::copy_nonoverlapping(string.as_ptr(), buffer.cast::<u8>(), length);
            *buffer.add(length) = 0;
        }
    }
    if !result.is_null() {
        // SAFETY: the caller's pointer.
        unsafe { *result = length };
    }
    napi_ok
}

// Objects and arrays.

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_is_array(
    _env: napi_env,
    value: napi_value,
    result: *mut bool,
) -> napi_status {
    out!(result, js_sys::Array::is_array(&arg!(value)))
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_array_length(
    _env: napi_env,
    value: napi_value,
    result: *mut u32,
) -> napi_status {
    match arg!(value).dyn_into::<js_sys::Array>() {
        Ok(array) => out!(result, array.length()),
        Err(_) => napi_array_expected,
    }
}

fn get(object: &JsValue, key: &JsValue, result: *mut napi_value) -> napi_status {
    if !is_object_like(object) {
        return napi_object_expected;
    }
    if is_pending() {
        return napi_pending_exception;
    }
    // Reflect.get takes objects only, and a primitive's members are its
    // wrapper's, as Node.js reads them.
    match js_sys::Reflect::get(&to_object(object), key) {
        Ok(value) => unsafe { give(result, value) },
        Err(error) => set_pending(error),
    }
}

fn set(object: &JsValue, key: &JsValue, value: &JsValue) -> napi_status {
    if !is_object_like(object) {
        return napi_object_expected;
    }
    if is_pending() {
        return napi_pending_exception;
    }
    match js_sys::Reflect::set(&to_object(object), key, value) {
        Ok(_) => napi_ok,
        Err(error) => set_pending(error),
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_named_property(
    _env: napi_env,
    object: napi_value,
    name: *const c_char,
    result: *mut napi_value,
) -> napi_status {
    let object = arg!(object);
    let Some(name) = (unsafe { text(name, NAPI_AUTO_LENGTH) }) else {
        return napi_invalid_arg;
    };
    get(&object, &JsValue::from_str(&name), result)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_set_named_property(
    _env: napi_env,
    object: napi_value,
    name: *const c_char,
    value: napi_value,
) -> napi_status {
    let (object, value) = (arg!(object), arg!(value));
    let Some(name) = (unsafe { text(name, NAPI_AUTO_LENGTH) }) else {
        return napi_invalid_arg;
    };
    set(&object, &JsValue::from_str(&name), &value)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_element(
    _env: napi_env,
    object: napi_value,
    index: u32,
    result: *mut napi_value,
) -> napi_status {
    get(&arg!(object), &JsValue::from(index), result)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_set_element(
    _env: napi_env,
    object: napi_value,
    index: u32,
    value: napi_value,
) -> napi_status {
    set(&arg!(object), &JsValue::from(index), &arg!(value))
}

// Functions.

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_function(
    _env: napi_env,
    name: *const c_char,
    length: usize,
    callback: napi_callback,
    data: *mut c_void,
    result: *mut napi_value,
) -> napi_status {
    let Some(callback) = callback else {
        return napi_invalid_arg;
    };
    let Some(name) = (unsafe { text(name, length) }) else {
        return napi_invalid_arg;
    };
    let function = make_function(&name, callback as usize as u32, data as u32);
    unsafe { give(result, function.into()) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_cb_info(
    _env: napi_env,
    info: napi_callback_info,
    argc: *mut usize,
    argv: *mut napi_value,
    this_arg: *mut napi_value,
    data: *mut *mut c_void,
) -> napi_status {
    if info.is_null() {
        return napi_invalid_arg;
    }
    // SAFETY: invoke() passes its info for the length of the call.
    let info = unsafe { &*info.cast::<CallbackInfo>() };
    let count = info.arguments.length() as usize;
    if !argv.is_null() {
        if argc.is_null() {
            return napi_invalid_arg;
        }
        // SAFETY: the caller's array of `*argc` values.
        let capacity = unsafe { *argc };
        for index in 0..capacity {
            let argument = if index < count {
                info.arguments.get(index as u32)
            } else {
                JsValue::UNDEFINED
            };
            // SAFETY: as above.
            unsafe { *argv.add(index) = push(argument) };
        }
    }
    if !argc.is_null() {
        // SAFETY: the caller's pointer.
        unsafe { *argc = count };
    }
    if !this_arg.is_null() {
        // SAFETY: the caller's pointer.
        unsafe { *this_arg = push(info.this.clone()) };
    }
    if !data.is_null() {
        // SAFETY: the caller's pointer.
        unsafe { *data = info.data };
    }
    napi_ok
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_call_function(
    _env: napi_env,
    receiver: napi_value,
    function: napi_value,
    argc: usize,
    argv: *const napi_value,
    result: *mut napi_value,
) -> napi_status {
    let receiver = arg!(receiver);
    let Ok(function) = arg!(function).dyn_into::<js_sys::Function>() else {
        return napi_function_expected;
    };
    if is_pending() {
        return napi_pending_exception;
    }
    if argc > 0 && argv.is_null() {
        return napi_invalid_arg;
    }
    let arguments = js_sys::Array::new_with_length(argc as u32);
    for index in 0..argc {
        // SAFETY: the caller's array of `argc` values.
        let argument = arg!(unsafe { *argv.add(index) });
        arguments.set(index as u32, argument);
    }
    match function.apply(&receiver, &arguments) {
        Ok(value) => {
            if !result.is_null() {
                // SAFETY: the caller's pointer.
                unsafe { *result = push(value) };
            }
            napi_ok
        }
        Err(error) => set_pending(error),
    }
}

// Exceptions.

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_throw(_env: napi_env, error: napi_value) -> napi_status {
    set_pending(arg!(error));
    napi_ok
}

unsafe fn throw(kind: u32, code: *const c_char, message: *const c_char) -> napi_status {
    let code = unsafe { text(code, NAPI_AUTO_LENGTH) }
        .filter(|_| !code.is_null())
        .map(String::from);
    let Some(message) = (unsafe { text(message, NAPI_AUTO_LENGTH) }) else {
        return napi_invalid_arg;
    };
    set_pending(make_error(kind, &message, code));
    napi_ok
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_throw_error(
    _env: napi_env,
    code: *const c_char,
    message: *const c_char,
) -> napi_status {
    unsafe { throw(0, code, message) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_throw_type_error(
    _env: napi_env,
    code: *const c_char,
    message: *const c_char,
) -> napi_status {
    unsafe { throw(1, code, message) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_throw_range_error(
    _env: napi_env,
    code: *const c_char,
    message: *const c_char,
) -> napi_status {
    unsafe { throw(2, code, message) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_is_exception_pending(
    _env: napi_env,
    result: *mut bool,
) -> napi_status {
    out!(result, is_pending())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_and_clear_last_exception(
    _env: napi_env,
    result: *mut napi_value,
) -> napi_status {
    let error = with(|env| env.pending.take()).unwrap_or(JsValue::UNDEFINED);
    unsafe { give(result, error) }
}

/// Reports `error` as the page reports an error that nothing caught.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_fatal_exception(_env: napi_env, error: napi_value) -> napi_status {
    report_error(&arg!(error));
    napi_ok
}

// References.

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_reference(
    _env: napi_env,
    value: napi_value,
    count: u32,
    result: *mut napi_ref,
) -> napi_status {
    let value = arg!(value);
    if result.is_null() {
        return napi_invalid_arg;
    }
    let weakly = count == 0 && (value.is_object() || value.is_function());
    let reference = if weakly {
        Reference::Weak(weak_ref(&value))
    } else {
        Reference::Strong(value)
    };
    let slot = with(|env| {
        let entry = Some((reference, count));
        if let Some(index) = env.free_references.pop() {
            env.references[index] = entry;
            index
        } else {
            env.references.push(entry);
            env.references.len() - 1
        }
    });
    out!(result, (slot + 1) as napi_ref)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_delete_reference(_env: napi_env, reference: napi_ref) -> napi_status {
    let Some(index) = (reference as usize).checked_sub(1) else {
        return napi_invalid_arg;
    };
    let removed = with(|env| {
        let removed = env.references.get_mut(index).and_then(Option::take);
        if removed.is_some() {
            env.free_references.push(index);
        }
        removed
    });
    if removed.is_some() {
        napi_ok
    } else {
        napi_invalid_arg
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_reference_value(
    _env: napi_env,
    reference: napi_ref,
    result: *mut napi_value,
) -> napi_status {
    let Some(index) = (reference as usize).checked_sub(1) else {
        return napi_invalid_arg;
    };
    let referent = with(|env| match env.references.get(index) {
        Some(Some((Reference::Strong(value), _))) => Some(Ok(value.clone())),
        Some(Some((Reference::Weak(weak), _))) => Some(Err(weak.clone())),
        _ => None,
    });
    let value = match referent {
        Some(Ok(value)) => value,
        Some(Err(weak)) => {
            let value = deref(&weak);
            if value.is_undefined() {
                // A collected referent gives no value.
                return out!(result, std::ptr::null_mut());
            }
            value
        }
        None => return napi_invalid_arg,
    };
    unsafe { give(result, value) }
}

// Externals and instance data.

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_external(
    _env: napi_env,
    data: *mut c_void,
    finalize: napi_finalize,
    hint: *mut c_void,
    result: *mut napi_value,
) -> napi_status {
    let callback = finalize.map_or(0, |finalize| finalize as usize as u32);
    unsafe { give(result, make_external(data as u32, callback, hint as u32)) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_value_external(
    _env: napi_env,
    value: napi_value,
    result: *mut *mut c_void,
) -> napi_status {
    match external_data(&arg!(value)) {
        Some(data) => out!(result, data as *mut c_void),
        None => napi_invalid_arg,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_set_instance_data(
    _env: napi_env,
    data: *mut c_void,
    _finalize: napi_finalize,
    _hint: *mut c_void,
) -> napi_status {
    with(|env| env.instance_data = data);
    napi_ok
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_instance_data(
    _env: napi_env,
    data: *mut *mut c_void,
) -> napi_status {
    out!(data, with(|env| env.instance_data))
}

/// Takes the hook, which never runs: the environment ends with the page.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_add_env_cleanup_hook(
    _env: napi_env,
    _hook: napi_cleanup_hook,
    _argument: *mut c_void,
) -> napi_status {
    napi_ok
}

// Typed arrays and buffers.

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_is_typedarray(
    _env: napi_env,
    value: napi_value,
    result: *mut bool,
) -> napi_status {
    out!(result, is_typed_array(&arg!(value)))
}

/// Gives the typed array's facts, and for `data` a copy of its bytes that
/// lives until the current handle scope closes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_get_typedarray_info(
    _env: napi_env,
    value: napi_value,
    kind: *mut i32,
    length: *mut usize,
    data: *mut *mut c_void,
    arraybuffer: *mut napi_value,
    byte_offset: *mut usize,
) -> napi_status {
    let value = arg!(value);
    if !is_typed_array(&value) {
        return napi_invalid_arg;
    }
    let array = value.unchecked_ref::<js_sys::Uint8Array>();
    if !kind.is_null() {
        // SAFETY: the caller's pointer.
        unsafe { *kind = typed_array_type(&value) };
    }
    if !length.is_null() {
        // SAFETY: the caller's pointer.
        unsafe { *length = array.length() as usize };
    }
    if !data.is_null() {
        let bytes = js_sys::Uint8Array::new_with_byte_offset_and_length(
            &array.buffer(),
            array.byte_offset(),
            array.byte_length(),
        );
        let size = bytes.length() as usize;
        let Some(block) = Block::new(size, with(|env| env.scopes.len()), None) else {
            return napi_generic_failure;
        };
        // SAFETY: the block holds `size` bytes.
        bytes.copy_to(unsafe { std::slice::from_raw_parts_mut(block.pointer, size) });
        // SAFETY: the caller's pointer.
        unsafe { *data = block.pointer.cast() };
        with(|env| env.blocks.push(block));
    }
    if !arraybuffer.is_null() {
        // SAFETY: the caller's pointer.
        unsafe { *arraybuffer = push(array.buffer().into()) };
    }
    if !byte_offset.is_null() {
        // SAFETY: the caller's pointer.
        unsafe { *byte_offset = array.byte_offset() as usize };
    }
    napi_ok
}

/// Makes a buffer, and for `data` a zeroed block of the module's memory that
/// `mln_napi_sync_arraybuffer()` writes into it, which lives until the current
/// handle scope closes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_arraybuffer(
    _env: napi_env,
    byte_length: usize,
    data: *mut *mut c_void,
    result: *mut napi_value,
) -> napi_status {
    if result.is_null() {
        return napi_invalid_arg;
    }
    let buffer: JsValue = js_sys::ArrayBuffer::new(byte_length as u32).into();
    if !data.is_null() {
        let Some(block) = Block::new(
            byte_length,
            with(|env| env.scopes.len()),
            Some(buffer.clone()),
        ) else {
            return napi_generic_failure;
        };
        // SAFETY: the caller's pointer.
        unsafe { *data = block.pointer.cast() };
        with(|env| env.blocks.push(block));
    }
    unsafe { give(result, buffer) }
}

/// Writes the block that `napi_create_arraybuffer()` gave out for
/// `arraybuffer` into the buffer, and frees it.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_napi_sync_arraybuffer(
    _env: napi_env,
    arraybuffer: napi_value,
) -> napi_status {
    let buffer = arg!(arraybuffer);
    let block = with(|env| {
        let index = env.blocks.iter().position(|block| {
            block
                .buffer
                .as_ref()
                .is_some_and(|candidate| js_sys::Object::is(candidate, &buffer))
        })?;
        Some(env.blocks.remove(index))
    });
    let Some(block) = block else {
        return napi_invalid_arg;
    };
    let target = js_sys::Uint8Array::new(&buffer);
    let size = target.length() as usize;
    // SAFETY: the block holds at least the buffer's bytes.
    target.copy_from(unsafe { std::slice::from_raw_parts(block.pointer, size) });
    napi_ok
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_typedarray(
    _env: napi_env,
    kind: i32,
    length: usize,
    arraybuffer: napi_value,
    byte_offset: usize,
    result: *mut napi_value,
) -> napi_status {
    let buffer = arg!(arraybuffer);
    if !buffer.is_instance_of::<js_sys::ArrayBuffer>() {
        return napi_invalid_arg;
    }
    match make_typed_array(kind, &buffer, byte_offset as f64, length as f64) {
        Ok(array) => unsafe { give(result, array) },
        Err(error) => set_pending(error),
    }
}

// Threadsafe functions.

struct ThreadsafeFunction {
    function: Option<JsValue>,
    context: *mut c_void,
    call_js: napi_threadsafe_function_call_js,
    finalize: napi_finalize,
    finalize_data: *mut c_void,
    queue: VecDeque<*mut c_void>,
    threads: usize,
    closing: bool,
    aborted: bool,
    scheduled: bool,
}

type Shared = RefCell<ThreadsafeFunction>;

fn threadsafe<'a>(function: napi_threadsafe_function) -> &'a Shared {
    // SAFETY: napi_create_threadsafe_function() made it, and only drain()
    // frees it, once nothing can call it.
    unsafe { &*function.cast::<Shared>() }
}

fn request_drain(function: napi_threadsafe_function) {
    let shared = threadsafe(function);
    let mut state = shared.borrow_mut();
    if !state.scheduled {
        state.scheduled = true;
        drop(state);
        schedule(function as u32);
    }
}

/// Runs a threadsafe function's queued calls, and finalizes it once it is
/// closing and they have all run.
fn drain(function: u32) {
    let function = function as napi_threadsafe_function;
    let shared = threadsafe(function);
    shared.borrow_mut().scheduled = false;
    loop {
        let (next, aborted, call_js, js_function, context) = {
            let mut state = shared.borrow_mut();
            (
                state.queue.pop_front(),
                state.aborted,
                state.call_js,
                state.function.clone(),
                state.context,
            )
        };
        let Some(data) = next else { break };
        if aborted {
            // A torn-down function hands each call its data to free, with no
            // environment.
            if let Some(call_js) = call_js {
                // SAFETY: the C callback's ABI.
                unsafe { call_js(std::ptr::null_mut(), std::ptr::null_mut(), context, data) };
            }
            continue;
        }
        in_scope(|| {
            let callback = js_function.clone().map_or(std::ptr::null_mut(), push);
            match call_js {
                // SAFETY: the C callback's ABI.
                Some(call_js) => unsafe { call_js(env(), callback, context, data) },
                None => {
                    if let Some(Ok(function)) =
                        js_function.map(JsCast::dyn_into::<js_sys::Function>)
                        && let Err(error) = function.call0(&JsValue::UNDEFINED)
                    {
                        set_pending(error);
                    }
                }
            }
        });
    }
    let finalize = {
        let state = shared.borrow();
        (state.closing && state.queue.is_empty() && !state.scheduled).then_some((
            state.finalize,
            state.finalize_data,
            state.context,
        ))
    };
    if let Some((finalize, data, context)) = finalize {
        if let Some(finalize) = finalize {
            // SAFETY: the C finalizer's ABI.
            in_scope(|| unsafe { finalize(env(), data, context) });
        }
        // SAFETY: made by Box::into_raw, and nothing calls it now.
        drop(unsafe { Box::from_raw(function.cast::<Shared>()) });
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_create_threadsafe_function(
    _env: napi_env,
    function: napi_value,
    _async_resource: napi_value,
    _async_resource_name: napi_value,
    _max_queue_size: usize,
    initial_thread_count: usize,
    thread_finalize_data: *mut c_void,
    thread_finalize: napi_finalize,
    context: *mut c_void,
    call_js: napi_threadsafe_function_call_js,
    result: *mut napi_threadsafe_function,
) -> napi_status {
    let function = if function.is_null() {
        None
    } else {
        Some(arg!(function))
    };
    if result.is_null() || initial_thread_count == 0 || (function.is_none() && call_js.is_none()) {
        return napi_invalid_arg;
    }
    let shared: Box<Shared> = Box::new(RefCell::new(ThreadsafeFunction {
        function,
        context,
        call_js,
        finalize: thread_finalize,
        finalize_data: thread_finalize_data,
        queue: VecDeque::new(),
        threads: initial_thread_count,
        closing: false,
        aborted: false,
        scheduled: false,
    }));
    out!(result, Box::into_raw(shared).cast())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_call_threadsafe_function(
    function: napi_threadsafe_function,
    data: *mut c_void,
    _mode: i32,
) -> napi_status {
    if function.is_null() {
        return napi_invalid_arg;
    }
    {
        let mut state = threadsafe(function).borrow_mut();
        if state.closing {
            return napi_closing;
        }
        state.queue.push_back(data);
    }
    request_drain(function);
    napi_ok
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_release_threadsafe_function(
    function: napi_threadsafe_function,
    mode: i32,
) -> napi_status {
    if function.is_null() {
        return napi_invalid_arg;
    }
    {
        let mut state = threadsafe(function).borrow_mut();
        if state.closing {
            return napi_closing;
        }
        if mode == NAPI_TSFN_ABORT {
            state.aborted = true;
            state.closing = true;
        } else {
            state.threads = state.threads.saturating_sub(1);
            state.closing = state.threads == 0;
        }
        if !state.closing {
            return napi_ok;
        }
    }
    request_drain(function);
    napi_ok
}

/// Does nothing: no event loop waits on the module's functions.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_ref_threadsafe_function(
    _env: napi_env,
    _function: napi_threadsafe_function,
) -> napi_status {
    napi_ok
}

/// Does nothing, as `napi_ref_threadsafe_function()` does.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn napi_unref_threadsafe_function(
    _env: napi_env,
    _function: napi_threadsafe_function,
) -> napi_status {
    napi_ok
}
