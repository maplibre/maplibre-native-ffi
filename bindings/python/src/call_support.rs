/// One generated method's native call: the Python storage its
/// arguments borrow, and the native call with the interpreter detached.
///
/// Every native call that a method makes goes through `status`,
/// `status_unless`, `run`, `command`, or `complete`. Each is unsafe with the
/// contract of `generated_native_call`: the closure only reads the converted
/// arguments, which this call's storage keeps rooted, and calls native code.
struct GeneratedCall<'py> {
    py: Python<'py>,
    storage: GeneratedInputStorage<'py>,
}

impl<'py> GeneratedCall<'py> {
    /// Keeps converted inputs alive through the native call.
    fn new(py: Python<'py>) -> Self {
        Self {
            py,
            storage: GeneratedInputStorage::default(),
        }
    }

    /// Makes a call that returns a status.
    unsafe fn status(
        &mut self,
        call: impl FnOnce(*mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> PyResult<()> {
        let py = self.py;
        // SAFETY: the caller upholds the native call contract.
        maplibre_core::check(|diagnostic| unsafe { generated_native_call(py, || call(diagnostic)) })
            .map_err(map_error)
    }

    /// Makes a call that returns a status, where `absent` reports that native
    /// published no output rather than an error. Returns whether the output
    /// holds a value.
    unsafe fn status_unless(
        &mut self,
        absent: sys::mln_status,
        call: impl FnOnce(*mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> PyResult<bool> {
        let py = self.py;
        let mut present = true;
        maplibre_core::check(|diagnostic| {
            // SAFETY: the caller upholds the native call contract.
            let status = unsafe { generated_native_call(py, || call(diagnostic)) };
            if status != absent {
                return status;
            }
            present = false;
            sys::MLN_STATUS_OK
        })
        .map_err(map_error)?;
        Ok(present)
    }

    /// Makes a call that returns a value or nothing.
    unsafe fn run<R>(&mut self, call: impl FnOnce() -> R) -> R {
        // SAFETY: the caller upholds the native call contract.
        unsafe { generated_native_call(self.py, call) }
    }

    /// Submits an ordered command, whose future raises unless it commits.
    unsafe fn command(
        &mut self,
        submit: impl FnOnce(*const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> PyResult<Py<PyAny>> {
        let py = self.py;
        submit_python_future(
            py,
            |completion, diagnostic| {
                // SAFETY: the caller upholds the native call contract.
                unsafe { generated_native_call(py, || submit(completion, diagnostic)) }
            },
            py_command,
        )
    }

    unsafe fn discard(
        &mut self,
        submit: impl FnOnce(*const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> PyResult<Py<PyAny>> {
        // SAFETY: the caller upholds the native call contract.
        unsafe { self.status(|diagnostic| submit(std::ptr::null(), diagnostic)) }?;
        Ok(self.py.None())
    }

    /// Submits an operation whose completion `convert` copies.
    unsafe fn complete<C>(
        &mut self,
        submit: impl FnOnce(*const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
        convert: C,
    ) -> PyResult<Py<PyAny>>
    where
        C: FnOnce(Python<'_>, &sys::mln_completion_result) -> PyResult<Py<PyAny>> + Send + 'static,
    {
        let py = self.py;
        submit_python_future(
            py,
            |completion, diagnostic| {
                // SAFETY: the caller upholds the native call contract.
                unsafe { generated_native_call(py, || submit(completion, diagnostic)) }
            },
            convert,
        )
    }

    /// Transfers the call's callback roots after native accepts them.
    fn accept_callbacks(&mut self) -> Vec<std::sync::Weak<GeneratedCallbackRoot>> {
        self.storage.accept_callbacks()
    }
}

type GeneratedOwnerReservation<O> =
    maplibre_core::handle::ReleaseReservation<<O as GeneratedOwner>::Native>;

trait GeneratedOwner: pyo3::PyClass + Into<pyo3::PyClassInitializer<Self>> {
    type Native: maplibre_core::handle::NativeHandle;
    const NATIVE: &'static str;
    const COUNTED: bool;
    /// Disposes a handle that no close consumed, for a handle type with a
    /// disposal.
    const DISPOSE: Option<unsafe extern "C" fn(Self::Native) -> sys::mln_status>;
    /// The release that interpreter shutdown starts and waits for, if any.
    const EXIT_RELEASE: Option<ExitReleaseFn<Self::Native>>;

    fn shared(&self) -> &Arc<Mutex<NativeHandleState<Self::Native>>>;
    fn from_shared(state: Arc<Mutex<NativeHandleState<Self::Native>>>) -> Self;

    fn state(&self) -> MutexGuard<'_, NativeHandleState<Self::Native>> {
        self.shared().lock().unwrap_or_else(|p| p.into_inner())
    }

    fn input(&self) -> Self::Native {
        self.state().handle.raw()
    }

    /// Hold off close until borrowed result storage has been copied.
    fn read(&self) -> PyResult<maplibre_core::handle::NativeRead<Self::Native>> {
        self.state().handle.read().map_err(map_error)
    }

    /// Takes the handle for a consuming call; an idempotent close skips a released handle.
    fn reserve(&self, idempotent: bool) -> PyResult<Option<GeneratedOwnerReservation<Self>>> {
        let state = self.state();
        if idempotent && state.handle.is_released() {
            return Ok(None);
        }
        state.handle.reserve().map(Some).map_err(map_error)
    }

    /// Wraps a handle that an accepted call transferred, with the callback
    /// roots that the call registered.
    ///
    /// # Safety
    ///
    /// `raw` must be a live handle of this owner's type, owned by the caller.
    unsafe fn adopt(
        py: Python<'_>,
        raw: Self::Native,
        roots: Vec<std::sync::Weak<GeneratedCallbackRoot>>,
    ) -> PyResult<Py<PyAny>> {
        // SAFETY: the caller transfers ownership of the live handle.
        let mut state = unsafe { NativeHandleState::from_handle(raw, Self::NATIVE, Self::COUNTED) }
            .map_err(map_error)?
            .with_callback_roots(roots);
        if let Some(dispose) = Self::DISPOSE {
            state = state.with_disposal(dispose);
        }
        if let Some(release) = Self::EXIT_RELEASE {
            state = state.with_exit_release(release);
        }
        Py::new(py, Self::from_shared(generated_owner_state(state))).map(|value| value.into_any())
    }
}

macro_rules! generated_owner {
    ($owner:ident, $name:literal, $native:ident, $dispose:expr, $exit_release:expr, $counted:expr $(, $read_scope:expr)?) => {
        #[pyclass(name = $name)]
        struct $owner {
            state: Arc<Mutex<NativeHandleState<sys::$native>>>,
        }

        impl GeneratedOwner for $owner {
            type Native = sys::$native;
            const NATIVE: &'static str = stringify!($native);
            const COUNTED: bool = $counted;
            const DISPOSE: Option<unsafe extern "C" fn(sys::$native) -> sys::mln_status> = $dispose;
            const EXIT_RELEASE: Option<ExitReleaseFn<sys::$native>> = $exit_release;

            fn shared(&self) -> &Arc<Mutex<NativeHandleState<sys::$native>>> {
                &self.state
            }

            fn from_shared(state: Arc<Mutex<NativeHandleState<sys::$native>>>) -> Self {
                Self { state }
            }
        }

        #[pymethods]
        impl $owner {
            #[getter]
            fn _is_closed(&self) -> bool {
                self.state().is_closed()
            }

            #[getter]
            fn id(&self) -> u64 {
                self.state().handle.raw().to_raw()
            }

            fn __traverse__(
                &self,
                visit: pyo3::gc::PyVisit<'_>,
            ) -> Result<(), pyo3::gc::PyTraverseError> {
                self.state().traverse_callbacks(&visit)
            }

            fn __clear__(&self) {
                // Dropping a callback can run Python code that reenters this
                // owner, so the drop happens after the state lock is released.
                let callbacks = self.state().take_callbacks();
                drop(callbacks);
            }

            /// Disposes the handle as collecting this owner would, for a value
            /// that a completion delivered and nothing adopted.
            fn _dispose(&self) -> PyResult<()> {
                let mut state = self.state();
                let _reservation = state.handle.reserve().map_err(map_error)?;
                state.dispose_abandoned();
                Ok(())
            }

            $(fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
                let read_scope: fn(Python<'_>, &Self) -> PyResult<GeneratedReadScope> = $read_scope;
                read_scope(py, self)
            })?
        }
    };
}

/// Runs the Python side of a native callback with the interpreter attached.
/// An exception or panic is reported as unraisable and answers native with
/// `failure`. A finalized interpreter also answers with `failure`.
fn generated_invoke<R>(failure: impl Fn() -> R, body: impl FnOnce(Python<'_>) -> PyResult<R>) -> R {
    let _scope = maplibre_core::callback::CallbackScope::enter();
    let result = catch_unwind(AssertUnwindSafe(|| Python::try_attach(body)));
    match result {
        Ok(Some(Ok(result))) => result,
        Ok(Some(Err(error))) => {
            Python::try_attach(|py| generated_report_unraisable(py, error));
            failure()
        }
        Err(payload) => {
            generated_report_panic(payload);
            failure()
        }
        Ok(None) => failure(),
    }
}

fn generated_report_panic(payload: Box<dyn std::any::Any + Send>) {
    // A panic payload can itself panic when dropped across this C boundary.
    std::mem::forget(payload);
    Python::try_attach(|py| {
        generated_report_unraisable(
            py,
            pyo3::panic::PanicException::new_err("Rust callback panicked"),
        );
    });
}

/// The status that native receives from a status callback that raised. A
/// raised `MaplibreError` whose status is not OK is a deliberate failure, which
/// native receives without a report. Any other exception is reported and
/// answers `failure`.
fn generated_callback_status(
    py: Python<'_>,
    error: PyErr,
    failure: sys::mln_status,
) -> sys::mln_status {
    if error.is_instance_of::<py_errors::MaplibreError>(py) {
        let status = error
            .value(py)
            .getattr("status")
            .and_then(|status| status.extract::<sys::mln_status>());
        if let Ok(status) = status
            && status != sys::MLN_STATUS_OK
        {
            return status;
        }
    }
    generated_report_unraisable(py, error);
    failure
}

/// Reports an exception that a native callback raised to `sys.unraisablehook`.
/// The callback scope keeps finalization off the native callback stack.
fn generated_report_unraisable(py: Python<'_>, error: PyErr) {
    let _scope = maplibre_core::callback::CallbackScope::enter();
    error.write_unraisable(py, None);
}

fn generated_value<'py, T: IntoPyObject<'py>>(py: Python<'py>, value: T) -> PyResult<Py<PyAny>> {
    let value = value.into_pyobject(py).map_err(Into::into)?;
    Ok(pyo3::BoundObject::unbind(pyo3::BoundObject::into_any(
        value,
    )))
}

fn generated_optional(
    py: Python<'_>,
    present: bool,
    copy: impl FnOnce() -> PyResult<Py<PyAny>>,
) -> PyResult<Py<PyAny>> {
    if present { copy() } else { Ok(py.None()) }
}

fn generated_text(py: Python<'_>, view: sys::mln_buffer_view) -> PyResult<Py<PyAny>> {
    generated_value(py, copied_string_view(view)?)
}

/// Parses a JSON view with `json.loads`; an empty view is absent, `None`.
fn generated_json(py: Python<'_>, view: sys::mln_buffer_view) -> PyResult<Py<PyAny>> {
    if view.size == 0 {
        return Ok(py.None());
    }
    let text = copied_string_view(view)?;
    Ok(py.import("json")?.call_method1("loads", (text,))?.unbind())
}

/// Copies a byte view into Python bytes.
///
/// # Safety
///
/// A nonempty view must address readable bytes.
unsafe fn generated_bytes(py: Python<'_>, view: sys::mln_buffer_view) -> PyResult<Py<PyAny>> {
    // SAFETY: the caller guarantees the view is readable.
    let bytes = unsafe { generated_slice(view.data.cast::<u8>(), view.size)? };
    Ok(PyBytes::new(py, bytes).into_any().unbind())
}

/// Makes a call that writes text into storage the caller sizes, and copies
/// the text into `text`. A call reports the text's length whether or not it
/// fit, and reports the same text for the same inputs, so a call that did not
/// fit is made again with enough storage.
fn generated_caller_sized_text(
    text: &mut Vec<u8>,
    mut call: impl FnMut(*mut c_char, usize, *mut usize) -> sys::mln_status,
) -> sys::mln_status {
    let mut storage = vec![0u8; 256];
    loop {
        let mut length = 0;
        let status = call(storage.as_mut_ptr().cast(), storage.len(), &mut length);
        if status != sys::MLN_STATUS_OK {
            return status;
        }
        if length < storage.len() {
            storage.truncate(length);
            *text = storage;
            return status;
        }
        storage = vec![0; length + 1];
    }
}

/// Copies a terminated UTF-8 string, which is `None` when null and `optional`.
///
/// # Safety
///
/// A nonnull pointer must address a terminated string.
unsafe fn generated_c_string(
    py: Python<'_>,
    text: *const c_char,
    optional: bool,
) -> PyResult<Py<PyAny>> {
    if text.is_null() {
        return if optional {
            Ok(py.None())
        } else {
            Err(native_error("null native string"))
        };
    }
    // SAFETY: the caller guarantees a terminated string.
    let text = unsafe { std::ffi::CStr::from_ptr(text) }
        .to_str()
        .map_err(|_| native_error("native string is not UTF-8"))?;
    generated_value(py, text)
}

fn generated_list<T>(
    py: Python<'_>,
    items: impl IntoIterator<Item = T>,
    mut copy: impl FnMut(T) -> PyResult<Py<PyAny>>,
) -> PyResult<Py<PyAny>> {
    let list = PyList::empty(py);
    for item in items {
        list.append(copy(item)?)?;
    }
    Ok(list.into_any().unbind())
}

fn generated_variant(py: Python<'_>, kind: &str, value: Py<PyAny>) -> PyResult<Py<PyAny>> {
    let variant = PyDict::new(py);
    variant.set_item("kind", kind)?;
    variant.set_item("value", value)?;
    Ok(variant.into_any().unbind())
}

/// A union arm this binding predates, which keeps its tag.
fn generated_unknown_variant<'py>(
    py: Python<'py>,
    tag: impl IntoPyObject<'py>,
) -> PyResult<Py<PyAny>> {
    let variant = PyDict::new(py);
    variant.set_item("kind", py.None())?;
    variant.set_item("tag", tag)?;
    Ok(variant.into_any().unbind())
}

fn generated_present<'py>(
    value: &Bound<'py, PyAny>,
    name: &str,
) -> PyResult<Option<Bound<'py, PyAny>>> {
    let field = value.getattr(name)?;
    Ok((!field.is_none()).then_some(field))
}

fn generated_items<'py, T>(
    values: &Bound<'py, PyAny>,
    mut convert: impl FnMut(Bound<'py, PyAny>) -> PyResult<T>,
) -> PyResult<Vec<T>> {
    // A str is iterable, and would otherwise split into its characters.
    if values.is_instance_of::<pyo3::types::PyString>() {
        return Err(pyo3::exceptions::PyTypeError::new_err(
            "expected a sequence, not str",
        ));
    }
    values.try_iter()?.map(|item| convert(item?)).collect()
}

fn generated_maybe<'py, T>(
    value: &Bound<'py, PyAny>,
    convert: impl FnOnce(Bound<'py, PyAny>) -> PyResult<T>,
) -> PyResult<Option<T>> {
    if value.is_none() {
        Ok(None)
    } else {
        convert(value.clone()).map(Some)
    }
}

fn generated_pointer<T>(value: &Option<T>) -> *const T {
    value.as_ref().map_or(std::ptr::null(), |value| value)
}
