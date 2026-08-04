//! The Node-API transport for the TypeScript binding.
//!
//! This addon is the whole native boundary: it reports the ABI handshake, turns
//! JavaScript-owned `ArrayBuffer` slabs into addresses the C API can use, calls
//! entrypoints through the shared normalized dispatch, and copies bytes out of
//! library-owned memory. Everything above it — struct encoding, handle state,
//! error mapping, the public API — is TypeScript shared with the WebAssembly
//! transport.
//!
//! Addresses cross this boundary as `bigint`. A JavaScript `number` cannot carry
//! one: Android tags heap pointers in the top byte, so a pointer converted
//! through a double loses bits the allocator requires.

#![deny(clippy::all)]

use std::ffi::CStr;
use std::os::raw::c_char;

use napi::bindgen_prelude::*;
use napi::threadsafe_function::{ThreadsafeFunction, ThreadsafeFunctionCallMode};
use napi_derive::napi;

unsafe extern "C" {
    fn mln_abi_fingerprint() -> *const c_char;
    fn mln_abi_header_digest() -> *const c_char;
    fn mln_abi_entrypoint_count() -> u32;
    fn mln_abi_entrypoint_name(entrypoint: u32) -> *const c_char;
    fn mln_abi_call(
        entrypoint: u32,
        slots: *mut std::ffi::c_void,
        diagnostic: *mut c_char,
        diagnostic_capacity: u32,
        diagnostic_length: *mut u32,
    ) -> i32;
    fn mln_abi_symbol(entrypoint: u32) -> *mut std::ffi::c_void;
    fn mln_abi_log_listener(listener_data: *mut std::ffi::c_void, record: *mut std::ffi::c_void);
    fn mln_abi_resource_request_listener(
        listener_data: *mut std::ffi::c_void,
        request: *mut std::ffi::c_void,
    );
    fn mln_abi_custom_geometry_fetch_listener_address() -> *mut std::ffi::c_void;
    fn mln_abi_custom_geometry_cancel_listener_address() -> *mut std::ffi::c_void;
    fn mln_abi_record_destroy(kind: u32, record: *mut std::ffi::c_void);
    fn mln_abi_owner_create() -> u64;
    fn mln_abi_owner_destroy(owner: u64);
    fn mln_abi_owner_register(owner: u64) -> u64;
    fn mln_abi_queue_set_notifier(
        owner: u64,
        notify: Option<unsafe extern "C" fn(*mut std::ffi::c_void)>,
        user_data: *mut std::ffi::c_void,
    );
    fn mln_abi_queue_drain(owner: u64, records: *mut std::ffi::c_void, capacity: u32) -> u32;
    fn mln_abi_queue_depth(owner: u64) -> u32;
    fn mln_abi_transfer_issue(handle: u64) -> u64;
    fn mln_abi_transfer_claim(token: u64) -> u64;
    fn mln_abi_transfer_discard(token: u64) -> u64;
}

fn c_string(pointer: *const c_char) -> Option<String> {
    if pointer.is_null() {
        return None;
    }
    Some(
        unsafe { CStr::from_ptr(pointer) }
            .to_string_lossy()
            .into_owned(),
    )
}

/// Reads an address out of a JavaScript `BigInt`.
///
/// A value that does not fit an unsigned 64-bit integer is a caller error rather
/// than something to truncate, because the result would name another address.
fn address(value: BigInt, what: &str) -> Result<u64> {
    let (signed, words, lossless) = (value.sign_bit, value.words, true);
    if signed || words.len() > 1 && words[1..].iter().any(|word| *word != 0) || !lossless {
        return Err(Error::new(
            Status::InvalidArg,
            format!("{what} is not an unsigned 64-bit address"),
        ));
    }
    Ok(words.first().copied().unwrap_or(0))
}

/// Reports the ABI schema fingerprint this payload's dispatch was built from.
#[napi]
pub fn abi_fingerprint() -> String {
    c_string(unsafe { mln_abi_fingerprint() }).unwrap_or_default()
}

