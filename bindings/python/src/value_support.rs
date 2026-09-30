/// Borrows Python byte storage for one C call and owns native array layouts.
///
/// Generated converters keep this on the submitting stack. The C contracts
/// require accepted operations to copy call-lifetime inputs before returning.
#[derive(Default)]
struct GeneratedInputStorage<'py> {
    roots: Vec<Bound<'py, PyAny>>,
    arrays: Vec<Box<dyn std::any::Any>>,
    callbacks: Vec<GeneratedPendingCallback>,
}

impl<'py> GeneratedInputStorage<'py> {
    fn buffer(&mut self, value: Bound<'py, PyAny>, text: bool) -> PyResult<sys::mln_buffer_view> {
        if value.is_none() {
            return Ok(sys::mln_buffer_view {
                data: ptr::null(),
                size: 0,
            });
        }
        let mut view = if text {
            maplibre_core::string::buffer_view(value.extract::<&str>()?.as_bytes())
        } else {
            maplibre_core::string::buffer_view(value.cast::<PyBytes>()?.as_bytes())
        };
        if view.data.is_null() {
            // A present empty value has a nonnull pointer; None alone denotes absence.
            view.data = std::ptr::NonNull::<u8>::dangling().as_ptr().cast();
        }
        self.roots.push(value);
        Ok(view)
    }

    fn c_string(&mut self, value: Bound<'py, PyAny>) -> PyResult<*const c_char> {
        let string =
            maplibre_core::string::c_string(value.extract::<&str>()?).map_err(map_error)?;
        let pointer = string.as_ptr();
        self.arrays.push(Box::new(string));
        Ok(pointer)
    }

    fn keep_array<T: 'static>(&mut self, values: Vec<T>) -> *const T {
        let pointer = values.as_ptr();
        self.arrays.push(Box::new(values));
        pointer
    }

    fn keep_one<T: 'static>(&mut self, value: T) -> *const T {
        let value = Box::new(value);
        let pointer = &*value as *const T;
        self.arrays.push(value);
        pointer
    }
}

/// The generated caller supplies a C-contract borrowed array during its lifetime.
unsafe fn generated_slice<'a, T>(data: *const T, count: usize) -> PyResult<&'a [T]> {
    if count == 0 {
        return Ok(&[]);
    }
    if data.is_null()
        || count > (isize::MAX as usize) / std::mem::size_of::<T>().max(1)
        || !data.is_aligned()
    {
        return Err(native_error("invalid native array"));
    }
    // SAFETY: the caller's contract supplies count initialized elements, and
    // the checks above enforce Rust's slice size, alignment, and null rules.
    Ok(unsafe { std::slice::from_raw_parts(data, count) })
}

fn generated_completion_slice<T>(result: &sys::mln_completion_result) -> PyResult<&[T]> {
    // SAFETY: the generated callback selects T from the submitting operation's
    // resolved result contract; the borrowed result bounds the returned slice.
    unsafe { generated_slice(result.value.cast::<T>(), result.value_count) }
}

/// Removes a public owner before a consuming call and restores it on rejection.
/// No state mutex remains locked while native code can invoke host callbacks.
trait GeneratedOwnerState<T: maplibre_core::handle::NativeHandle> {
    fn owner_state(&mut self) -> &mut NativeHandleState<T>;
}

impl<T: maplibre_core::handle::NativeHandle> GeneratedOwnerState<T> for NativeHandleState<T> {
    fn owner_state(&mut self) -> &mut NativeHandleState<T> {
        self
    }
}

struct GeneratedHandleReservation<
    'a,
    T: maplibre_core::handle::NativeHandle,
    S: GeneratedOwnerState<T>,
> {
    state: &'a Mutex<S>,
    handle: Option<T>,
}

