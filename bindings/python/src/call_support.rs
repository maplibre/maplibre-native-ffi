/// One generated method's native call: its admission, the Python storage its
/// arguments borrow, and the native call with the interpreter detached.
///
/// Every native call that a method makes goes through `status`, `run`,
/// `command`, `complete`, or `complete_owned`. Each is unsafe with the
/// contract of `generated_native_call`: the closure only reads the converted
/// arguments, which this call's storage keeps rooted, and calls native code.
struct GeneratedCall<'py> {
    py: Python<'py>,
    storage: GeneratedInputStorage<'py>,
}

impl<'py> GeneratedCall<'py> {
    /// Admits `operation` on the owner whose handle is `owner`, or 0 for none.
    fn new(py: Python<'py>, operation: &str, owner: u64) -> PyResult<Self> {
        generated_check_operation(operation, owner)?;
        Ok(Self {
            py,
            storage: GeneratedInputStorage::default(),
        })
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

    /// Makes a call that returns a value or nothing.
    unsafe fn run<R>(&mut self, call: impl FnOnce() -> R) -> R {
        // SAFETY: the caller upholds the native call contract.
        unsafe { generated_native_call(self.py, call) }
    }

    /// Submits an ordered command, whose completion reports its own
    /// disposition.
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
            true,
            None,
        )
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
            false,
            None,
        )
    }

    /// Submits an operation whose completion transfers a handle, which
    /// `discard` disposes when the interpreter has gone or the future was
    /// cancelled first.
    unsafe fn complete_owned<C>(
        &mut self,
        submit: impl FnOnce(*const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
        convert: C,
        discard: unsafe fn(&sys::mln_completion_result),
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
            false,
            Some(discard),
        )
    }

    /// Transfers the call's callback roots after native accepts them.
    fn accept_callbacks(&mut self) -> Vec<std::sync::Weak<GeneratedCallbackRoot>> {
        self.storage.accept_callbacks()
    }
}

/// The reservation that a consuming call holds on an owner's handle.
type GeneratedOwnerReservation<'a, O> = GeneratedHandleReservation<
    'a,
    <O as GeneratedOwner>::Native,
    NativeHandleState<<O as GeneratedOwner>::Native>,
>;

/// A Python owner class whose handle `NativeHandleState` tracks.
trait GeneratedOwner: pyo3::PyClass + Into<pyo3::PyClassInitializer<Self>> {
    type Native: maplibre_core::handle::NativeHandle;
    /// The C type name, for diagnostics.
    const NATIVE: &'static str;
    /// The public type name, which a lifecycle error names.
    const OWNER_NAME: &'static str;
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

    /// The handle that callback reentry policies name, or 0 once closed.
    fn admission(&self) -> u64 {
        self.state()
            .live_handle()
            .map(maplibre_core::handle::NativeHandle::to_raw)
            .unwrap_or(0)
    }

    /// The live handle that another owner's call takes as an argument.
    fn input(&self) -> PyResult<Self::Native> {
        self.state().require_live()
    }

    /// The live handle for a call that borrows none of its storage.
    fn live(&self) -> PyResult<Self::Native> {
        self.state().require_live()
    }

    /// Holds off close while a call's results borrow the handle's storage.
    fn read(
        &self,
    ) -> PyResult<GeneratedReadReservation<'_, Self::Native, NativeHandleState<Self::Native>>> {
        GeneratedReadReservation::new(self.shared())
    }

    /// Takes the handle for a call that consumes it, or `None` once closed.
    fn reserve(&self) -> PyResult<Option<GeneratedOwnerReservation<'_, Self>>> {
        GeneratedHandleReservation::new(self.shared())
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
        let mut state =
            unsafe { NativeHandleState::from_handle(raw, Self::NATIVE, Self::OWNER_NAME) }
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