/// Reports the digest of the public headers this payload was built against.
#[napi]
pub fn abi_header_digest() -> String {
    c_string(unsafe { mln_abi_header_digest() }).unwrap_or_default()
}

/// Reports how many entrypoints this payload dispatches.
#[napi]
pub fn entrypoint_count() -> u32 {
    unsafe { mln_abi_entrypoint_count() }
}

/// Names an entrypoint, so a failure can say which call it came from.
#[napi]
pub fn entrypoint_name(entrypoint: u32) -> Option<String> {
    c_string(unsafe { mln_abi_entrypoint_name(entrypoint) })
}

/// Reports the address of a slab's backing store.
///
/// The caller keeps the `ArrayBuffer` reachable for as long as the address is in
/// use. Backing stores do not move, so the address stays valid for the buffer's
/// life.
#[napi]
pub fn register_slab(buffer: ArrayBuffer) -> BigInt {
    BigInt::from(buffer.as_ref().as_ptr() as usize as u64)
}

/// Calls one entrypoint through the shared normalized dispatch.
///
/// `slots` and `diagnostic` are addresses inside slabs the caller owns. The
/// diagnostic is copied inside the failing call, so nothing the host does
/// afterwards can replace it.
#[napi]
pub fn call(
    entrypoint: u32,
    slots: BigInt,
    diagnostic: BigInt,
    diagnostic_capacity: u32,
    diagnostic_length: BigInt,
) -> Result<i32> {
    let slots = address(slots, "slots")? as usize as *mut std::ffi::c_void;
    let diagnostic = address(diagnostic, "diagnostic")? as usize as *mut c_char;
    let diagnostic_length = address(diagnostic_length, "diagnosticLength")? as usize as *mut u32;
    Ok(unsafe {
        mln_abi_call(
            entrypoint,
            slots,
            diagnostic,
            diagnostic_capacity,
            diagnostic_length,
        )
    })
}

/// Reports the address of an entrypoint, for a struct field the C API reads as
/// a callback.
#[napi]
pub fn symbol(entrypoint: u32) -> BigInt {
    BigInt::from(unsafe { mln_abi_symbol(entrypoint) } as usize as u64)
}

/// Copies bytes out of library-owned memory.
///
/// Library allocations sit outside every slab, so the shared layer cannot read
/// them through a view. This is the copy step the binding specification requires
/// before a borrowed native pointer's window ends.
#[napi]
pub fn read_foreign(pointer: BigInt, length: u32) -> Result<Uint8Array> {
    let pointer = address(pointer, "pointer")?;
    if pointer == 0 {
        return Err(Error::new(
            Status::InvalidArg,
            "cannot read from a null pointer".to_owned(),
        ));
    }
    let bytes =
        unsafe { std::slice::from_raw_parts(pointer as usize as *const u8, length as usize) };
    Ok(Uint8Array::new(bytes.to_vec()))
}

/// Copies a null-terminated library-owned string.
#[napi]
pub fn read_foreign_c_string(pointer: BigInt) -> Result<Option<String>> {
    let pointer = address(pointer, "pointer")?;
    Ok(c_string(pointer as usize as *const c_char))
}

/// Issues a one-shot token naming a handle, for moving it to another context.
#[napi]
pub fn transfer_issue(handle: BigInt) -> Result<BigInt> {
    Ok(BigInt::from(unsafe {
        mln_abi_transfer_issue(address(handle, "handle")?)
    }))
}

/// Claims a transfer token, reporting the handle it named.
#[napi]
pub fn transfer_claim(token: BigInt) -> Result<BigInt> {
    Ok(BigInt::from(unsafe {
        mln_abi_transfer_claim(address(token, "token")?)
    }))
}

/// Discards an unclaimed transfer token.
#[napi]
pub fn transfer_discard(token: BigInt) -> Result<BigInt> {
    Ok(BigInt::from(unsafe {
        mln_abi_transfer_discard(address(token, "token")?)
    }))
}

