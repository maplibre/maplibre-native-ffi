#![deny(unsafe_op_in_unsafe_fn)]

use maplibre_core::handle::NativeHandle;
use maplibre_native_ffi_core::{self as maplibre_core, Error, Status};
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
include!("generated_operations.rs");

include!("generated_errors.rs");

#[derive(Debug)]
struct NativeHandleState<T: maplibre_core::handle::NativeHandle> {
    handle: Arc<maplibre_core::handle::HandleState<T>>,
    callback_roots: Vec<std::sync::Weak<GeneratedCallbackRoot>>,
    dispose_abandoned: Option<unsafe extern "C" fn(T) -> sys::mln_status>,
    /// The release that interpreter shutdown starts and waits for, for a
    /// root handle whose release reports retirement through a completion.
    exit_release: Option<ExitReleaseFn<T>>,
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
    unsafe fn from_handle(
        handle: T,
        type_name: &'static str,
        counted: bool,
    ) -> Result<Self, Error> {
        Ok(Self {
            handle: Arc::new(maplibre_core::handle::HandleState::new(
                handle, type_name, counted,
            )?),
            callback_roots: Vec::new(),
            dispose_abandoned: None,
            exit_release: None,
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

    fn with_exit_release(mut self, release: ExitReleaseFn<T>) -> Self {
        self.exit_release = Some(release);
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

/// The longest interpreter shutdown waits for its runtimes to retire.
const EXIT_RETIREMENT_TIMEOUT: std::time::Duration = std::time::Duration::from_secs(5);

type ExitReleaseFn<T> = unsafe extern "C" fn(T, *const sys::mln_completion) -> sys::mln_status;

/// An owner that interpreter shutdown retires. The interpreter finalizes before
/// the C runtime's exit handlers run, so native callbacks into it must stop
/// first, and releasing each root owner and waiting for its completion stops
/// them.
trait ExitOwner: Send + Sync {
    fn releases_at_exit(&self) -> bool;
    fn release_at_exit(&self, raw: u64, completion: &sys::mln_completion) -> sys::mln_status;
    /// Marks an unreleased owner for shutdown. Borrowed output reads keep
    /// their owner in place until the copy ends.
    fn take_at_exit(&self) -> Option<u64>;
    fn dispose_at_exit(&self, raw: u64);
}

impl<T: maplibre_core::handle::NativeHandle> ExitOwner for Mutex<NativeHandleState<T>> {
    fn releases_at_exit(&self) -> bool {
        self.lock()
            .unwrap_or_else(|p| p.into_inner())
            .exit_release
            .is_some()
    }

    fn release_at_exit(&self, raw: u64, completion: &sys::mln_completion) -> sys::mln_status {
        let Some(release) = self.lock().unwrap_or_else(|p| p.into_inner()).exit_release else {
            return sys::MLN_STATUS_INVALID_STATE;
        };
        // SAFETY: take_at_exit retained this owner's id for shutdown.
        unsafe { release(T::from_raw(raw), completion) }
    }

    fn take_at_exit(&self) -> Option<u64> {
        let state = self.lock().unwrap_or_else(|p| p.into_inner());
        if state.handle.is_released() || state.dispose_abandoned.is_none() {
            return None;
        }
        let mut reservation = state.handle.reserve().ok()?;
        let raw = reservation.handle().to_raw();
        reservation.commit();
        Some(raw)
    }

    fn dispose_at_exit(&self, raw: u64) {
        let state = self.lock().unwrap_or_else(|p| p.into_inner());
        let (Some(dispose), type_name) = (state.dispose_abandoned, state.type_name) else {
            return;
        };
        drop(state);
        end_graphics_at_exit::<T>(raw);
        // SAFETY: take_at_exit retained this owner's id for shutdown.
        if unsafe { dispose(T::from_raw(raw)) } != sys::MLN_STATUS_OK {
            maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
                type_name,
                id: raw,
            });
        }
    }
}

unsafe extern "C" {
    // Binding-internal export from maplibre_native_c/binding_support.h,
    // which the sys crate does not bind.
    fn mln_binding_render_session_abandon_at_exit(
        session: sys::mln_render_session,
        out_result: *mut sys::mln_render_abandon_result,
        out_diagnostic: *mut sys::mln_diagnostic,
    ) -> sys::mln_status;
}

/// Ends a render session's graphics calls before shutdown disposes it.
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
fn end_graphics_at_exit<T: 'static>(raw: u64) {
    if std::any::TypeId::of::<T>() != std::any::TypeId::of::<sys::mln_render_session>() {
        return;
    }
    let mut result = sys::mln_render_abandon_result {
        size: std::mem::size_of::<sys::mln_render_abandon_result>() as u32,
        disposition: 0,
        quarantined_resource_count: 0,
        reserved: 0,
    };
    // SAFETY: the caller holds sole ownership of the live session, and result
    // is a writable, correctly sized output.
    let _ = unsafe {
        mln_binding_render_session_abandon_at_exit(
            sys::mln_render_session(raw),
            &mut result,
            ptr::null_mut(),
        )
    };
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
/// shutdown. The first owner that shutdown releases registers the hook.
fn generated_owner_state<T: maplibre_core::handle::NativeHandle>(
    state: NativeHandleState<T>,
) -> Arc<Mutex<NativeHandleState<T>>> {
    let state = Arc::new(Mutex::new(state));
    let owner: Arc<dyn ExitOwner> = state.clone();
    if owner.releases_at_exit() {
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

/// Disposes every owner the interpreter still holds, ending each render
/// session's graphics calls first, then releases each root owner and waits
/// for its retirement, so no native thread can call into a finalized
/// interpreter.
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
        for owner in owners.iter().filter(|owner| !owner.releases_at_exit()) {
            if let Some(raw) = owner.take_at_exit() {
                owner.dispose_at_exit(raw);
            }
        }
        let mut releases = Vec::new();
        for owner in owners.iter().filter(|owner| owner.releases_at_exit()) {
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
                queue: sys::mln_delivery_queue(0),
            };
            let status = owner.release_at_exit(raw, &completion);
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
    let _scope = maplibre_core::callback::CallbackScope::enter();
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
    let _scope = maplibre_core::callback::CallbackScope::enter();
    if !user_data.is_null() {
        // SAFETY: native releases accepted completion state exactly once.
        let bridge = unsafe { Box::from_raw(user_data.cast::<PyCompletionBridge>()) };
        if let Err(payload) = catch_unwind(AssertUnwindSafe(|| drop(bridge))) {
            generated_report_panic(payload);
        }
    }
}

/// Submits a completion-based call and returns the future it resolves.
///
/// `convert` receives only a result whose status is OK.
fn submit_python_future<S, C>(py: Python<'_>, submit: S, convert: C) -> PyResult<Py<PyAny>>
where
    S: FnOnce(*const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    C: FnOnce(Python<'_>, &sys::mln_completion_result) -> PyResult<Py<PyAny>> + Send + 'static,
{
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
        queue: sys::mln_delivery_queue(0),
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
    register_generated_functions(module)?;
    Ok(())
}

fn copied_string_view(view: sys::mln_buffer_view) -> PyResult<String> {
    unsafe { maplibre_core::string::copy_string_view(view) }.map_err(map_error)
}
