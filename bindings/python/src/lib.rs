#![deny(unsafe_op_in_unsafe_fn)]

use maplibre_native_ffi_core::{self as maplibre_core, Error, ErrorKind};
use maplibre_native_ffi_sys as sys;
use pyo3::prelude::*;
use pyo3::types::{PyAny, PyBytes, PyDict, PyList};
use std::ffi::{c_char, c_void};
use std::mem::ManuallyDrop;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr;
use std::sync::atomic::Ordering;
use std::sync::{Arc, Mutex, MutexGuard};

include!("value_support.rs");
include!("call_support.rs");
include!("generated_operations.rs");

mod py_errors {
    pyo3::import_exception!(maplibre_native_ffi.errors, BusyError);
    pyo3::import_exception!(maplibre_native_ffi.errors, CancelledError);
    pyo3::import_exception!(maplibre_native_ffi.errors, InvalidArgumentError);
    pyo3::import_exception!(maplibre_native_ffi.errors, InvalidStateError);
    pyo3::import_exception!(maplibre_native_ffi.errors, NativeError);
    pyo3::import_exception!(maplibre_native_ffi.errors, NotFoundError);
    pyo3::import_exception!(maplibre_native_ffi.errors, NotReadyError);
    pyo3::import_exception!(maplibre_native_ffi.errors, TargetLostError);
    pyo3::import_exception!(maplibre_native_ffi.errors, UnknownStatusError);
    pyo3::import_exception!(maplibre_native_ffi.errors, UnsupportedFeatureError);
    pyo3::import_exception!(maplibre_native_ffi.errors, WrongThreadError);
}

#[derive(Debug)]
struct NativeHandleState<T: maplibre_core::handle::NativeHandle> {
    views_valid: bool,
    active_reads: usize,
    callback_roots: Vec<std::sync::Weak<GeneratedCallbackRoot>>,
    id: Option<std::num::NonZeroU64>,
    // The handle ID the native API issued, kept after close so a closed owner
    // still matches the IDs that earlier runtime events reported.
    issued_id: std::num::NonZeroU64,
    closing: bool,
    _typed_handle: std::marker::PhantomData<fn() -> T>,
    dispose_abandoned: Option<unsafe extern "C" fn(T) -> sys::mln_status>,
    /// The native type name, which a leak report carries.
    type_name: &'static str,
    /// The public type name, which a lifecycle error names.
    owner_name: &'static str,
}

impl<T: maplibre_core::handle::NativeHandle> NativeHandleState<T> {
    /// Takes ownership of a native handle.
    ///
    /// # Safety
    ///
    /// `handle` must be a live handle of the matching native type owned by the
    /// caller, which must later close this state with the matching C API
    /// destroy function.
    unsafe fn from_handle(
        handle: T,
        type_name: &'static str,
        owner_name: &'static str,
    ) -> Result<Self, Error> {
        let Some(id) = std::num::NonZeroU64::new(handle.to_raw()) else {
            return Err(maplibre_core::ptr::null_handle_error(type_name));
        };
        Ok(Self {
            views_valid: true,
            active_reads: 0,
            callback_roots: Vec::new(),
            id: Some(id),
            issued_id: id,
            closing: false,
            _typed_handle: std::marker::PhantomData,
            dispose_abandoned: None,
            type_name,
            owner_name,
        })
    }

    fn retain_callback_roots(&mut self, roots: Vec<std::sync::Weak<GeneratedCallbackRoot>>) {
        self.callback_roots.retain(|root| root.strong_count() != 0);
        self.callback_roots.extend(roots);
    }

    fn with_callback_roots(mut self, roots: Vec<std::sync::Weak<GeneratedCallbackRoot>>) -> Self {
        self.callback_roots = roots;
        self
    }

    fn traverse_callbacks(
        &self,
        visit: &pyo3::gc::PyVisit<'_>,
    ) -> Result<(), pyo3::gc::PyTraverseError> {
        for root in self
            .callback_roots
            .iter()
            .filter_map(std::sync::Weak::upgrade)
        {
            for callback in root.lock().unwrap_or_else(|p| p.into_inner()).iter() {
                visit.call(callback)?;
            }
        }
        Ok(())
    }

