/// Borrows Python text and bytes for one C call and owns native array layouts.
///
/// Generated converters keep this on the submitting stack. The C contracts
/// require accepted operations to copy call-lifetime inputs before returning.
#[derive(Default)]
struct GeneratedInputStorage<'py> {
    roots: Vec<Bound<'py, PyAny>>,
    storage: maplibre_core::memory::InputStorage,
    callbacks: Vec<GeneratedPendingCallback>,
    native_registrations: Vec<GeneratedPendingNative>,
}

impl<'py> GeneratedInputStorage<'py> {
    fn buffer(&mut self, value: Bound<'py, PyAny>, text: bool) -> PyResult<sys::mln_buffer_view> {
        if value.is_none() {
            return Ok(sys::mln_buffer_view {
                data: ptr::null(),
                size: 0,
            });
        }
        // Native reads the buffer with the interpreter detached, so a buffer
        // that Python code could mutate meanwhile is copied into bytes first.
        let value = if text || value.is_exact_instance_of::<PyBytes>() {
            value
        } else {
            pyo3::types::PyMemoryView::from(&value)?.call_method0("tobytes")?
        };
        let bytes = if text {
            value.extract::<&str>()?.as_bytes()
        } else {
            value.cast::<PyBytes>()?.as_bytes()
        };
        // A slice pointer is never null, so a present empty value keeps a
        // nonnull pointer, and None alone denotes absence.
        let view = sys::mln_buffer_view {
            data: bytes.as_ptr().cast(),
            size: bytes.len(),
        };
        self.roots.push(value);
        Ok(view)
    }

    /// Serializes a value with `json.dumps`. A record member that is None,
    /// `member`, is absent and converts to an empty view; a parameter that is
    /// None is JSON null.
    fn json(&mut self, value: Bound<'py, PyAny>, member: bool) -> PyResult<sys::mln_buffer_view> {
        if member && value.is_none() {
            return Ok(sys::mln_buffer_view {
                data: ptr::null(),
                size: 0,
            });
        }
        let text = value.py().import("json")?.call_method1("dumps", (value,))?;
        self.buffer(text, true)
    }

    fn c_string(&mut self, value: Bound<'py, PyAny>) -> PyResult<*const c_char> {
        self.storage
            .c_string(value.extract::<&str>()?)
            .map_err(map_error)
    }
    fn keep_array<T: 'static>(&mut self, values: Vec<T>) -> *const T {
        self.storage.array(values)
    }
    fn keep_one<T: 'static>(&mut self, value: T) -> *const T {
        self.storage.store(value)
    }
}

/// The generated caller supplies a C-contract borrowed array during its lifetime.
unsafe fn generated_slice<'a, T>(data: *const T, count: impl TryInto<usize>) -> PyResult<&'a [T]> {
    unsafe { maplibre_core::ptr::slice(data, generated_count(count)?) }.map_err(map_error)
}

/// Native array elements lie value_size bytes apart, which may exceed sizeof(T).
fn generated_completion_values<T: Copy>(
    result: &sys::mln_completion_result,
) -> PyResult<impl Iterator<Item = T>> {
    // SAFETY: the generated callback selects T from the submitting operation's
    // resolved result contract, and native lays out value_count elements
    // value_size bytes apart for the duration of the callback.
    unsafe {
        generated_strided_values(
            result.value.cast::<T>(),
            result.value_count,
            result.value_size,
        )
    }
}

/// Owns every immediate output before converting any of them to Python objects.
struct GeneratedOwnedOutput<T: maplibre_core::handle::NativeHandle> {
    handle: T,
    dispose: unsafe extern "C" fn(T) -> sys::mln_status,
}

impl<T: maplibre_core::handle::NativeHandle> GeneratedOwnedOutput<T> {
    fn new(handle: T, dispose: unsafe extern "C" fn(T) -> sys::mln_status) -> Self {
        Self { handle, dispose }
    }