impl<'a, T: maplibre_core::handle::NativeHandle, S: GeneratedOwnerState<T>>
    GeneratedHandleReservation<'a, T, S>
{
    fn new(state: &'a Mutex<S>) -> PyResult<Option<Self>> {
        let mut guard = state.lock().unwrap_or_else(|p| p.into_inner());
        let owner = guard.owner_state();
        if owner.closing || owner.active_reads != 0 {
            return Err(invalid_state_error("handle has an active read or close"));
        }
        let Some(handle) = owner.id.take() else {
            return Ok(None);
        };
        owner.closing = true;
        Ok(Some(Self {
            state,
            handle: Some(T::from_raw(handle.get())),
        }))
    }

    fn handle(&self) -> T {
        self.handle.expect("uncommitted owner reservation")
    }

    fn commit(&mut self) {
        self.state
            .lock()
            .unwrap_or_else(|p| p.into_inner())
            .owner_state()
            .closing = false;
        self.handle = None;
    }
}

impl<T: maplibre_core::handle::NativeHandle, S: GeneratedOwnerState<T>> Drop
    for GeneratedHandleReservation<'_, T, S>
{
    fn drop(&mut self) {
        if let Some(handle) = self.handle {
            let mut guard = self.state.lock().unwrap_or_else(|p| p.into_inner());
            let state = guard.owner_state();
            debug_assert!(state.id.is_none());
            state.id = std::num::NonZeroU64::new(handle.to_raw());
            state.closing = false;
        }
    }
}

struct GeneratedReadReservation<
    'a,
    T: maplibre_core::handle::NativeHandle,
    S: GeneratedOwnerState<T>,
> {
    state: &'a Mutex<S>,
    handle: T,
}

impl<'a, T: maplibre_core::handle::NativeHandle, S: GeneratedOwnerState<T>>
    GeneratedReadReservation<'a, T, S>
{
    fn new(state: &'a Mutex<S>) -> PyResult<Self> {
        let mut guard = state.lock().unwrap_or_else(|p| p.into_inner());
        let owner = guard.owner_state();
        let handle = owner
            .live_handle()
            .ok_or_else(|| invalid_state_error("handle is closed"))?;
        owner.active_reads += 1;
        Ok(Self { state, handle })
    }
}