    fn take_callbacks(&mut self) -> Vec<Py<PyAny>> {
        let mut callbacks = Vec::new();
        for root in self
            .callback_roots
            .drain(..)
            .filter_map(|root| root.upgrade())
        {
            callbacks.extend(std::mem::take(
                &mut *root.lock().unwrap_or_else(|p| p.into_inner()),
            ));
        }
        callbacks
    }

    fn with_disposal(mut self, dispose: unsafe extern "C" fn(T) -> sys::mln_status) -> Self {
        self.dispose_abandoned = Some(dispose);
        self
    }

    fn live_handle(&self) -> Option<T> {
        self.id.map(|id| T::from_raw(id.get()))
    }

    /// The live handle, or the lifecycle error for a closing or closed owner.
    fn require_live(&self) -> PyResult<T> {
        if self.closing {
            return Err(self.lifecycle_error("is closing"));
        }
        self.live_handle()
            .ok_or_else(|| self.lifecycle_error("is closed"))
    }

    /// The binding-raised error for an owner that is closed, closing, or in
    /// use, which carries no native status.
    fn lifecycle_error(&self, state: &str) -> PyErr {
        invalid_state_error(format!("{} {state}", self.owner_name))
    }

    fn is_closed(&self) -> bool {
        self.id.is_none()
    }

    fn issued_id(&self) -> u64 {
        self.issued_id.get()
    }
}

impl<T: maplibre_core::handle::NativeHandle> Drop for NativeHandleState<T> {
    fn drop(&mut self) {
        let (Some(id), Some(dispose)) = (self.id, self.dispose_abandoned) else {
            return;
        };
        self.id = None;
        let type_name = self.type_name;
        generated_finalize(move || {
            // SAFETY: the state transferred its sole ownership to this closure.
            let status = unsafe { dispose(T::from_raw(id.get())) };
            if status != sys::MLN_STATUS_OK {
                maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
                    type_name,
                    id: id.get(),
                });
            }
        });
    }
}

/// The longest interpreter shutdown waits for its runtimes to retire.
const EXIT_RETIREMENT_TIMEOUT: std::time::Duration = std::time::Duration::from_secs(5);

/// An owner that interpreter shutdown retires. The interpreter finalizes before
/// the C runtime's exit handlers run, so native callbacks into it must stop
/// first, and releasing each runtime and waiting for its completion stops them.
trait ExitOwner: Send + Sync {
    fn is_runtime(&self) -> bool;
    /// Takes the handle from an idle owner, so neither close nor finalization
    /// reaches it again. An owner in a read or close on another thread stays.
    fn take_at_exit(&self) -> Option<u64>;
    fn dispose_at_exit(&self, raw: u64);
}

impl<T: maplibre_core::handle::NativeHandle> ExitOwner for Mutex<NativeHandleState<T>> {
    fn is_runtime(&self) -> bool {
        std::any::TypeId::of::<T>() == std::any::TypeId::of::<sys::mln_runtime>()
    }

    fn take_at_exit(&self) -> Option<u64> {
        let mut state = self.lock().unwrap_or_else(|p| p.into_inner());
        if state.closing || state.active_reads != 0 || state.dispose_abandoned.is_none() {
            return None;
        }
        state.id.take().map(std::num::NonZeroU64::get)
    }

    fn dispose_at_exit(&self, raw: u64) {
        let state = self.lock().unwrap_or_else(|p| p.into_inner());
        let (Some(dispose), type_name) = (state.dispose_abandoned, state.type_name) else {
            return;
        };
        drop(state);
        // SAFETY: take_at_exit transferred sole ownership of the handle here.
        if unsafe { dispose(T::from_raw(raw)) } != sys::MLN_STATUS_OK {
            maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
                type_name,
                id: raw,
            });
        }
    }
}

struct ExitOwners {
    owners: Vec<std::sync::Weak<dyn ExitOwner>>,
    prune_at: usize,
}

static EXIT_OWNERS: Mutex<ExitOwners> = Mutex::new(ExitOwners {
    owners: Vec::new(),
    prune_at: 64,
});