    fn take(&mut self) -> T {
        std::mem::replace(&mut self.handle, T::from_raw(0))
    }
}

impl<T: maplibre_core::handle::NativeHandle> Drop for GeneratedOwnedOutput<T> {
    fn drop(&mut self) {
        if self.handle.to_raw() != 0 {
            let id = self.handle.to_raw();
            let dispose = self.dispose;
            generated_finalize(move || {
                // SAFETY: the generated owner plan pairs the output and disposer.
                unsafe { dispose(T::from_raw(id)) };
            });
        }
    }
}

type GeneratedCallbackRoot = Mutex<Vec<Py<PyAny>>>;

struct GeneratedPendingCallback {
    root: Arc<GeneratedCallbackRoot>,
    native: Option<*mut Arc<GeneratedCallbackRoot>>,
}

impl Drop for GeneratedPendingCallback {
    fn drop(&mut self) {
        if let Some(native) = self.native.take() {
            // SAFETY: unaccepted registrations remain owned by the submitting call.
            drop(unsafe { Box::from_raw(native) });
        }
    }
}

/// A registration whose function pointers and context the host's native code
/// supplies. The binding copies them unchanged into the C registration.
#[pyclass(name = "_NativeRegistration", frozen)]
struct GeneratedNativeRegistration {
    /// The C registration type that it fills.
    kind: String,
    parts: GeneratedNativeParts,
    consumed: std::sync::atomic::AtomicBool,
}

#[derive(Clone)]
struct GeneratedNativeParts {
    /// One address for each callback field, in field order.
    callbacks: Vec<usize>,
    user_data: usize,
    release: usize,
}

#[pymethods]
impl GeneratedNativeRegistration {
    #[new]
    fn new(kind: String, callbacks: Vec<usize>, user_data: usize, release: usize) -> Self {
        Self {
            kind,
            parts: GeneratedNativeParts {
                callbacks,
                user_data,
                release,
            },
            consumed: std::sync::atomic::AtomicBool::new(false),
        }
    }
}

/// A native registration that a call took. Unless native accepts the call,
/// the binding releases its context, as native would have.
struct GeneratedPendingNative {
    user_data: usize,
    release: Option<unsafe extern "C" fn(*mut c_void)>,
    accepted: bool,
}

impl Drop for GeneratedPendingNative {
    fn drop(&mut self) {
        if self.accepted {
            return;
        }
        if let Some(release) = self.release {
            let user_data = self.user_data;
            // SAFETY: the host supplied the release for this context, which
            // native never accepted.
            generated_finalize(move || unsafe { release(user_data as *mut c_void) });
        }
    }
}

impl GeneratedInputStorage<'_> {
    fn register_callbacks(&mut self, callbacks: Vec<Py<PyAny>>) -> *mut c_void {
        let root = Arc::new(Mutex::new(callbacks));
        let native = Box::into_raw(Box::new(Arc::clone(&root)));
        self.callbacks.push(GeneratedPendingCallback {
            root,
            native: Some(native),
        });
        native.cast()
    }

    /// Takes the native registration that `value` holds, when it holds one
    /// for the registration type `kind`. The call that takes it owns its
    /// context until native accepts the call.
    fn native_registration(
        &mut self,
        value: &Bound<'_, PyAny>,
        kind: &str,
    ) -> PyResult<Option<GeneratedNativeParts>> {
        let Some(registration) = value.getattr_opt(pyo3::intern!(value.py(), "_registration"))?
        else {
            return Ok(None);
        };
        let registration = registration.cast_into::<GeneratedNativeRegistration>()?;
        let registration = registration.get();
        if registration.kind != kind {
            return Err(pyo3::exceptions::PyTypeError::new_err(format!(
                "expected a native {kind}, got a native {}",
                registration.kind
            )));
        }
        if registration.consumed.swap(true, Ordering::AcqRel) {
            return Err(invalid_state_error(
                "a native registration is consumed by the first call that carries it",
            ));
        }
        let parts = registration.parts.clone();
        self.native_registrations.push(GeneratedPendingNative {
            user_data: parts.user_data,
            // SAFETY: a nonzero address is the host's release function, and
            // zero is None.
            release: unsafe {
                std::mem::transmute::<usize, Option<unsafe extern "C" fn(*mut c_void)>>(
                    parts.release,
                )
            },
            accepted: false,
        });
        Ok(Some(parts))
    }

    fn accept_callbacks(&mut self) -> Vec<std::sync::Weak<GeneratedCallbackRoot>> {
        for pending in &mut self.native_registrations {
            pending.accepted = true;
        }
        self.callbacks
            .iter_mut()
            .map(|pending| {
                pending.native = None;
                Arc::downgrade(&pending.root)
            })
            .collect()
    }
}