/// The addresses of the listeners a registration installs.
///
/// The C API stores these in a struct field, so the host needs their addresses
/// rather than a way to call them.
#[napi]
pub fn listener_address(kind: u32) -> BigInt {
    let address = match kind {
        1 => mln_abi_log_listener as *const () as usize,
        2 => mln_abi_resource_request_listener as *const () as usize,
        3 => (unsafe { mln_abi_custom_geometry_fetch_listener_address() }) as usize,
        4 => (unsafe { mln_abi_custom_geometry_cancel_listener_address() }) as usize,
        _ => 0,
    };
    BigInt::from(address as u64)
}

/// Releases a record a drain delivered.
///
/// Each family owns its records differently, and the host does not have to know
/// which: naming the kind is enough.
#[napi]
pub fn destroy_record(kind: u32, record: BigInt) -> Result<()> {
    let record = address(record, "record")? as usize as *mut std::ffi::c_void;
    unsafe { mln_abi_record_destroy(kind, record) };
    Ok(())
}

/// Creates the owner one JavaScript realm's callback registry registers under.
///
/// Every realm in a process shares this one loaded library, so an identity a
/// realm chose for itself would collide with the identity another realm chose.
/// Owners come from the shared layer for that reason, and the registration
/// identities minted under them are unique across the whole process.
#[napi]
pub fn create_callback_owner() -> BigInt {
    BigInt::from(unsafe { mln_abi_owner_create() })
}

/// Destroys an owner, releasing records still queued for it.
#[napi]
pub fn destroy_callback_owner(owner: BigInt) -> Result<()> {
    unsafe { mln_abi_owner_destroy(address(owner, "owner")?) };
    Ok(())
}

/// Reserves a registration identity belonging to an owner.
#[napi]
pub fn register_callback(owner: BigInt) -> Result<BigInt> {
    Ok(BigInt::from(unsafe {
        mln_abi_owner_register(address(owner, "owner")?)
    }))
}

/// Drains an owner's queued callback records into host storage.
#[napi]
pub fn drain_records(owner: BigInt, records: BigInt, capacity: u32) -> Result<u32> {
    let owner = address(owner, "owner")?;
    let records = address(records, "records")? as usize as *mut std::ffi::c_void;
    if records.is_null() {
        return Err(Error::new(
            Status::InvalidArg,
            "records must name host storage".to_owned(),
        ));
    }
    Ok(unsafe { mln_abi_queue_drain(owner, records, capacity) })
}

/// Reports how many of an owner's records are waiting.
#[napi]
pub fn record_depth(owner: BigInt) -> Result<u32> {
    Ok(unsafe { mln_abi_queue_depth(address(owner, "owner")?) })
}

/// Wakes the JavaScript context when a record is queued.
///
/// MapLibre produces records on its own threads, so the notifier cannot run
/// user code. It signals a non-blocking thread-safe function, and the callback
/// that function runs on the JavaScript context drains the queue.
struct RecordNotifier {
    callback: ThreadsafeFunction<(), (), (), Status, false>,
}

unsafe extern "C" fn notify_records(user_data: *mut std::ffi::c_void) {
    if user_data.is_null() {
        return;
    }
    let notifier = unsafe { &*(user_data as *const RecordNotifier) };
    // Non-blocking: a producer thread never waits on the JavaScript context,
    // and a full queue coalesces into the drain that is already scheduled.
    notifier
        .callback
        .call((), ThreadsafeFunctionCallMode::NonBlocking);
}

/// The notifier each owner installed.
///
/// This library is loaded once per process and every JavaScript realm in it
/// shares these statics, so one slot would mean the realm that starts
/// notifications last silences the ones before it. Owners are one per registry
/// and few, so the list is walked rather than indexed, and a `Vec` is what a
/// static can be built from without a lazy initializer.
static NOTIFIERS: std::sync::Mutex<Vec<(u64, Box<RecordNotifier>)>> =
    std::sync::Mutex::new(Vec::new());