/// Shares a new owner's state and enrolls it for retirement at interpreter
/// shutdown. The first runtime registers the shutdown hook.
fn generated_owner_state<T: maplibre_core::handle::NativeHandle>(
    state: NativeHandleState<T>,
) -> Arc<Mutex<NativeHandleState<T>>> {
    let state = Arc::new(Mutex::new(state));
    let owner: Arc<dyn ExitOwner> = state.clone();
    if owner.is_runtime() {
        static REGISTERED: std::sync::atomic::AtomicBool =
            std::sync::atomic::AtomicBool::new(false);
        if !REGISTERED.swap(true, Ordering::AcqRel) {
            Python::attach(|py| {
                let registered = wrap_pyfunction!(retire_owners_at_exit, py)
                    .and_then(|hook| py.import("atexit")?.call_method1("register", (hook,)));
                if let Err(error) = registered {
                    error.write_unraisable(py, None);
                }
            });
        }
    }
    let mut registry = EXIT_OWNERS.lock().unwrap_or_else(|p| p.into_inner());
    if registry.owners.len() >= registry.prune_at {
        registry.owners.retain(|owner| owner.strong_count() != 0);
        registry.prune_at = (registry.owners.len() * 2).max(64);
    }
    registry.owners.push(Arc::downgrade(&owner));
    state
}

struct ExitRelease {
    retired: Mutex<bool>,
    signal: std::sync::Condvar,
}

unsafe extern "C" fn complete_exit_release(
    user_data: *mut c_void,
    _result: *const sys::mln_completion_result,
) {
    // SAFETY: native owns one reference from accepted release until its
    // release callback.
    let release = unsafe { &*user_data.cast::<ExitRelease>() };
    *release.retired.lock().unwrap_or_else(|p| p.into_inner()) = true;
    release.signal.notify_all();
}

unsafe extern "C" fn release_exit_release(user_data: *mut c_void) {
    // SAFETY: native releases accepted completion state exactly once.
    drop(unsafe { Arc::from_raw(user_data.cast::<ExitRelease>()) });
}

/// Disposes every owner the interpreter still holds, then releases each
/// runtime and waits for its retirement, so no native thread can call into a
/// finalized interpreter.
#[pyfunction]
fn retire_owners_at_exit(py: Python<'_>) {
    let owners: Vec<Arc<dyn ExitOwner>> =
        std::mem::take(&mut EXIT_OWNERS.lock().unwrap_or_else(|p| p.into_inner()).owners)
            .iter()
            .filter_map(std::sync::Weak::upgrade)
            .collect();
    // Native threads finishing this work may need the GIL to deliver results
    // and release callbacks.
    py.detach(move || {
        for owner in owners.iter().filter(|owner| !owner.is_runtime()) {
            if let Some(raw) = owner.take_at_exit() {
                owner.dispose_at_exit(raw);
            }
        }
        let mut releases = Vec::new();
        for owner in owners.iter().filter(|owner| owner.is_runtime()) {
            let Some(raw) = owner.take_at_exit() else {
                continue;
            };
            let release = Arc::new(ExitRelease {
                retired: Mutex::new(false),
                signal: std::sync::Condvar::new(),
            });
            let completion = sys::mln_completion {
                size: std::mem::size_of::<sys::mln_completion>() as u32,
                callback: Some(complete_exit_release),
                user_data: Arc::into_raw(Arc::clone(&release)).cast_mut().cast(),
                release_user_data: Some(release_exit_release),
            };
            let runtime = <sys::mln_runtime as maplibre_core::handle::NativeHandle>::from_raw(raw);
            // SAFETY: take_at_exit transferred sole ownership of the handle.
            let status = unsafe { sys::mln_runtime_release(runtime, &completion, ptr::null_mut()) };
            if status == sys::MLN_STATUS_OK {
                releases.push(release);
            } else {
                // SAFETY: a rejected release retains no completion state.
                drop(unsafe { Arc::from_raw(completion.user_data.cast::<ExitRelease>()) });
                // A pending child keeps release from starting, so retirement
                // proceeds unobserved.
                owner.dispose_at_exit(raw);
            }
        }
        let deadline = std::time::Instant::now() + EXIT_RETIREMENT_TIMEOUT;
        for release in releases {
            let retired = release.retired.lock().unwrap_or_else(|p| p.into_inner());
            let timeout = deadline.saturating_duration_since(std::time::Instant::now());
            let _ = release
                .signal
                .wait_timeout_while(retired, timeout, |retired| !*retired);
        }
    });
}