fn generated_root_callback(
    py: Python<'_>,
    root: &GeneratedCallbackRoot,
    index: usize,
) -> Option<Py<PyAny>> {
    root.lock()
        .unwrap_or_else(|p| p.into_inner())
        .get(index)
        .map(|value| value.clone_ref(py))
}

unsafe fn generated_get_callback(
    py: Python<'_>,
    user_data: *mut c_void,
    index: usize,
) -> Option<Py<PyAny>> {
    // SAFETY: native keeps the registration alive through each callback.
    let root = unsafe { &*user_data.cast::<Arc<GeneratedCallbackRoot>>() };
    generated_root_callback(py, root, index)
}

/// Holds a registration's callback root and drops its Python callbacks with
/// the interpreter attached when the registration retires.
struct GeneratedCallbackRootOwner(Arc<GeneratedCallbackRoot>);

impl Drop for GeneratedCallbackRootOwner {
    fn drop(&mut self) {
        let callbacks = std::mem::take(&mut *self.0.lock().unwrap_or_else(|p| p.into_inner()));
        let mut callbacks = Some(callbacks);
        Python::try_attach(|_| drop(callbacks.take()));
        drop(callbacks);
    }
}

unsafe extern "C" fn generated_release_callbacks(user_data: *mut c_void) {
    let _scope = maplibre_core::callback::CallbackScope::enter();
    // SAFETY: native releases the accepted descriptor exactly once at quiescence.
    let root = unsafe { Box::from_raw(user_data.cast::<Arc<GeneratedCallbackRoot>>()) };
    drop(GeneratedCallbackRootOwner(*root));
}

struct GeneratedDetached<T>(T);
// SAFETY: generated callers keep borrowed C storage alive on this thread and
// use the wrapper only across a GIL detach/reattach on that same thread.
unsafe impl<T> Send for GeneratedDetached<T> {}

impl<F, R> GeneratedDetached<F>
where
    F: FnOnce() -> R,
{
    fn invoke(self) -> GeneratedDetached<R> {
        GeneratedDetached((self.0)())
    }
}