impl<T: maplibre_core::handle::NativeHandle, S: GeneratedOwnerState<T>> Drop
    for GeneratedReadReservation<'_, T, S>
{
    fn drop(&mut self) {
        self.state
            .lock()
            .unwrap_or_else(|p| p.into_inner())
            .owner_state()
            .active_reads -= 1;
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

    fn accept_callbacks(&mut self) -> Vec<std::sync::Weak<GeneratedCallbackRoot>> {
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

impl GeneratedCallbackRootOwner {
    fn new(callbacks: Vec<Py<PyAny>>) -> Self {
        Self(Arc::new(Mutex::new(callbacks)))
    }

    /// Lets an owner visit the callbacks for garbage collection.
    fn downgrade(&self) -> std::sync::Weak<GeneratedCallbackRoot> {
        Arc::downgrade(&self.0)
    }

    fn get(&self, py: Python<'_>, index: usize) -> Option<Py<PyAny>> {
        generated_root_callback(py, &self.0, index)
    }
}

impl Drop for GeneratedCallbackRootOwner {
    fn drop(&mut self) {
        let callbacks = std::mem::take(&mut *self.0.lock().unwrap_or_else(|p| p.into_inner()));
        let mut callbacks = Some(callbacks);
        Python::try_attach(|_| drop(callbacks.take()));
        drop(callbacks);
    }
}

unsafe extern "C" fn generated_release_callbacks(user_data: *mut c_void) {
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

struct GeneratedOwnedRead<T: maplibre_core::handle::NativeHandle, S: GeneratedOwnerState<T>> {
    state: Arc<Mutex<S>>,
    _handle: std::marker::PhantomData<fn() -> T>,
}

impl<T: maplibre_core::handle::NativeHandle, S: GeneratedOwnerState<T>> Drop
    for GeneratedOwnedRead<T, S>
{
    fn drop(&mut self) {
        self.state
            .lock()
            .unwrap_or_else(|p| p.into_inner())
            .owner_state()
            .active_reads -= 1;
    }
}

#[pyclass]
struct GeneratedReadScope {
    lease: Mutex<Option<Box<dyn Send>>>,
}

struct GeneratedNativeRead {
    token: usize,
    end: unsafe extern "C" fn(*mut c_void),
}

impl Drop for GeneratedNativeRead {
    fn drop(&mut self) {
        let token = self.token;
        let end = self.end;
        generated_finalize(move || unsafe { end(token as *mut c_void) });
    }
}

impl GeneratedReadScope {
    fn with_native<T, S>(
        py: Python<'_>,
        state: Arc<Mutex<S>>,
        begin: unsafe extern "C" fn(
            T,
            *mut *mut c_void,
            *mut sys::mln_diagnostic,
        ) -> sys::mln_status,
        end: unsafe extern "C" fn(*mut c_void),
    ) -> PyResult<Self>
    where
        T: maplibre_core::handle::NativeHandle + 'static,
        S: GeneratedOwnerState<T> + Send + 'static,
    {
        let scope = Self::new::<T, S>(Arc::clone(&state))?;
        let handle = state
            .lock()
            .unwrap_or_else(|p| p.into_inner())
            .owner_state()
            .live_handle()
            .ok_or_else(|| invalid_state_error("borrowed view owner is closed"))?;
        let mut token = ptr::null_mut();
        maplibre_core::check(|diagnostic| unsafe {
            generated_native_call(py, || begin(handle, &mut token, diagnostic))
        })
        .map_err(map_error)?;
        let mut lease = scope.lease.lock().unwrap_or_else(|p| p.into_inner());
        let owner = lease.take();
        *lease = Some(Box::new((
            GeneratedNativeRead {
                token: token as usize,
                end,
            },
            owner,
        )));
        drop(lease);
        Ok(scope)
    }

    fn new<T, S>(state: Arc<Mutex<S>>) -> PyResult<Self>
    where
        T: maplibre_core::handle::NativeHandle + 'static,
        S: GeneratedOwnerState<T> + Send + 'static,
    {
        {
            let mut guard = state.lock().unwrap_or_else(|p| p.into_inner());
            let owner = guard.owner_state();
            if owner.live_handle().is_none() || !owner.views_valid {
                return Err(invalid_state_error("borrowed view owner is no longer live"));
            }
            owner.active_reads += 1;
        }
        Ok(Self {
            lease: Mutex::new(Some(Box::new(GeneratedOwnedRead::<T, S> {
                state,
                _handle: std::marker::PhantomData,
            }))),
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

type GeneratedCallbackPolicy = maplibre_core::callback::PolicyScope;

fn generated_check_reentry() -> PyResult<()> {
    generated_check_operation("", 0)
}

fn generated_check_operation(operation: &str, owner: u64) -> PyResult<()> {
    maplibre_core::callback::check(operation, owner).map_err(map_error)
}

unsafe extern "C" fn generated_release_callbacks_no_reentry(user_data: *mut c_void) {
    let _guard = GeneratedCallbackPolicy::enter(&[], 0);
    unsafe { generated_release_callbacks(user_data) };
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
    count: usize,
    stride: usize,
) -> PyResult<impl Iterator<Item = T>> {
    if count != 0
        && (data.is_null()
            || stride < std::mem::size_of::<T>()
            || count > isize::MAX as usize / stride)
    {
        return Err(native_error("invalid native record stride"));
    }
    Ok((0..count).map(move |index| {
        // SAFETY: the array contract supplies count records of the given stride.
        unsafe {
            data.cast::<u8>()
                .add(index * stride)
                .cast::<T>()
                .read_unaligned()
        }
    }))
}

unsafe fn generated_arena_string(
    data: *const u8,
    size: usize,
    offset: usize,
    length: usize,
) -> PyResult<String> {
    if offset > size || length > size - offset {
        return Err(native_error("native message is outside its arena"));
    }
    let arena = unsafe { generated_slice(data, size)? };
    std::str::from_utf8(&arena[offset..offset + length])
        .map(str::to_owned)
        .map_err(|_| native_error("native message is not UTF-8"))
}