type PyCompletionConverter =
    Box<dyn FnOnce(Python<'_>, &sys::mln_completion_result) -> PyResult<Py<PyAny>> + Send>;

struct PyCompletionBridge {
    future: Py<PyAny>,
    convert: Mutex<Option<PyCompletionConverter>>,
    accept_error_status: bool,
    discard: Option<unsafe fn(&sys::mln_completion_result)>,
}

fn new_python_future(py: Python<'_>) -> PyResult<Bound<'_, PyAny>> {
    let future = py
        .import("maplibre_native_ffi._future")?
        .getattr("NativeFuture")?
        .call0()?;
    // Native work is already running once its C submission is accepted. Mark
    // the future accordingly so cancel() cannot claim that it stopped the work.
    future.call_method0("set_running_or_notify_cancel")?;
    Ok(future)
}

unsafe extern "C" fn complete_python_future(
    user_data: *mut c_void,
    result: *const sys::mln_completion_result,
) {
    if user_data.is_null() || result.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: native owns the bridge from accepted submission through its
        // release callback. The result is borrowed for this callback.
        let bridge = unsafe { &*user_data.cast::<PyCompletionBridge>() };
        let result = unsafe { &*result };
        let attached = Python::try_attach(|py| {
            let converted = if result.status == sys::MLN_STATUS_OK || bridge.accept_error_status {
                match bridge
                    .convert
                    .lock()
                    .unwrap_or_else(|poisoned| poisoned.into_inner())
                    .take()
                {
                    Some(convert) => convert(py, result),
                    None => Err(invalid_state_error("native completion ran more than once")),
                }
            } else {
                Err(map_error(Error::from_status_and_diagnostic(
                    result.status,
                    copy_completion_diagnostic(result),
                )))
            };
            let call = match converted {
                Ok(value) => bridge.future.bind(py).call_method1("set_result", (value,)),
                Err(error) => bridge
                    .future
                    .bind(py)
                    .call_method1("set_exception", (error.value(py),)),
            };
            if let Err(error) = call {
                error.write_unraisable(py, None);
            }
        });
        if attached.is_none()
            && result.status == sys::MLN_STATUS_OK
            && let Some(discard) = bridge.discard
        {
            unsafe { discard(result) };
        }
    }));
}

unsafe extern "C" fn release_python_future(user_data: *mut c_void) {
    if !user_data.is_null() {
        // SAFETY: native releases accepted completion state exactly once.
        drop(unsafe { Box::from_raw(user_data.cast::<PyCompletionBridge>()) });
    }
}