/// Installs the drain signal for one owner.
#[napi]
pub fn start_record_notifications(
    owner: BigInt,
    callback: ThreadsafeFunction<(), (), (), Status, false>,
) -> Result<()> {
    let owner = address(owner, "owner")?;
    let notifier = Box::new(RecordNotifier { callback });
    // The box owns the heap allocation the address names, so moving the box
    // into the list afterwards leaves the address valid.
    let pointer = (&*notifier as *const RecordNotifier) as *mut std::ffi::c_void;
    let mut notifiers = NOTIFIERS.lock().unwrap();
    // This owner's previous notifier is cleared from native first, so no
    // producer can be inside it while it is dropped.
    unsafe { mln_abi_queue_set_notifier(owner, None, std::ptr::null_mut()) };
    notifiers.retain(|(installed, _)| *installed != owner);
    notifiers.push((owner, notifier));
    unsafe { mln_abi_queue_set_notifier(owner, Some(notify_records), pointer) };
    Ok(())
}

/// Removes one owner's drain signal, leaving its records for an explicit drain.
#[napi]
pub fn stop_record_notifications(owner: BigInt) -> Result<()> {
    let owner = address(owner, "owner")?;
    let mut notifiers = NOTIFIERS.lock().unwrap();
    unsafe { mln_abi_queue_set_notifier(owner, None, std::ptr::null_mut()) };
    notifiers.retain(|(installed, _)| *installed != owner);
    Ok(())
}

/// Registration for the ArkTS runtime, whose module loader differs.
///
/// Node, Bun, and Deno call `napi_register_module_v1` on a library they have
/// loaded, so nothing else is needed there. The ArkTS runtime instead resolves
/// `@app:<bundle>/<module>/<name>` against a registry that every native module
/// adds itself to while loading, and reports a module with no exports when the
/// name is absent. This adds the entry, and delegates to the registration
/// napi-rs already generates.
#[cfg(target_env = "ohos")]
mod ohos {
    use std::ffi::c_void;
    use std::os::raw::c_char;

    use napi::sys::{napi_env, napi_value};

    unsafe extern "C" {
        fn napi_register_module_v1(env: napi_env, exports: napi_value) -> napi_value;
        fn napi_module_register(module: *mut NapiModule);
    }

    #[repr(C)]
    struct NapiModule {
        nm_version: i32,
        nm_flags: u32,
        nm_filename: *const c_char,
        nm_register_func:
            Option<unsafe extern "C" fn(env: napi_env, exports: napi_value) -> napi_value>,
        nm_modname: *const c_char,
        nm_priv: *mut c_void,
        reserved: [*mut c_void; 4],
    }

    // The registry keeps the pointer it is given, so the module outlives the
    // call that registers it.
    unsafe impl Sync for NapiModule {}

    unsafe extern "C" fn register(env: napi_env, exports: napi_value) -> napi_value {
        unsafe { napi_register_module_v1(env, exports) }
    }

    /// The name the application imports, which is the library's own without the
    /// `lib` prefix and `.so` suffix.
    const MODULE_NAME: &[u8] = b"maplibre-native-ffi\0";
    const FILE_NAME: &[u8] = b"libmaplibre-native-ffi.so\0";

    static mut MODULE: NapiModule = NapiModule {
        nm_version: 1,
        nm_flags: 0,
        nm_filename: FILE_NAME.as_ptr().cast(),
        nm_register_func: Some(register),
        nm_modname: MODULE_NAME.as_ptr().cast(),
        nm_priv: std::ptr::null_mut(),
        reserved: [std::ptr::null_mut(); 4],
    };

    unsafe extern "C" fn register_module() {
        unsafe { napi_module_register(&raw mut MODULE) };
    }

    // Registration has to happen as the library loads, before the runtime looks
    // the name up, which is what the constructor section is for.
    #[used]
    #[unsafe(link_section = ".init_array")]
    static REGISTER: unsafe extern "C" fn() = register_module;
}