/// Declares the Python class `$name` that owns one `$native` handle, and its
/// members that every owner shares.
macro_rules! generated_owner {
    ($owner:ident, $name:literal, $native:ident, $dispose:expr, $exit_release:expr, $read_scope:expr) => {
        #[pyclass(name = $name)]
        struct $owner {
            state: Arc<Mutex<NativeHandleState<sys::$native>>>,
        }

        impl GeneratedOwner for $owner {
            type Native = sys::$native;
            const NATIVE: &'static str = stringify!($native);
            const OWNER_NAME: &'static str = stringify!($owner);
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
            fn closed(&self) -> bool {
                self.state().is_closed()
            }

            #[getter]
            fn id(&self) -> u64 {
                self.state().issued_id()
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

            fn _read_scope(&self, py: Python<'_>) -> PyResult<GeneratedReadScope> {
                generated_check_reentry()?;
                let read_scope: fn(Python<'_>, &Self) -> PyResult<GeneratedReadScope> = $read_scope;
                read_scope(py, self)
            }
        }
    };
}

/// Runs the Python side of a native callback with the interpreter attached.
/// An exception is reported as unraisable and, like a panic or a finalized
/// interpreter, answers native with `failure`.
fn generated_invoke<R>(failure: impl Fn() -> R, body: impl FnOnce(Python<'_>) -> PyResult<R>) -> R {
    let result = catch_unwind(AssertUnwindSafe(|| Python::try_attach(body)));
    match result {
        Ok(Some(Ok(result))) => result,
        Ok(Some(Err(error))) => {
            Python::try_attach(|py| generated_report_unraisable(py, error));
            failure()
        }
        _ => failure(),
    }
}

/// Reports an exception that a native callback raised to `sys.unraisablehook`.
/// The hook runs on the native callback's stack, so it may make no native
/// call, whatever the callback itself may call.
fn generated_report_unraisable(py: Python<'_>, error: PyErr) {
    let _policy = GeneratedCallbackPolicy::enter(&[], 0);
    error.write_unraisable(py, None);
}

/// Converts a scalar, enum, or pointer value into a Python object.
fn generated_value<'py, T: IntoPyObject<'py>>(py: Python<'py>, value: T) -> PyResult<Py<PyAny>> {
    let value = value.into_pyobject(py).map_err(Into::into)?;
    Ok(pyo3::BoundObject::unbind(pyo3::BoundObject::into_any(
        value,
    )))
}

/// Copies an optional value: `None` unless `present`.
fn generated_optional(
    py: Python<'_>,
    present: bool,
    copy: impl FnOnce() -> PyResult<Py<PyAny>>,
) -> PyResult<Py<PyAny>> {
    if present { copy() } else { Ok(py.None()) }
}

/// Copies a UTF-8 view into a Python string.
fn generated_text(py: Python<'_>, view: sys::mln_buffer_view) -> PyResult<Py<PyAny>> {
    generated_value(py, copied_string_view(view)?)
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

/// Copies each item into a Python list.
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

/// One arm of a tagged union: its variant name and payload.
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

/// The attribute `name` of a Python value, or `None` when it holds `None`.
fn generated_present<'py>(
    value: &Bound<'py, PyAny>,
    name: &str,
) -> PyResult<Option<Bound<'py, PyAny>>> {
    let field = value.getattr(name)?;
    Ok((!field.is_none()).then_some(field))
}

/// Converts each item of a Python iterable.
fn generated_items<'py, T>(
    values: &Bound<'py, PyAny>,
    mut convert: impl FnMut(Bound<'py, PyAny>) -> PyResult<T>,
) -> PyResult<Vec<T>> {
    values.try_iter()?.map(|item| convert(item?)).collect()
}

/// Converts a Python value that may be `None`.
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

/// A pointer to an optional converted value, or null when it is absent.
fn generated_pointer<T>(value: &Option<T>) -> *const T {
    value.as_ref().map_or(std::ptr::null(), |value| value)
}