/// Submits a completion-based call and returns the future it resolves.
///
/// With `accept_error_status`, `convert` also receives a failed result, for a
/// command whose completion reports its own disposition. `discard` disposes a
/// value that the result transfers when the interpreter has already gone.
fn submit_python_future<S, C>(
    py: Python<'_>,
    submit: S,
    convert: C,
    accept_error_status: bool,
    discard: Option<unsafe fn(&sys::mln_completion_result)>,
) -> PyResult<Py<PyAny>>
where
    S: FnOnce(*const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    C: FnOnce(Python<'_>, &sys::mln_completion_result) -> PyResult<Py<PyAny>> + Send + 'static,
{
    let future = new_python_future(py)?.unbind();
    let bridge = Box::new(PyCompletionBridge {
        future: future.clone_ref(py),
        convert: Mutex::new(Some(Box::new(convert))),
        accept_error_status,
        discard,
    });
    let bridge = Box::into_raw(bridge);
    let completion = sys::mln_completion {
        size: std::mem::size_of::<sys::mln_completion>() as u32,
        callback: Some(complete_python_future),
        user_data: bridge.cast(),
        release_user_data: Some(release_python_future),
    };
    if let Err(error) = maplibre_core::check(|diagnostic| submit(&completion, diagnostic)) {
        // SAFETY: rejected submissions retain no callback state.
        drop(unsafe { Box::from_raw(bridge) });
        return Err(map_error(error));
    }
    Ok(future)
}

fn completed_python_future(py: Python<'_>) -> PyResult<Py<PyAny>> {
    let future = new_python_future(py)?;
    future.call_method1("set_result", (py.None(),))?;
    Ok(future.unbind())
}

fn py_none(py: Python<'_>, result: &sys::mln_completion_result) -> PyResult<Py<PyAny>> {
    if !result.value.is_null() || result.value_count != 0 {
        return Err(native_error("unit completion returned a value"));
    }
    Ok(py.None())
}

fn py_command(py: Python<'_>, result: &sys::mln_completion_result) -> PyResult<Py<PyAny>> {
    if !result.value.is_null() || result.value_count != 0 {
        return Err(native_error("command completion returned a value"));
    }
    Ok(py
        .import("maplibre_native_ffi._completion")?
        .getattr("CommandCompletion")?
        .call1((
            result.disposition,
            result.generation,
            result.status,
            copy_completion_diagnostic(result),
        ))?
        .unbind())
}

fn copy_completion_diagnostic(result: &sys::mln_completion_result) -> String {
    if result.diagnostic.data.is_null() || result.diagnostic.size == 0 {
        return String::new();
    }
    // SAFETY: completion diagnostics are borrowed for this callback.
    String::from_utf8_lossy(unsafe {
        std::slice::from_raw_parts(result.diagnostic.data.cast::<u8>(), result.diagnostic.size)
    })
    .into_owned()
}

fn completion_value<T: Copy>(result: &sys::mln_completion_result) -> PyResult<T> {
    if result.value.is_null() || result.value_count != 1 {
        return Err(native_error("native completion returned no value"));
    }
    // SAFETY: each caller supplies the result type documented by the
    // submitting C function, and the value is borrowed for this callback.
    Ok(unsafe { result.value.cast::<T>().read_unaligned() })
}

fn invalid_argument_error(diagnostic: impl Into<String>) -> PyErr {
    py_errors::InvalidArgumentError::new_err(diagnostic.into())
}

fn invalid_state_error(diagnostic: impl Into<String>) -> PyErr {
    py_errors::InvalidStateError::new_err(diagnostic.into())
}

fn native_error(diagnostic: impl Into<String>) -> PyErr {
    py_errors::NativeError::new_err(diagnostic.into())
}

fn map_error(error: Error) -> PyErr {
    let raw_status = error.raw_status();
    let diagnostic = error.diagnostic().to_owned();
    match error.kind() {
        ErrorKind::NotFound => py_errors::NotFoundError::new_err((diagnostic, raw_status)),
        ErrorKind::InvalidArgument => {
            py_errors::InvalidArgumentError::new_err((diagnostic, raw_status))
        }
        ErrorKind::InvalidState => py_errors::InvalidStateError::new_err((diagnostic, raw_status)),
        ErrorKind::WrongThread => py_errors::WrongThreadError::new_err((diagnostic, raw_status)),
        ErrorKind::Unsupported => {
            py_errors::UnsupportedFeatureError::new_err((diagnostic, raw_status))
        }
        ErrorKind::NativeError => py_errors::NativeError::new_err((diagnostic, raw_status)),
        ErrorKind::Cancelled => py_errors::CancelledError::new_err((diagnostic, raw_status)),
        ErrorKind::Busy => py_errors::BusyError::new_err((diagnostic, raw_status)),
        ErrorKind::TargetLost => py_errors::TargetLostError::new_err((diagnostic, raw_status)),
        ErrorKind::NotReady => py_errors::NotReadyError::new_err((diagnostic, raw_status)),
        ErrorKind::UnknownStatus => {
            py_errors::UnknownStatusError::new_err((diagnostic, raw_status.unwrap_or_default()))
        }
        ErrorKind::AbiVersionMismatch => {
            py_errors::UnsupportedFeatureError::new_err((diagnostic, raw_status))
        }
        _ => py_errors::NativeError::new_err((diagnostic, raw_status)),
    }
}

#[pymodule]
fn _native(module: &Bound<'_, PyModule>) -> PyResult<()> {
    maplibre_core::validate_abi_version().map_err(map_error)?;
    register_generated_functions(module)?;
    Ok(())
}

fn copied_string_view(view: sys::mln_buffer_view) -> PyResult<String> {
    unsafe { maplibre_core::string::copy_string_view(view) }.map_err(map_error)
}