/// Runs a native call while callbacks on other threads can acquire the GIL.
///
/// # Safety
/// The closure must only access prepared C values and call native code. Borrowed
/// Python buffers must be immutable and rooted until this function returns.
unsafe fn generated_native_call<F, R>(py: Python<'_>, call: F) -> R
where
    F: FnOnce() -> R,
{
    let call = GeneratedDetached(call);
    py.detach(move || call.invoke()).0
}

#[pyclass]
struct GeneratedReadScope {
    lease: Mutex<Option<GeneratedNativeRead>>,
}

struct GeneratedNativeRead {
    token: usize,
    end: unsafe extern "C" fn(*mut c_void),
    _owner: Arc<dyn Send + Sync>,
}

impl Drop for GeneratedNativeRead {
    fn drop(&mut self) {
        let token = self.token;
        let end = self.end;
        generated_finalize(move || unsafe { end(token as *mut c_void) });
    }
}

impl GeneratedReadScope {
    fn with_native<T: maplibre_core::handle::NativeHandle>(
        py: Python<'_>,
        state: Arc<Mutex<NativeHandleState<T>>>,
        begin: unsafe extern "C" fn(
            T,
            *mut *mut c_void,
            *mut sys::mln_diagnostic,
        ) -> sys::mln_status,
        end: unsafe extern "C" fn(*mut c_void),
    ) -> PyResult<Self> {
        let handle = state.lock().unwrap_or_else(|p| p.into_inner()).handle.raw();
        let mut token = ptr::null_mut();
        maplibre_core::check(|diagnostic| unsafe {
            generated_native_call(py, || begin(handle, &mut token, diagnostic))
        })
        .map_err(map_error)?;
        Ok(Self {
            lease: Mutex::new(Some(GeneratedNativeRead {
                token: token as usize,
                end,
                _owner: state,
            })),
        })
    }
}

#[pymethods]
impl GeneratedReadScope {
    fn __enter__(slf: PyRef<'_, Self>) -> PyRef<'_, Self> {
        slf
    }

    fn __exit__(
        &self,
        _kind: &Bound<'_, PyAny>,
        _value: &Bound<'_, PyAny>,
        _traceback: &Bound<'_, PyAny>,
    ) {
        let lease = self.lease.lock().unwrap_or_else(|p| p.into_inner()).take();
        drop(lease);
    }
}

#[derive(Clone)]
struct GeneratedCallbackScope {
    alive: Arc<std::sync::atomic::AtomicBool>,
    thread: std::thread::ThreadId,
}

impl GeneratedCallbackScope {
    fn pointer(&self, pointer: usize) -> PyResult<usize> {
        // A scope expires for every thread when its callback returns, so the
        // expiry is reported first, wherever the call comes from.
        if !self.alive.load(Ordering::Acquire) || pointer == 0 {
            return Err(invalid_state_error("response callback has returned"));
        }
        if self.thread != std::thread::current().id() {
            return Err(invalid_state_error(
                "response belongs to its callback thread",
            ));
        }
        Ok(pointer)
    }
}

struct GeneratedCallbackGuard(GeneratedCallbackScope);

impl GeneratedCallbackGuard {
    fn new() -> Self {
        Self(GeneratedCallbackScope {
            alive: Arc::new(std::sync::atomic::AtomicBool::new(true)),
            thread: std::thread::current().id(),
        })
    }

    fn scope(&self) -> GeneratedCallbackScope {
        self.0.clone()
    }
}

impl Drop for GeneratedCallbackGuard {
    fn drop(&mut self) {
        self.0.alive.store(false, Ordering::Release);
    }
}

fn generated_finalize(call: impl FnOnce() + Send + 'static) {
    maplibre_core::callback::finalize(move || {
        let mut call = Some(call);
        Python::try_attach(|py| py.detach(call.take().unwrap()));
        if let Some(call) = call {
            call();
        }
    });
}

/// Iterates a native extensible record array while its owner remains reserved.
unsafe fn generated_strided_values<T: Copy>(
    data: *const T,
    count: impl TryInto<usize>,
    stride: impl TryInto<usize>,
) -> PyResult<impl Iterator<Item = T>> {
    let (count, stride) = (generated_count(count)?, generated_count(stride)?);
    // SAFETY: the array contract supplies count records of the given stride.
    unsafe { maplibre_core::ptr::strided_records(data, count, stride) }.map_err(map_error)
}

fn generated_count(count: impl TryInto<usize>) -> PyResult<usize> {
    count
        .try_into()
        .map_err(|_| native_error("native count exceeds the address space"))
}

fn generated_view<P>(data: *const P, count: impl TryInto<usize>) -> PyResult<sys::mln_buffer_view> {
    Ok(sys::mln_buffer_view {
        data: data.cast(),
        size: generated_count(count)?,
    })
}

fn generated_length<C: TryFrom<usize>>(length: usize) -> PyResult<C> {
    C::try_from(length)
        .map_err(|_| pyo3::exceptions::PyOverflowError::new_err("length exceeds native count"))
}
