#![deny(unsafe_op_in_unsafe_fn)]

#[path = "maplibre_core/mod.rs"]
mod maplibre_core;

use maplibre_core::handle::NativeHandle;
use maplibre_core::{Error, Status};
use maplibre_native_ffi_sys as sys;
use pyo3::prelude::*;
use pyo3::types::{PyAny, PyBytes, PyDict, PyList};
use std::ffi::{c_char, c_void};
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr;
use std::sync::atomic::Ordering;
use std::sync::{Arc, Mutex, MutexGuard};

include!("value_support.rs");
include!("call_support.rs");
include!("dispatcher.rs");
include!("generated_operations.rs");

include!("generated_errors.rs");

#[derive(Debug)]
struct NativeHandleState<T: maplibre_core::handle::NativeHandle> {
    handle: Arc<maplibre_core::handle::HandleState<T>>,
    callback_roots: Vec<std::sync::Weak<GeneratedCallbackRoot>>,
    dispose_abandoned: Option<unsafe extern "C" fn(T) -> sys::mln_status>,
    type_name: &'static str,
}

impl<T: maplibre_core::handle::NativeHandle> NativeHandleState<T> {
    /// Takes ownership of a native handle.
    ///
    /// # Safety
    ///
    /// `handle` must be a live handle of the matching native type owned by the
    /// caller, which must later close this state with the matching C API
    /// destroy function.
    unsafe fn from_handle(handle: T, type_name: &'static str) -> Result<Self, Error> {
        Ok(Self {
            handle: Arc::new(maplibre_core::handle::HandleState::new(handle, type_name)?),
            callback_roots: Vec::new(),
            dispose_abandoned: None,
            type_name,
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

    fn is_closed(&self) -> bool {
        self.handle.is_released()
    }
}

impl<T: maplibre_core::handle::NativeHandle> NativeHandleState<T> {
    /// Hands the live handle to the finalizer's silent disposal, which reports
    /// a leak if native refuses it.
    fn dispose_abandoned(&mut self) {
        let Some(dispose) = self.dispose_abandoned else {
            return;
        };
        if let Some(finalize) = self.handle.finalizer(move |handle| {
            // SAFETY: the state transfers ownership to its matching disposer.
            maplibre_core::check(|_| unsafe { dispose(handle) })
        }) {
            generated_finalize(finalize);
        }
    }
}

impl<T: maplibre_core::handle::NativeHandle> Drop for NativeHandleState<T> {
    fn drop(&mut self) {
        self.dispose_abandoned();
    }
}

/// The render sessions that interpreter shutdown abandons. Python owns these
/// sessions, which core cannot tell from those that other code in the process
/// owns.
static GENERATED_SESSIONS: Mutex<
    Vec<std::sync::Weak<Mutex<NativeHandleState<sys::mln_render_session>>>>,
> = Mutex::new(Vec::new());

/// Shares a new owner's state, and enrolls a render session for abandonment
/// at interpreter shutdown.
fn generated_owner_state<T: maplibre_core::handle::NativeHandle>(
    state: NativeHandleState<T>,
) -> Arc<Mutex<NativeHandleState<T>>> {
    let state = Arc::new(Mutex::new(state));
    let shared: Arc<dyn std::any::Any + Send + Sync> = state.clone();
    if let Ok(session) = shared.downcast::<Mutex<NativeHandleState<sys::mln_render_session>>>() {
        let mut sessions = generated_lock(&GENERATED_SESSIONS);
        sessions.retain(|session| session.strong_count() != 0);
        sessions.push(Arc::downgrade(&session));
    }
    state
}

/// Ends the graphics calls of every render session that Python still owns,
/// then disposes it.
///
/// Disposal alone would detach an attached session on its worker while the
/// process exits, and graphics drivers tear down their own state in exit
/// handlers that can run first. The host's own shutdown may also have
/// destroyed the graphics objects behind the session already. This
/// abandonment returns once the in-flight driver call has ended, makes no
/// graphics call, and keeps the session's graphics objects until the process
/// exits, so the disposal that follows makes no graphics call either. A
/// session that refuses abandonment, such as one inside a caller-driven call,
/// is disposed as it is.
fn generated_abandon_sessions_at_exit() {
    let sessions = std::mem::take(&mut *generated_lock(&GENERATED_SESSIONS));
    for session in sessions.iter().filter_map(std::sync::Weak::upgrade) {
        let state = generated_lock(&session);
        let Some(dispose) = state.dispose_abandoned else {
            continue;
        };
        let type_name = state.type_name;
        if state.handle.is_released() {
            continue;
        }
        let mut reservation = state.handle.reserve();
        let raw = reservation.handle();
        reservation.commit();
        drop(reservation);
        drop(state);
        let mut result = sys::mln_render_abandon_result {
            size: std::mem::size_of::<sys::mln_render_abandon_result>() as u32,
            disposition: 0,
            quarantined_resource_count: 0,
            reserved: 0,
        };
        // SAFETY: the committed reservation took sole ownership of the live
        // session, and result is a writable, correctly sized output.
        let _ = unsafe {
            sys::mln_binding_render_session_abandon_at_exit(raw, &mut result, ptr::null_mut())
        };
        // SAFETY: the reservation transferred the session to this disposal.
        if unsafe { dispose(raw) } != sys::MLN_STATUS_OK {
            maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
                type_name,
                id: raw.to_raw(),
            });
        }
    }
}

type PyCompletionConverter =
    Box<dyn FnOnce(Python<'_>, &sys::mln_completion_result) -> PyResult<Py<PyAny>> + Send>;

struct PyCompletionBridge {
    future: Py<PyAny>,
    convert: Mutex<Option<PyCompletionConverter>>,
}

fn new_python_future(py: Python<'_>) -> PyResult<Bound<'_, PyAny>> {
    let future = py
        .import("maplibre_native_ffi._future")?
        .getattr("NativeFuture")?
        .call0()?;
    // The future stays pending until its completion arrives, so cancel()
    // abandons the wait. Native work continues to its terminal disposition
    // either way.
    Ok(future)
}

unsafe extern "C" fn complete_python_future(
    user_data: *mut c_void,
    result: *const sys::mln_completion_result,
) {
    if user_data.is_null() || result.is_null() {
        return;
    }
    if let Err(payload) = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: native owns the bridge from accepted submission through its
        // release callback. The result is borrowed for this callback.
        let bridge = unsafe { &*user_data.cast::<PyCompletionBridge>() };
        let result = unsafe { &*result };
        let _ = Python::try_attach(|py| {
            // Claim the future before converting, as an executor claims its
            // work. A future that cancel() abandoned first takes no value.
            let claimed = bridge
                .future
                .bind(py)
                .call_method0("_claim")
                .and_then(|claimed| claimed.is_truthy());
            match claimed {
                Ok(true) => {}
                Ok(false) => return,
                Err(error) => {
                    error.write_unraisable(py, None);
                    return;
                }
            }
            let converted = if result.status == sys::MLN_STATUS_OK {
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
                completion_error(py, result)
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
    })) {
        generated_report_panic(payload);
    }
}

unsafe extern "C" fn release_python_future(user_data: *mut c_void) {
    if !user_data.is_null() {
        // SAFETY: native releases accepted completion state exactly once.
        let bridge = unsafe { Box::from_raw(user_data.cast::<PyCompletionBridge>()) };
        if let Err(payload) = catch_unwind(AssertUnwindSafe(|| drop(bridge))) {
            generated_report_panic(payload);
        }
    }
}

/// Submits a completion-based call and returns the future it resolves on the
/// dispatcher thread.
///
/// `convert` receives only a result whose status is OK.
fn submit_python_future<S, C>(py: Python<'_>, submit: S, convert: C) -> PyResult<Py<PyAny>>
where
    S: FnOnce(*const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    C: FnOnce(Python<'_>, &sys::mln_completion_result) -> PyResult<Py<PyAny>> + Send + 'static,
{
    let queue = generated_dispatch_queue()?;
    let future = new_python_future(py)?.unbind();
    let bridge = Box::new(PyCompletionBridge {
        future: future.clone_ref(py),
        convert: Mutex::new(Some(Box::new(convert))),
    });
    let bridge = Box::into_raw(bridge);
    let completion = sys::mln_completion {
        size: std::mem::size_of::<sys::mln_completion>() as u32,
        callback: Some(complete_python_future),
        user_data: bridge.cast(),
        release_user_data: Some(release_python_future),
        queue,
    };
    if let Err(error) = maplibre_core::check(|diagnostic| submit(&completion, diagnostic)) {
        // SAFETY: rejected submissions retain no callback state.
        drop(unsafe { Box::from_raw(bridge) });
        return Err(map_error(error));
    }
    Ok(future)
}

fn py_none(py: Python<'_>, result: &sys::mln_completion_result) -> PyResult<Py<PyAny>> {
    if !result.value.is_null() || result.value_count != 0 {
        return Err(native_error("unit completion returned a value"));
    }
    Ok(py.None())
}

/// Copies a committed command's generations. A failed or cancelled command
/// completes with a non-OK status, which `completion_error` raises instead.
fn py_command(py: Python<'_>, result: &sys::mln_completion_result) -> PyResult<Py<PyAny>> {
    if !result.value.is_null() || result.value_count != 0 {
        return Err(native_error("command completion returned a value"));
    }
    Ok(py
        .import("maplibre_native_ffi._completion")?
        .getattr("CommandCompletion")?
        .call1((result.generation, result.render_update_generation))?
        .unbind())
}

/// The error a failed completion raises, with the generation that a failed
/// map command published.
fn completion_error(py: Python<'_>, result: &sys::mln_completion_result) -> PyResult<Py<PyAny>> {
    let error = map_error(Error::from_status_and_diagnostic(
        result.status,
        copy_completion_diagnostic(result),
    ));
    error.value(py).setattr("generation", result.generation)?;
    Err(error)
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

fn invalid_state_error(diagnostic: impl Into<String>) -> PyErr {
    map_error(Error::new(Status::InvalidState, diagnostic))
}

fn native_error(diagnostic: impl Into<String>) -> PyErr {
    map_error(Error::new(Status::NativeError, diagnostic))
}

#[pymodule]
fn _native(module: &Bound<'_, PyModule>) -> PyResult<()> {
    maplibre_core::validate_abi_version().map_err(map_error)?;
    module.add_class::<GeneratedNativeRegistration>()?;
    module.add_function(wrap_pyfunction!(generated_on_dispatcher_thread, module)?)?;
    register_generated_functions(module)?;
    Ok(())
}

fn copied_string_view(view: sys::mln_buffer_view) -> PyResult<String> {
    unsafe { maplibre_core::string::copy_string_view(view) }.map_err(map_error)
}
