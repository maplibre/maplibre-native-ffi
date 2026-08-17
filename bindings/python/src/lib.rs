#![deny(unsafe_op_in_unsafe_fn)]

use maplibre_native_ffi_core::{
    self as maplibre_core, Error, ErrorKind, LogEvent, LogSeverity, NetworkStatus, RenderMode,
    ResourceErrorReason, ResourceResponseStatus, RuntimeEventPayload, RuntimeEventType,
    TileOperation,
};
use maplibre_native_ffi_sys as sys;
use pyo3::prelude::*;
use pyo3::types::{PyAny, PyBytes, PyDict, PyList};
use std::collections::VecDeque;
use std::ffi::{c_char, c_void};
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Condvar, Mutex, MutexGuard};

/// Wire shape for `maplibre_native_ffi.camera.AnimationOptions`.
///
/// Ordered as duration in milliseconds, velocity, minimum zoom, easing control
/// points, and the caller-chosen transition ID.
type AnimationParts = (
    Option<f64>,
    Option<f64>,
    Option<f64>,
    Option<(f64, f64, f64, f64)>,
    Option<u64>,
);

mod py_errors {
    pyo3::import_exception!(maplibre_native_ffi.errors, BusyError);
    pyo3::import_exception!(maplibre_native_ffi.errors, CancelledError);
    pyo3::import_exception!(maplibre_native_ffi.errors, InvalidArgumentError);
    pyo3::import_exception!(maplibre_native_ffi.errors, _OperationResultConsumedError);
    pyo3::import_exception!(maplibre_native_ffi.errors, InvalidStateError);
    pyo3::import_exception!(maplibre_native_ffi.errors, NativeError);
    pyo3::import_exception!(maplibre_native_ffi.errors, NotFoundError);
    pyo3::import_exception!(maplibre_native_ffi.errors, NotReadyError);
    pyo3::import_exception!(maplibre_native_ffi.errors, TargetLostError);
    pyo3::import_exception!(maplibre_native_ffi.errors, UnknownStatusError);
    pyo3::import_exception!(maplibre_native_ffi.errors, UnsupportedFeatureError);
    pyo3::import_exception!(maplibre_native_ffi.errors, WrongThreadError);
}

#[pyclass(name = "_RuntimeHandle")]
struct RuntimeHandle {
    state: Mutex<maplibre_core::handle::NativeHandleState<sys::mln_runtime>>,
    notification_source: Mutex<sys::mln_notification_source>,
    operation_gate: RuntimeOperationGate,
    notification_callback: Mutex<Option<Box<PyNotificationCallbackState>>>,
}

#[derive(Debug)]
struct RuntimeOperationGate {
    state: Mutex<RuntimeOperationGateState>,
}

#[derive(Debug, Default)]
struct RuntimeOperationGateState {
    closing: bool,
    closed: bool,
}

struct PyNotificationCallbackState {
    callback: Py<PyAny>,
}
struct PyResourceProviderState {
    callback: Py<PyAny>,
    pending_callbacks: AtomicUsize,
    max_pending_callbacks: usize,
}

struct PyResourceTransformState {
    callback: Py<PyAny>,
    pending_callbacks: AtomicUsize,
    max_pending_callbacks: usize,
}

struct PyHttpHeaderTransformState {
    callback: Py<PyAny>,
    pending_callbacks: AtomicUsize,
    max_pending_callbacks: usize,
}

#[pyclass(name = "_ResourceRequestHandle")]
struct ResourceRequestHandle {
    state: Arc<maplibre_core::resource::ResourceRequestHandleState>,
}

#[derive(Debug, Clone)]
struct CopiedLogRecordRaw {
    severity: u32,
    event: u32,
    code: i64,
    message: String,
}

#[derive(Debug)]
struct PyLogCallbackState {
    queue: Mutex<VecDeque<CopiedLogRecordRaw>>,
    max_queued_records: usize,
    dropped_records: AtomicUsize,
    consume: bool,
}

#[derive(Debug)]
struct GlobalPyLogCallbackState {
    current: Option<Arc<PyLogCallbackState>>,
    retired: Vec<Arc<PyLogCallbackState>>,
}

static LOG_CALLBACK_STATE: Mutex<GlobalPyLogCallbackState> = Mutex::new(GlobalPyLogCallbackState {
    current: None,
    retired: Vec::new(),
});

#[pyclass(name = "_LogReceiver")]
struct LogReceiver {
    state: Arc<PyLogCallbackState>,
}

#[pyclass(name = "_MapHandle")]
struct MapHandle {
    state: Mutex<maplibre_core::handle::NativeHandleState<sys::mln_map>>,
}

#[pyclass(name = "_MapProjectionHandle")]
struct MapProjectionHandle {
    state: Mutex<maplibre_core::handle::NativeHandleState<sys::mln_map_projection>>,
}

#[pyclass(name = "_GeoJsonSourceDataHandle")]
struct GeoJsonSourceDataHandle {
    state: Mutex<maplibre_core::handle::NativeHandleState<sys::mln_geojson_source_data>>,
}

#[derive(Debug, Clone, Copy)]
struct CustomGeometryEvent {
    kind: u32,
    tile_id: sys::mln_canonical_tile_id,
}

#[derive(Debug)]
struct CustomGeometryQueue {
    events: VecDeque<CustomGeometryEvent>,
    dropped_events: u64,
    active_callbacks: usize,
    closing: bool,
    closed: bool,
}

impl CustomGeometryQueue {
    fn new() -> Self {
        Self {
            events: VecDeque::new(),
            dropped_events: 0,
            active_callbacks: 0,
            closing: false,
            closed: false,
        }
    }
}

#[derive(Debug)]
struct PyCustomGeometrySourceShared {
    queue: Mutex<CustomGeometryQueue>,
    idle: Condvar,
    max_queued_events: usize,
}

struct PyCustomGeometrySourceState {
    shared: Arc<PyCustomGeometrySourceShared>,
    min_zoom: Option<f64>,
    max_zoom: Option<f64>,
    tolerance: Option<f64>,
    tile_size: Option<u32>,
    buffer: Option<u32>,
    clip: Option<bool>,
    wrap: Option<bool>,
    has_cancel_tile: bool,
}

#[pyclass(name = "_CustomGeometrySourceHandle")]
struct CustomGeometrySourceHandle {
    shared: Arc<PyCustomGeometrySourceShared>,
}

struct RenderSessionState {
    handle: maplibre_core::handle::NativeHandleState<sys::mln_render_session>,
}

#[pyclass(name = "_RenderSessionHandle")]
struct RenderSessionHandle {
    state: Arc<Mutex<RenderSessionState>>,
}

#[pyclass(name = "_MetalOwnedTextureFrameHandle")]
struct MetalOwnedTextureFrameHandle {
    frame: Mutex<sys::mln_acquired_frame>,
}

#[pyclass(name = "_VulkanOwnedTextureFrameHandle")]
struct VulkanOwnedTextureFrameHandle {
    frame: Mutex<sys::mln_acquired_frame>,
}

#[pyclass(name = "_WebGPUOwnedTextureFrameHandle")]
struct WebGPUOwnedTextureFrameHandle {
    frame: Mutex<sys::mln_acquired_frame>,
}
#[pyclass(name = "_OpenGLOwnedTextureFrameHandle")]
struct OpenGLOwnedTextureFrameHandle {
    frame: Mutex<sys::mln_acquired_frame>,
}

impl RuntimeHandle {
    fn state(&self) -> MutexGuard<'_, maplibre_core::handle::NativeHandleState<sys::mln_runtime>> {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
    }

    fn state_for_operation(
        &self,
    ) -> PyResult<MutexGuard<'_, maplibre_core::handle::NativeHandleState<sys::mln_runtime>>> {
        self.operation_gate.ensure_open()?;
        let state = self.state();
        self.operation_gate.ensure_open()?;
        Ok(state)
    }
}

impl RuntimeOperationGate {
    fn new() -> Self {
        Self {
            state: Mutex::new(RuntimeOperationGateState::default()),
        }
    }

    fn ensure_open(&self) -> PyResult<()> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if state.closed {
            return Err(invalid_state_error("runtime handle is closed"));
        }
        if state.closing {
            return Err(invalid_state_error("runtime is closing"));
        }
        Ok(())
    }

    fn begin_close(&self) -> PyResult<bool> {
        let mut state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if state.closed {
            return Ok(false);
        }
        if state.closing {
            return Err(invalid_state_error("runtime is closing"));
        }
        state.closing = true;
        Ok(true)
    }

    fn finish_successful_close(&self) {
        let mut state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.closing = false;
        state.closed = true;
    }

    fn finish_failed_close(&self) {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .closing = false;
    }

    fn is_closed(&self) -> bool {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .closed
    }
}

unsafe extern "C" fn notification_callback_trampoline(user_data: *mut c_void) {
    if user_data.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: user_data points to boxed state retained until native clears
        // or closes the callback and waits for every in-flight invocation.
        let state = unsafe { &*user_data.cast::<PyNotificationCallbackState>() };
        Python::attach(|py| {
            if let Err(error) = state.callback.bind(py).call0() {
                error.write_unraisable(py, None);
            }
        });
    }));
}

struct OwnedOperation(sys::mln_operation);

impl Drop for OwnedOperation {
    fn drop(&mut self) {
        unsafe { sys::mln_operation_release(self.0) };
    }
}

struct OwnedReadyBatch(sys::mln_ready_batch);

impl Drop for OwnedReadyBatch {
    fn drop(&mut self) {
        // SAFETY: this wrapper owns the batch until drop.
        unsafe { sys::mln_ready_batch_release(self.0) };
    }
}

struct OwnedRenderFrameBatch(sys::mln_render_frame_batch);

impl Drop for OwnedRenderFrameBatch {
    fn drop(&mut self) {
        unsafe { sys::mln_render_frame_batch_release(self.0) };
    }
}
fn wait_operation(py: Python<'_>, operation: sys::mln_operation) -> PyResult<()> {
    let mut completed = false;
    let status = py.detach(|| unsafe { sys::mln_operation_wait(operation, -1, &mut completed) });
    maplibre_core::check(status).map_err(map_error)?;
    if !completed {
        return Err(invalid_state_error(
            "unbounded native operation wait returned before completion",
        ));
    }

    let mut operation_status = sys::MLN_STATUS_OK;
    maplibre_core::check(unsafe {
        sys::mln_operation_get_status(operation, &mut operation_status)
    })
    .map_err(map_error)?;
    if operation_status == sys::MLN_STATUS_OK {
        return Ok(());
    }

    let mut diagnostic_size = 0;
    maplibre_core::check(unsafe {
        sys::mln_operation_copy_diagnostic(operation, ptr::null_mut(), 0, &mut diagnostic_size)
    })
    .map_err(map_error)?;
    let mut diagnostic = vec![0u8; diagnostic_size];
    maplibre_core::check(unsafe {
        sys::mln_operation_copy_diagnostic(
            operation,
            diagnostic.as_mut_ptr().cast(),
            diagnostic.len(),
            &mut diagnostic_size,
        )
    })
    .map_err(map_error)?;
    diagnostic.truncate(diagnostic_size);
    Err(map_error(Error::from_status_and_diagnostic(
        operation_status,
        String::from_utf8_lossy(&diagnostic).into_owned(),
    )))
}

fn start_offline_operation<F>(runtime: &RuntimeHandle, start: F) -> PyResult<u64>
where
    F: FnOnce(sys::mln_runtime, *mut sys::mln_operation) -> i32,
{
    let state = runtime.state_for_operation()?;
    let mut operation = sys::mln_operation(0);
    maplibre_core::check(start(state.handle(), &mut operation)).map_err(map_error)?;
    Ok(operation.0)
}

fn leak_optional_box<T>(slot: &Mutex<Option<Box<T>>>) {
    let Some(value) = slot
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
        .take()
    else {
        return;
    };
    Box::leak(value);
}

impl Drop for RuntimeHandle {
    fn drop(&mut self) {
        let native_live = self
            .state
            .lock()
            .map(|state| !state.is_closed())
            .unwrap_or(true);
        if native_live {
            leak_optional_box(&self.notification_callback);
        }
    }
}

impl PyLogCallbackState {
    fn new(max_queued_records: usize, consume: bool) -> Arc<Self> {
        Arc::new(Self {
            queue: Mutex::new(VecDeque::new()),
            max_queued_records,
            dropped_records: AtomicUsize::new(0),
            consume,
        })
    }

    fn push(&self, record: CopiedLogRecordRaw) -> u32 {
        let mut queue = self
            .queue
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if queue.len() >= self.max_queued_records {
            self.dropped_records.fetch_add(1, Ordering::AcqRel);
        } else {
            queue.push_back(record);
        }
        u32::from(self.consume)
    }
}

impl MapHandle {
    fn state(&self) -> MutexGuard<'_, maplibre_core::handle::NativeHandleState<sys::mln_map>> {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
    }

    #[allow(clippy::too_many_arguments)]
    fn add_tile_source_url_with(
        &self,
        source_id: String,
        url: String,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        attribution: Option<String>,
        scheme: Option<u32>,
        bounds: Option<((f64, f64), (f64, f64))>,
        tile_size: Option<u32>,
        vector_encoding: Option<u32>,
        raster_dem_encoding: Option<u32>,
        add: unsafe extern "C" fn(
            sys::mln_map,
            sys::mln_buffer_view,
            sys::mln_buffer_view,
            *const sys::mln_style_tile_source_options,
            *mut u64,
        ) -> sys::mln_status,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let url = maplibre_core::string::string_view(&url);
        let options = tile_source_options_from_parts(
            min_zoom,
            max_zoom,
            attribution,
            scheme,
            bounds,
            tile_size,
            vector_encoding,
            raster_dem_encoding,
        )?;
        let options = maplibre_core::style::tile_source_options_to_native(&options);
        self.run_style_command(|map, out| unsafe {
            add(map, source_id.raw(), url.raw(), options.as_ptr(), out)
        })
    }

    #[allow(clippy::too_many_arguments)]
    fn add_tile_source_tiles_with(
        &self,
        source_id: String,
        tiles: Vec<String>,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        attribution: Option<String>,
        scheme: Option<u32>,
        bounds: Option<((f64, f64), (f64, f64))>,
        tile_size: Option<u32>,
        vector_encoding: Option<u32>,
        raster_dem_encoding: Option<u32>,
        add: unsafe extern "C" fn(
            sys::mln_map,
            sys::mln_buffer_view,
            *const sys::mln_buffer_view,
            usize,
            *const sys::mln_style_tile_source_options,
            *mut u64,
        ) -> sys::mln_status,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let tiles = maplibre_core::style::NativeTileUrls::new(&tiles);
        let options = tile_source_options_from_parts(
            min_zoom,
            max_zoom,
            attribution,
            scheme,
            bounds,
            tile_size,
            vector_encoding,
            raster_dem_encoding,
        )?;
        let options = maplibre_core::style::tile_source_options_to_native(&options);
        self.run_style_command(|map, out| unsafe {
            add(
                map,
                source_id.raw(),
                tiles.as_ptr(),
                tiles.len(),
                options.as_ptr(),
                out,
            )
        })
    }

    fn run_style_operation<T>(
        &self,
        py: Python<'_>,
        start: impl FnOnce(sys::mln_map, *mut sys::mln_operation) -> sys::mln_status,
        take: impl FnOnce(sys::mln_operation) -> PyResult<T>,
    ) -> PyResult<T> {
        let state = self.state();
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(start(state.handle(), &mut operation)).map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        take(operation.0)
    }

    fn run_style_command(
        &self,
        accept: impl FnOnce(sys::mln_map, *mut u64) -> sys::mln_status,
    ) -> PyResult<u64> {
        let state = self.state();
        let mut command_id = 0;
        maplibre_core::check(accept(state.handle(), &mut command_id)).map_err(map_error)?;
        Ok(command_id)
    }
}

impl MapProjectionHandle {
    fn state(
        &self,
    ) -> MutexGuard<'_, maplibre_core::handle::NativeHandleState<sys::mln_map_projection>> {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
    }
}

impl GeoJsonSourceDataHandle {
    fn state(
        &self,
    ) -> MutexGuard<'_, maplibre_core::handle::NativeHandleState<sys::mln_geojson_source_data>>
    {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
    }
}

#[pymethods]
impl GeoJsonSourceDataHandle {
    fn close(&self) {
        let state = self.state();
        // SAFETY: state owns an mln_geojson_source_data handle created by
        // mln_geojson_source_data_create and pairs it with the matching
        // destroy function, which accepts calls from any thread. Holding the
        // state mutex orders this close after any install call that already
        // borrowed the live handle.
        unsafe { state.close_infallible(sys::mln_geojson_source_data_destroy) };
    }

    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
}

impl PyCustomGeometrySourceShared {
    fn new(max_queued_events: usize) -> Arc<Self> {
        Arc::new(Self {
            queue: Mutex::new(CustomGeometryQueue::new()),
            idle: Condvar::new(),
            max_queued_events,
        })
    }

    fn enqueue(&self, kind: u32, tile_id: sys::mln_canonical_tile_id) {
        let mut queue = self
            .queue
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if queue.closing || queue.closed {
            return;
        }
        if queue.events.len() >= self.max_queued_events {
            queue.dropped_events = queue.dropped_events.saturating_add(1);
        } else {
            queue
                .events
                .push_back(CustomGeometryEvent { kind, tile_id });
        }
    }

    fn enter_callback(&self) -> Option<CustomGeometryCallbackGuard<'_>> {
        let mut queue = self
            .queue
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if queue.closing || queue.closed {
            return None;
        }
        queue.active_callbacks += 1;
        Some(CustomGeometryCallbackGuard { shared: self })
    }

    fn exit_callback(&self) {
        let mut queue = self
            .queue
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        queue.active_callbacks -= 1;
        if queue.active_callbacks == 0 {
            self.idle.notify_all();
        }
    }

    fn close(&self) {
        let mut queue = self
            .queue
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if queue.closed {
            return;
        }
        queue.closing = true;
        while queue.active_callbacks != 0 {
            queue = self
                .idle
                .wait(queue)
                .unwrap_or_else(|poisoned| poisoned.into_inner());
        }
        queue.events.clear();
        queue.closed = true;
    }
}

struct CustomGeometryCallbackGuard<'a> {
    shared: &'a PyCustomGeometrySourceShared,
}

impl Drop for CustomGeometryCallbackGuard<'_> {
    fn drop(&mut self) {
        self.shared.exit_callback();
    }
}

impl PyCustomGeometrySourceState {
    #[allow(clippy::too_many_arguments)]
    fn new(
        max_queued_events: usize,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        tolerance: Option<f64>,
        tile_size: Option<u32>,
        buffer: Option<u32>,
        clip: Option<bool>,
        wrap: Option<bool>,
        has_cancel_tile: bool,
    ) -> Box<Self> {
        Box::new(Self {
            shared: PyCustomGeometrySourceShared::new(max_queued_events),
            min_zoom,
            max_zoom,
            tolerance,
            tile_size,
            buffer,
            clip,
            wrap,
            has_cancel_tile,
        })
    }

    fn descriptor(&self) -> sys::mln_custom_geometry_source_options {
        maplibre_core::style::custom_geometry_source_options_to_native(
            maplibre_core::style::CustomGeometrySourceDescriptorFields {
                fetch_tile: Some(custom_geometry_fetch_tile_trampoline),
                cancel_tile: self
                    .has_cancel_tile
                    .then_some(custom_geometry_cancel_tile_trampoline as _),
                release_user_data: Some(custom_geometry_release_trampoline),
                user_data: ptr::from_ref(self).cast_mut().cast::<c_void>(),
                min_zoom: self.min_zoom,
                max_zoom: self.max_zoom,
                tolerance: self.tolerance,
                tile_size: self.tile_size,
                buffer: self.buffer,
                clip: self.clip,
                wrap: self.wrap,
            },
        )
    }
}

impl Drop for PyCustomGeometrySourceState {
    fn drop(&mut self) {
        self.shared.close();
    }
}

/// Takes back the callback state the C API stopped referencing.
///
/// The C API calls this once after the source is removed, dropped by a style
/// load, or retired with the map.
unsafe extern "C" fn custom_geometry_release_trampoline(user_data: *mut c_void) {
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let Some(state) = ptr::NonNull::new(user_data.cast::<PyCustomGeometrySourceState>()) else {
            return;
        };
        // SAFETY: user_data is the Box this binding leaked into a successful
        // mln_map_add_custom_geometry_source, and the C API releases it once.
        drop(unsafe { Box::from_raw(state.as_ptr()) });
    }));
}

unsafe extern "C" fn custom_geometry_fetch_tile_trampoline(
    user_data: *mut c_void,
    tile_id: sys::mln_canonical_tile_id,
) {
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let Some(state) = ptr::NonNull::new(user_data.cast::<PyCustomGeometrySourceState>()) else {
            return;
        };
        // SAFETY: user_data points to PyCustomGeometrySourceState retained by the
        // map until source/style/map teardown waits for in-flight callbacks.
        let state = unsafe { state.as_ref() };
        let Some(_guard) = state.shared.enter_callback() else {
            return;
        };
        state.shared.enqueue(0, tile_id);
    }));
}

unsafe extern "C" fn custom_geometry_cancel_tile_trampoline(
    user_data: *mut c_void,
    tile_id: sys::mln_canonical_tile_id,
) {
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let Some(state) = ptr::NonNull::new(user_data.cast::<PyCustomGeometrySourceState>()) else {
            return;
        };
        // SAFETY: user_data points to PyCustomGeometrySourceState retained by the
        // map until source/style/map teardown waits for in-flight callbacks.
        let state = unsafe { state.as_ref() };
        let Some(_guard) = state.shared.enter_callback() else {
            return;
        };
        state.shared.enqueue(1, tile_id);
    }));
}

impl RenderSessionState {
    fn new(native: sys::mln_render_session) -> PyResult<Self> {
        // SAFETY: native came from a successful render-session attach function
        // and is paired with mln_render_session_destroy in close.
        let handle = unsafe {
            maplibre_core::handle::NativeHandleState::from_handle(native, "mln_render_session")
        }
        .map_err(map_error)?;
        Ok(Self { handle })
    }

    fn native(&self) -> sys::mln_render_session {
        self.handle.handle()
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }
}
fn operation_result_consumed(error: PyErr) -> PyErr {
    py_errors::_OperationResultConsumedError::new_err(error.to_string())
}

#[pymethods]
impl RuntimeHandle {
    fn close(&self, py: Python<'_>) -> PyResult<()> {
        if !self.operation_gate.begin_close()? {
            return Ok(());
        }
        {
            let state = self.state();
            let Some(runtime_handle) = state.live_handle() else {
                self.operation_gate.finish_successful_close();
                return Ok(());
            };
            if let Err(error) = py.detach(|| {
                maplibre_core::check(unsafe { sys::mln_runtime_release(runtime_handle) })
            }) {
                self.operation_gate.finish_failed_close();
                return Err(map_error(error));
            }
            state.mark_closed();
        }

        let mut callback = self
            .notification_callback
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let source = *self
            .notification_source
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if source.0 != 0 {
            py.detach(|| unsafe { sys::mln_notification_source_release(source) });
            *self
                .notification_source
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner()) = sys::mln_notification_source(0);
        }
        callback.take();
        self.operation_gate.finish_successful_close();
        Ok(())
    }

    fn barrier(&self, py: Python<'_>) -> PyResult<()> {
        let state = self.state_for_operation()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_runtime_barrier_start(state.handle(), &mut operation)
        })
        .map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)
    }

    fn set_notification_callback(&self, py: Python<'_>, callback: Py<PyAny>) -> PyResult<()> {
        self.operation_gate.ensure_open()?;
        let replacement = Box::new(PyNotificationCallbackState { callback });
        let user_data = ptr::from_ref(&*replacement).cast_mut().cast::<c_void>() as usize;
        let mut callback_slot = self
            .notification_callback
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let source = *self
            .notification_source
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let status = py.detach(|| unsafe {
            sys::mln_notification_source_set_callback(
                source,
                Some(notification_callback_trampoline),
                user_data as *mut c_void,
            )
        });
        maplibre_core::check(status).map_err(map_error)?;
        callback_slot.replace(replacement);
        Ok(())
    }

    fn clear_notification_callback(&self, py: Python<'_>) -> PyResult<()> {
        self.operation_gate.ensure_open()?;
        let mut callback_slot = self
            .notification_callback
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let source = *self
            .notification_source
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let status = py.detach(|| unsafe { sys::mln_notification_source_clear_callback(source) });
        maplibre_core::check(status).map_err(map_error)?;
        callback_slot.take();
        Ok(())
    }

    fn drain_ready(&self) -> PyResult<Vec<(u32, u64)>> {
        self.operation_gate.ensure_open()?;
        let source = *self
            .notification_source
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let mut batch = sys::mln_ready_batch(0);
        maplibre_core::check(unsafe {
            sys::mln_notification_source_drain_ready(source, &mut batch)
        })
        .map_err(map_error)?;
        let batch = OwnedReadyBatch(batch);
        let mut view = sys::mln_ready_batch_view {
            size: std::mem::size_of::<sys::mln_ready_batch_view>() as u32,
            endpoint_size: 0,
            endpoints: ptr::null(),
            endpoint_count: 0,
        };
        maplibre_core::check(unsafe { sys::mln_ready_batch_get(batch.0, &mut view) })
            .map_err(map_error)?;
        if view.endpoint_size < std::mem::size_of::<sys::mln_ready_endpoint>() as u32
            || (view.endpoint_count != 0 && view.endpoints.is_null())
        {
            return Err(py_errors::NativeError::new_err((
                Option::<i32>::None,
                "native returned an invalid ready-batch view",
            )));
        }
        let mut endpoints = Vec::with_capacity(view.endpoint_count);
        for index in 0..view.endpoint_count {
            // SAFETY: the validated view contains endpoint_count records at
            // endpoint_size strides until batch drops.
            let endpoint = unsafe {
                ptr::read_unaligned(
                    view.endpoints
                        .cast::<u8>()
                        .add(index * view.endpoint_size as usize)
                        .cast::<sys::mln_ready_endpoint>(),
                )
            };
            endpoints.push((endpoint.kind, endpoint.id));
        }
        Ok(endpoints)
    }

    fn run_ambient_cache_operation_start(&self, ambient_operation: u32) -> PyResult<u64> {
        let state = self.state_for_operation()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: The C API validates the runtime handle, operation enum value,
        // and writable operation pointer.
        maplibre_core::check(unsafe {
            sys::mln_runtime_run_ambient_cache_operation_start(
                state.handle(),
                ambient_operation,
                &mut operation,
            )
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn set_maximum_ambient_cache_size_start(&self, size: u64) -> PyResult<u64> {
        let state = self.state_for_operation()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: The C API validates the runtime handle and writable operation
        // pointer.
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_maximum_ambient_cache_size_start(
                state.handle(),
                size,
                &mut operation,
            )
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn offline_region_create_start(
        &self,
        definition: &Bound<'_, PyAny>,
        metadata: Vec<u8>,
    ) -> PyResult<u64> {
        let state = self.state_for_operation()?;
        let definition = offline_region_definition_from_wire(definition)?;
        let definition = maplibre_core::runtime::offline_region_definition_to_native(&definition)
            .map_err(map_error)?;
        let raw = definition.to_raw();
        let mut operation = sys::mln_operation(0);
        // SAFETY: The C API validates the runtime, definition, metadata pointer/length, and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_create_start(
                state.handle(),
                &raw,
                maplibre_core::runtime::metadata_ptr(&metadata),
                metadata.len(),
                &mut operation,
            )
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn offline_region_get_start(&self, region_id: i64) -> PyResult<u64> {
        start_offline_operation(self, |runtime, out| unsafe {
            sys::mln_runtime_offline_region_get_start(runtime, region_id, out)
        })
    }

    fn offline_regions_list_start(&self) -> PyResult<u64> {
        start_offline_operation(self, |runtime, out| unsafe {
            sys::mln_runtime_offline_regions_list_start(runtime, out)
        })
    }

    fn offline_regions_merge_database_start(&self, side_database_path: String) -> PyResult<u64> {
        let path = maplibre_core::string::c_string(&side_database_path).map_err(map_error)?;
        start_offline_operation(self, |runtime, out| unsafe {
            sys::mln_runtime_offline_regions_merge_database_start(runtime, path.as_ptr(), out)
        })
    }

    fn offline_region_update_metadata_start(
        &self,
        region_id: i64,
        metadata: Vec<u8>,
    ) -> PyResult<u64> {
        start_offline_operation(self, |runtime, out| unsafe {
            sys::mln_runtime_offline_region_update_metadata_start(
                runtime,
                region_id,
                maplibre_core::runtime::metadata_ptr(&metadata),
                metadata.len(),
                out,
            )
        })
    }

    fn offline_region_get_status_start(&self, region_id: i64) -> PyResult<u64> {
        start_offline_operation(self, |runtime, out| unsafe {
            sys::mln_runtime_offline_region_get_status_start(runtime, region_id, out)
        })
    }

    fn offline_region_set_observed_start(&self, region_id: i64, observed: bool) -> PyResult<u64> {
        start_offline_operation(self, |runtime, out| unsafe {
            sys::mln_runtime_offline_region_set_observed_start(runtime, region_id, observed, out)
        })
    }

    fn offline_region_set_download_state_start(&self, region_id: i64, state: u32) -> PyResult<u64> {
        start_offline_operation(self, |runtime, out| unsafe {
            sys::mln_runtime_offline_region_set_download_state_start(runtime, region_id, state, out)
        })
    }

    fn offline_region_invalidate_start(&self, region_id: i64) -> PyResult<u64> {
        start_offline_operation(self, |runtime, out| unsafe {
            sys::mln_runtime_offline_region_invalidate_start(runtime, region_id, out)
        })
    }

    fn offline_region_delete_start(&self, region_id: i64) -> PyResult<u64> {
        start_offline_operation(self, |runtime, out| unsafe {
            sys::mln_runtime_offline_region_delete_start(runtime, region_id, out)
        })
    }

    fn operation_poll(&self, operation: u64) -> PyResult<bool> {
        let mut completed = false;
        maplibre_core::check(unsafe {
            sys::mln_operation_poll(sys::mln_operation(operation), &mut completed)
        })
        .map_err(map_error)?;
        Ok(completed)
    }

    fn operation_wait(&self, py: Python<'_>, operation: u64, timeout_ms: i64) -> PyResult<bool> {
        let mut completed = false;
        let status = py.detach(|| unsafe {
            sys::mln_operation_wait(sys::mln_operation(operation), timeout_ms, &mut completed)
        });
        maplibre_core::check(status).map_err(map_error)?;
        Ok(completed)
    }

    fn operation_cancel(&self, operation: u64) -> PyResult<()> {
        maplibre_core::check(unsafe { sys::mln_operation_cancel(sys::mln_operation(operation)) })
            .map_err(map_error)
    }

    fn operation_status(&self, operation: u64) -> PyResult<(sys::mln_status, String)> {
        let mut operation_status = sys::MLN_STATUS_OK;
        maplibre_core::check(unsafe {
            sys::mln_operation_get_status(sys::mln_operation(operation), &mut operation_status)
        })
        .map_err(map_error)?;

        let mut diagnostic_size = 0;
        maplibre_core::check(unsafe {
            sys::mln_operation_copy_diagnostic(
                sys::mln_operation(operation),
                ptr::null_mut(),
                0,
                &mut diagnostic_size,
            )
        })
        .map_err(map_error)?;
        let mut diagnostic = vec![0u8; diagnostic_size];
        maplibre_core::check(unsafe {
            sys::mln_operation_copy_diagnostic(
                sys::mln_operation(operation),
                diagnostic.as_mut_ptr().cast(),
                diagnostic.len(),
                &mut diagnostic_size,
            )
        })
        .map_err(map_error)?;
        diagnostic.truncate(diagnostic_size);
        Ok((
            operation_status,
            String::from_utf8_lossy(&diagnostic).into_owned(),
        ))
    }

    fn operation_release(&self, operation: u64) {
        unsafe { sys::mln_operation_release(sys::mln_operation(operation)) };
    }

    fn offline_region_create_take_result(
        &self,
        py: Python<'_>,
        operation_id: u64,
    ) -> PyResult<Py<PyAny>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_snapshot>::new();
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_create_take_result(
                sys::mln_operation(operation_id),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        (|| {
            let native = out
                .into_live("mln_offline_region_snapshot")
                .map_err(map_error)?;
            let info = unsafe { maplibre_core::runtime::copy_offline_region_snapshot(native) }
                .map_err(map_error)?;
            offline_region_info_to_py(py, &info)
        })()
        .map_err(operation_result_consumed)
    }

    fn offline_region_get_take_result(
        &self,
        py: Python<'_>,
        operation_id: u64,
    ) -> PyResult<Option<Py<PyAny>>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_snapshot>::new();
        let mut found = false;
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_get_take_result(
                sys::mln_operation(operation_id),
                out.as_mut_ptr(),
                &mut found,
            )
        })
        .map_err(map_error)?;
        if !found {
            return Ok(None);
        }
        (|| {
            let native = out
                .into_live("mln_offline_region_snapshot")
                .map_err(map_error)?;
            let info = unsafe { maplibre_core::runtime::copy_offline_region_snapshot(native) }
                .map_err(map_error)?;
            offline_region_info_to_py(py, &info).map(Some)
        })()
        .map_err(operation_result_consumed)
    }

    fn offline_regions_list_take_result(
        &self,
        py: Python<'_>,
        operation_id: u64,
    ) -> PyResult<Py<PyAny>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_list>::new();
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_regions_list_take_result(
                sys::mln_operation(operation_id),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        (|| {
            let native = out
                .into_live("mln_offline_region_list")
                .map_err(map_error)?;
            let regions = unsafe { maplibre_core::runtime::copy_offline_region_list(native) }
                .map_err(map_error)?;
            offline_region_list_to_py(py, &regions)
        })()
        .map_err(operation_result_consumed)
    }

    fn offline_regions_merge_database_take_result(
        &self,
        py: Python<'_>,
        operation_id: u64,
    ) -> PyResult<Py<PyAny>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_list>::new();
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_regions_merge_database_take_result(
                sys::mln_operation(operation_id),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        (|| {
            let native = out
                .into_live("mln_offline_region_list")
                .map_err(map_error)?;
            let regions = unsafe { maplibre_core::runtime::copy_offline_region_list(native) }
                .map_err(map_error)?;
            offline_region_list_to_py(py, &regions)
        })()
        .map_err(operation_result_consumed)
    }

    fn offline_region_update_metadata_take_result(
        &self,
        py: Python<'_>,
        operation_id: u64,
    ) -> PyResult<Py<PyAny>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_snapshot>::new();
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_update_metadata_take_result(
                sys::mln_operation(operation_id),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        (|| {
            let native = out
                .into_live("mln_offline_region_snapshot")
                .map_err(map_error)?;
            let info = unsafe { maplibre_core::runtime::copy_offline_region_snapshot(native) }
                .map_err(map_error)?;
            offline_region_info_to_py(py, &info)
        })()
        .map_err(operation_result_consumed)
    }

    fn offline_region_get_status_take_result(
        &self,
        py: Python<'_>,
        operation_id: u64,
    ) -> PyResult<Py<PyAny>> {
        let mut status = empty_offline_region_status();
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_get_status_take_result(
                sys::mln_operation(operation_id),
                &mut status,
            )
        })
        .map_err(map_error)?;
        offline_region_status_to_py(py, &status).map_err(operation_result_consumed)
    }

    fn operation_finish(&self, operation_id: u64) -> PyResult<()> {
        maplibre_core::check(unsafe { sys::mln_operation_finish(sys::mln_operation(operation_id)) })
            .map_err(map_error)
    }

    fn set_resource_provider(
        &self,
        callback: Py<PyAny>,
        max_pending_callbacks: usize,
    ) -> PyResult<u64> {
        if max_pending_callbacks == 0 {
            return Err(invalid_argument_error(
                "max_pending_callbacks must be greater than zero",
            ));
        }
        let replacement = Box::new(PyResourceProviderState::new(
            callback,
            max_pending_callbacks,
        ));
        let descriptor = replacement.descriptor();
        let runtime = self
            .state_for_operation()?
            .live_handle()
            .ok_or_else(|| invalid_state_error("runtime handle is closed"))?;
        let mut command_id = 0;
        let replacement = Box::into_raw(replacement);
        let result = maplibre_core::check(unsafe {
            sys::mln_runtime_set_resource_provider(runtime, &descriptor, &mut command_id)
        })
        .map_err(map_error);
        if let Err(error) = result {
            // SAFETY: native never invokes release_user_data for a rejected registration.
            drop(unsafe { Box::from_raw(replacement) });
            return Err(error);
        }
        Ok(command_id)
    }

    fn clear_resource_provider(&self) -> PyResult<u64> {
        let runtime = self
            .state_for_operation()?
            .live_handle()
            .ok_or_else(|| invalid_state_error("runtime handle is closed"))?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_clear_resource_provider(runtime, &mut command_id)
        })
        .map_err(map_error)?;
        Ok(command_id)
    }

    fn set_resource_transform(
        &self,
        callback: Py<PyAny>,
        max_pending_callbacks: usize,
    ) -> PyResult<u64> {
        if max_pending_callbacks == 0 {
            return Err(invalid_argument_error(
                "max_pending_callbacks must be greater than zero",
            ));
        }
        let replacement = Box::new(PyResourceTransformState::new(
            callback,
            max_pending_callbacks,
        ));
        let descriptor = replacement.descriptor();
        let runtime = self
            .state_for_operation()?
            .live_handle()
            .ok_or_else(|| invalid_state_error("runtime handle is closed"))?;
        let mut command_id = 0;
        let replacement = Box::into_raw(replacement);
        let result = maplibre_core::check(unsafe {
            sys::mln_runtime_set_resource_transform(runtime, &descriptor, &mut command_id)
        })
        .map_err(map_error);
        if let Err(error) = result {
            // SAFETY: native never invokes release_user_data for a rejected registration.
            drop(unsafe { Box::from_raw(replacement) });
            return Err(error);
        }
        Ok(command_id)
    }

    fn clear_resource_transform(&self) -> PyResult<u64> {
        let runtime = self
            .state_for_operation()?
            .live_handle()
            .ok_or_else(|| invalid_state_error("runtime handle is closed"))?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_clear_resource_transform(runtime, &mut command_id)
        })
        .map_err(map_error)?;
        Ok(command_id)
    }

    fn set_http_header_transform(
        &self,
        callback: Py<PyAny>,
        max_pending_callbacks: usize,
    ) -> PyResult<u64> {
        if max_pending_callbacks == 0 {
            return Err(invalid_argument_error(
                "max_pending_callbacks must be greater than zero",
            ));
        }
        let replacement = Box::new(PyHttpHeaderTransformState::new(
            callback,
            max_pending_callbacks,
        ));
        let descriptor = replacement.descriptor();
        let runtime = self
            .state_for_operation()?
            .live_handle()
            .ok_or_else(|| invalid_state_error("runtime handle is closed"))?;
        let mut command_id = 0;
        let replacement = Box::into_raw(replacement);
        let result = maplibre_core::check(unsafe {
            sys::mln_runtime_set_http_header_transform(runtime, &descriptor, &mut command_id)
        })
        .map_err(map_error);
        if let Err(error) = result {
            // SAFETY: native never invokes release_user_data for a rejected registration.
            drop(unsafe { Box::from_raw(replacement) });
            return Err(error);
        }
        Ok(command_id)
    }

    fn clear_http_header_transform(&self) -> PyResult<u64> {
        let runtime = self
            .state_for_operation()?
            .live_handle()
            .ok_or_else(|| invalid_state_error("runtime handle is closed"))?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_clear_http_header_transform(runtime, &mut command_id)
        })
        .map_err(map_error)?;
        Ok(command_id)
    }

    fn drain_events(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state_for_operation()?;
        let mut batch = sys::mln_event_batch(0);
        maplibre_core::check(unsafe { sys::mln_runtime_drain_events(state.handle(), &mut batch) })
            .map_err(map_error)?;
        let result = (|| {
            let mut view: sys::mln_runtime_event_batch_view = unsafe { std::mem::zeroed() };
            view.size = std::mem::size_of::<sys::mln_runtime_event_batch_view>() as u32;
            maplibre_core::check(unsafe { sys::mln_event_batch_get(batch, &mut view) })
                .map_err(map_error)?;
            let mut copied = Vec::with_capacity(view.event_count);
            for event in unsafe { maplibre_core::events::drain_batch(&view) } {
                let event = event.map_err(map_error)?;
                copied.push(event);
            }
            event_batch_to_py(py, copied)
        })();
        unsafe { sys::mln_event_batch_release(batch) };
        result
    }

    fn set_event_mask(&self, mask: u64) -> PyResult<()> {
        let state = self.state_for_operation()?;
        // SAFETY: The C API validates the runtime handle and mask bits.
        maplibre_core::check(unsafe { sys::mln_runtime_set_event_mask(state.handle(), mask) })
            .map_err(map_error)
    }

    fn get_event_mask(&self) -> PyResult<u64> {
        let state = self.state_for_operation()?;
        let mut mask = 0u64;
        // SAFETY: The C API validates the runtime handle, and mask points to
        // writable storage for one u64.
        maplibre_core::check(unsafe { sys::mln_runtime_get_event_mask(state.handle(), &mut mask) })
            .map_err(map_error)?;
        Ok(mask)
    }

    #[getter]
    fn closed(&self) -> bool {
        self.operation_gate.is_closed()
    }
}

#[pymethods]
impl MapHandle {
    fn close(&self) -> PyResult<()> {
        let state = self.state();
        let Some(handle) = state.live_handle() else {
            return Ok(());
        };
        maplibre_core::check(unsafe { sys::mln_map_release(handle) }).map_err(map_error)?;
        state.mark_closed();
        Ok(())
    }

    /// This map's native handle, which names one map for the life of the
    /// process. It carries no ownership and cannot operate on the map.
    fn id(&self) -> PyResult<u64> {
        let state = self.state();
        Ok(state.handle().0)
    }

    fn request_repaint(&self) -> PyResult<u64> {
        let state = self.state();
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_request_repaint(state.handle(), &mut command_id)
        })
        .map_err(map_error)?;
        Ok(command_id)
    }

    fn request_still_image_start(&self) -> PyResult<u64> {
        let state = self.state();
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_request_still_image_start(state.handle(), &mut operation)
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn dump_debug_logs(&self) -> PyResult<u64> {
        self.run_style_command(|map, out| unsafe { sys::mln_map_dump_debug_logs(map, out) })
    }

    fn set_event_mask(&self, mask: u64) -> PyResult<u64> {
        self.run_style_command(|map, out| unsafe { sys::mln_map_set_event_mask(map, mask, out) })
    }

    fn get_event_mask(&self) -> PyResult<u64> {
        let state = self.state();
        let mut snapshot: sys::mln_map_snapshot = unsafe { std::mem::zeroed() };
        snapshot.size = std::mem::size_of::<sys::mln_map_snapshot>() as u32;
        maplibre_core::check(unsafe { sys::mln_map_snapshot_get(state.handle(), &mut snapshot) })
            .map_err(map_error)?;
        Ok(snapshot.event_mask)
    }

    fn set_debug_options(&self, options: u32) -> PyResult<u64> {
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_debug_options(map, options, out)
        })
    }

    fn set_rendering_stats_view_enabled(&self, enabled: bool) -> PyResult<u64> {
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_rendering_stats_view_enabled(map, enabled, out)
        })
    }

    fn get_size(&self) -> PyResult<(u32, u32, f64)> {
        let state = self.state();
        let mut snapshot: sys::mln_map_snapshot = unsafe { std::mem::zeroed() };
        snapshot.size = std::mem::size_of::<sys::mln_map_snapshot>() as u32;
        maplibre_core::check(unsafe { sys::mln_map_snapshot_get(state.handle(), &mut snapshot) })
            .map_err(map_error)?;
        Ok((
            snapshot.logical_extent.width,
            snapshot.logical_extent.height,
            snapshot.logical_extent.scale_factor,
        ))
    }

    fn resize(&self, width: u32, height: u32, scale_factor: f64) -> PyResult<u64> {
        let state = self.state();
        let extent = sys::mln_logical_extent {
            width,
            height,
            scale_factor,
        };
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_resize(state.handle(), extent, &mut command_id)
        })
        .map_err(map_error)?;
        Ok(command_id)
    }
    fn snapshot(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let mut snapshot: sys::mln_map_snapshot = unsafe { std::mem::zeroed() };
        snapshot.size = std::mem::size_of::<sys::mln_map_snapshot>() as u32;
        maplibre_core::check(unsafe { sys::mln_map_snapshot_get(state.handle(), &mut snapshot) })
            .map_err(map_error)?;
        let result = PyDict::new(py);
        result.set_item("generation", snapshot.generation)?;
        result.set_item("camera", camera_options_to_py(py, &snapshot.camera)?)?;
        result.set_item("width", snapshot.logical_extent.width)?;
        result.set_item("height", snapshot.logical_extent.height)?;
        result.set_item("scale_factor", snapshot.logical_extent.scale_factor)?;
        result.set_item(
            "projection_mode",
            projection_mode_to_py(py, &snapshot.projection_mode)?,
        )?;
        result.set_item("viewport", viewport_options_to_py(py, &snapshot.viewport)?)?;
        result.set_item("debug_options", snapshot.debug_options)?;
        result.set_item("fully_loaded", snapshot.fully_loaded)?;
        result.set_item(
            "rendering_stats_view_enabled",
            snapshot.rendering_stats_view_enabled,
        )?;
        result.set_item("repaint_demand", snapshot.repaint_demand)?;
        result.set_item("event_mask", snapshot.event_mask)?;
        result.set_item(
            "latest_render_update_generation",
            snapshot.latest_render_update_generation,
        )?;
        result.set_item("tile", tile_options_to_py(py, &snapshot.tile)?)?;
        result.set_item("bounds", bound_options_to_py(py, &snapshot.bounds)?)?;
        result.set_item(
            "free_camera",
            free_camera_options_to_py(py, &snapshot.free_camera)?,
        )?;
        Ok(result.into_any().unbind())
    }

    fn set_viewport_options(
        &self,
        north_orientation: Option<u32>,
        constrain_mode: Option<u32>,
        viewport_mode: Option<u32>,
        frustum_offset: Option<(f64, f64, f64, f64)>,
    ) -> PyResult<u64> {
        let options = viewport_options_from_parts(
            north_orientation,
            constrain_mode,
            viewport_mode,
            frustum_offset,
        );
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_viewport_options(map, &options, out)
        })
    }

    fn set_tile_options(
        &self,
        prefetch_zoom_delta: Option<u32>,
        lod_min_radius: Option<f64>,
        lod_scale: Option<f64>,
        lod_pitch_threshold: Option<f64>,
        lod_zoom_shift: Option<f64>,
        lod_mode: Option<u32>,
    ) -> PyResult<u64> {
        let options = tile_options_from_parts(
            prefetch_zoom_delta,
            lod_min_radius,
            lod_scale,
            lod_pitch_threshold,
            lod_zoom_shift,
            lod_mode,
        );
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_tile_options(map, &options, out)
        })
    }

    fn create_projection(&self, py: Python<'_>) -> PyResult<MapProjectionHandle> {
        let state = self.state();
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_projection_create_start(state.handle(), &mut operation)
        })
        .map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_map_projection>::new();
        maplibre_core::check(unsafe {
            sys::mln_map_projection_create_take_result(operation.0, out.as_mut_ptr())
        })
        .map_err(map_error)?;
        let native = out.into_live("mln_map_projection").map_err(map_error)?;
        let handle = unsafe {
            maplibre_core::handle::NativeHandleState::from_handle(native, "mln_map_projection")
        }
        .map_err(map_error)?;
        Ok(MapProjectionHandle {
            state: Mutex::new(handle),
        })
    }

    fn set_style_url(&self, url: String) -> PyResult<u64> {
        let url = maplibre_core::string::c_string(&url).map_err(map_error)?;
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_style_url(map, url.as_ptr(), out)
        })
    }

    fn set_style_json(&self, json: &Bound<'_, PyBytes>) -> PyResult<u64> {
        let json = maplibre_core::string::buffer_view(json.as_bytes());
        self.run_style_command(|map, out| unsafe { sys::mln_map_set_style_json(map, json, out) })
    }

    fn copy_loaded_style_json(&self, py: Python<'_>) -> PyResult<Py<PyBytes>> {
        self.run_style_operation(
            py,
            |map, out| unsafe { sys::mln_map_loaded_style_json_start(map, out) },
            |operation| {
                let mut buffer = sys::mln_buffer(0);
                maplibre_core::check(unsafe {
                    sys::mln_map_loaded_style_json_take_result(operation, &mut buffer)
                })
                .map_err(map_error)?;
                owned_buffer_to_py(py, buffer)
            },
        )
    }

    fn copy_style_url(&self, py: Python<'_>) -> PyResult<String> {
        self.run_style_operation(
            py,
            |map, out| unsafe { sys::mln_map_style_url_start(map, out) },
            |operation| {
                let mut buffer = sys::mln_buffer(0);
                maplibre_core::check(unsafe {
                    sys::mln_map_style_url_take_result(operation, &mut buffer)
                })
                .map_err(map_error)?;
                owned_buffer_to_string(buffer)
            },
        )
    }

    fn get_camera(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let mut camera = unsafe { sys::mln_camera_options_default() };
        let mut generation = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_camera_snapshot_get(state.handle(), &mut camera, &mut generation)
        })
        .map_err(map_error)?;
        camera_options_to_py(py, &camera)
    }

    fn get_camera_ordered(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_camera_query_start(state.handle(), &mut operation)
        })
        .map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        let mut result: sys::mln_camera_query_result = unsafe { std::mem::zeroed() };
        result.size = std::mem::size_of::<sys::mln_camera_query_result>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_map_camera_query_take_result(operation.0, &mut result)
        })
        .map_err(map_error)?;
        let raw = PyDict::new(py);
        raw.set_item("generation", result.generation)?;
        raw.set_item("camera", camera_options_to_py(py, &result.camera)?)?;
        Ok(raw.into_any().unbind())
    }

    #[allow(clippy::too_many_arguments)]
    fn jump_to(
        &self,
        center: Option<(f64, f64)>,
        zoom: Option<f64>,
        bearing: Option<f64>,
        pitch: Option<f64>,
        center_altitude: Option<f64>,
        padding: Option<(f64, f64, f64, f64)>,
        anchor: Option<(f64, f64)>,
        roll: Option<f64>,
        field_of_view: Option<f64>,
    ) -> PyResult<u64> {
        let state = self.state();
        let mut update = unsafe { sys::mln_camera_update_default() };
        update.mode = sys::MLN_CAMERA_UPDATE_MODE_JUMP;
        update.camera = camera_options_from_parts(
            center,
            zoom,
            bearing,
            pitch,
            center_altitude,
            padding,
            anchor,
            roll,
            field_of_view,
        );
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_update_camera(state.handle(), &update, &mut command_id)
        })
        .map_err(map_error)?;
        Ok(command_id)
    }

    #[allow(clippy::too_many_arguments)]
    fn ease_to(
        &self,
        center: Option<(f64, f64)>,
        zoom: Option<f64>,
        bearing: Option<f64>,
        pitch: Option<f64>,
        center_altitude: Option<f64>,
        padding: Option<(f64, f64, f64, f64)>,
        anchor: Option<(f64, f64)>,
        roll: Option<f64>,
        field_of_view: Option<f64>,
        animation: Option<AnimationParts>,
    ) -> PyResult<u64> {
        let state = self.state();
        let mut update = unsafe { sys::mln_camera_update_default() };
        update.mode = sys::MLN_CAMERA_UPDATE_MODE_EASE;
        update.camera = camera_options_from_parts(
            center,
            zoom,
            bearing,
            pitch,
            center_altitude,
            padding,
            anchor,
            roll,
            field_of_view,
        );
        if let Some(animation) = animation {
            update.animation = animation_options_from_parts(animation);
        }
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_update_camera(state.handle(), &update, &mut command_id)
        })
        .map_err(map_error)?;
        Ok(command_id)
    }

    #[allow(clippy::too_many_arguments)]
    fn fly_to(
        &self,
        center: Option<(f64, f64)>,
        zoom: Option<f64>,
        bearing: Option<f64>,
        pitch: Option<f64>,
        center_altitude: Option<f64>,
        padding: Option<(f64, f64, f64, f64)>,
        anchor: Option<(f64, f64)>,
        roll: Option<f64>,
        field_of_view: Option<f64>,
        animation: Option<AnimationParts>,
    ) -> PyResult<u64> {
        let state = self.state();
        let mut update = unsafe { sys::mln_camera_update_default() };
        update.mode = sys::MLN_CAMERA_UPDATE_MODE_FLY;
        update.camera = camera_options_from_parts(
            center,
            zoom,
            bearing,
            pitch,
            center_altitude,
            padding,
            anchor,
            roll,
            field_of_view,
        );
        if let Some(animation) = animation {
            update.animation = animation_options_from_parts(animation);
        }
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_update_camera(state.handle(), &update, &mut command_id)
        })
        .map_err(map_error)?;
        Ok(command_id)
    }

    fn apply_camera_delta(
        &self,
        kind: u32,
        offset: (f64, f64),
        amount: f64,
        anchor: Option<(f64, f64)>,
        animation: Option<AnimationParts>,
    ) -> PyResult<u64> {
        let state = self.state();
        let mut delta = unsafe { sys::mln_camera_delta_default() };
        delta.kind = kind;
        delta.offset = screen_point_from_tuple(offset);
        delta.amount = amount;
        if let Some(anchor) = anchor {
            delta.has_anchor = true;
            delta.anchor = screen_point_from_tuple(anchor);
        }
        if let Some(animation) = animation {
            delta.animation = animation_options_from_parts(animation);
        }
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_apply_camera_delta(state.handle(), &delta, &mut command_id)
        })
        .map_err(map_error)?;
        Ok(command_id)
    }

    fn camera_for_lat_lng_bounds(
        &self,
        py: Python<'_>,
        southwest: (f64, f64),
        northeast: (f64, f64),
        fit_padding: Option<(f64, f64, f64, f64)>,
        fit_bearing: Option<f64>,
        fit_pitch: Option<f64>,
    ) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let bounds = lat_lng_bounds_from_tuple((southwest, northeast));
        let fit = camera_fit_options_from_parts(fit_padding, fit_bearing, fit_pitch);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_lat_lng_bounds_start(
                state.handle(),
                bounds,
                &fit,
                &mut operation,
            )
        })
        .map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        let mut camera = unsafe { sys::mln_camera_options_default() };
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_lat_lng_bounds_take_result(operation.0, &mut camera)
        })
        .map_err(map_error)?;
        camera_options_to_py(py, &camera)
    }

    fn camera_for_lat_lngs(
        &self,
        py: Python<'_>,
        coordinates: Vec<(f64, f64)>,
        fit_padding: Option<(f64, f64, f64, f64)>,
        fit_bearing: Option<f64>,
        fit_pitch: Option<f64>,
    ) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let coordinates = lat_lngs_from_tuples(coordinates);
        let fit = camera_fit_options_from_parts(fit_padding, fit_bearing, fit_pitch);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_lat_lngs_start(
                state.handle(),
                coordinates.as_ptr(),
                coordinates.len(),
                &fit,
                &mut operation,
            )
        })
        .map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        let mut camera = unsafe { sys::mln_camera_options_default() };
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_lat_lngs_take_result(operation.0, &mut camera)
        })
        .map_err(map_error)?;
        camera_options_to_py(py, &camera)
    }

    fn camera_for_geometry(
        &self,
        py: Python<'_>,
        geometry: &Bound<'_, PyBytes>,
        fit_padding: Option<(f64, f64, f64, f64)>,
        fit_bearing: Option<f64>,
        fit_pitch: Option<f64>,
    ) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let geometry = maplibre_core::string::buffer_view(geometry.as_bytes());
        let fit = camera_fit_options_from_parts(fit_padding, fit_bearing, fit_pitch);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_geometry_start(state.handle(), geometry, &fit, &mut operation)
        })
        .map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        let mut camera = unsafe { sys::mln_camera_options_default() };
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_geometry_take_result(operation.0, &mut camera)
        })
        .map_err(map_error)?;
        camera_options_to_py(py, &camera)
    }

    #[allow(clippy::too_many_arguments)]
    fn lat_lng_bounds_for_camera(
        &self,
        py: Python<'_>,
        center: Option<(f64, f64)>,
        zoom: Option<f64>,
        bearing: Option<f64>,
        pitch: Option<f64>,
        center_altitude: Option<f64>,
        padding: Option<(f64, f64, f64, f64)>,
        anchor: Option<(f64, f64)>,
        roll: Option<f64>,
        field_of_view: Option<f64>,
        unwrapped: bool,
    ) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let camera = camera_options_from_parts(
            center,
            zoom,
            bearing,
            pitch,
            center_altitude,
            padding,
            anchor,
            roll,
            field_of_view,
        );
        let mut operation = sys::mln_operation(0);
        let status = if unwrapped {
            unsafe {
                sys::mln_map_lat_lng_bounds_for_camera_unwrapped_start(
                    state.handle(),
                    &camera,
                    &mut operation,
                )
            }
        } else {
            unsafe {
                sys::mln_map_lat_lng_bounds_for_camera_start(
                    state.handle(),
                    &camera,
                    &mut operation,
                )
            }
        };
        maplibre_core::check(status).map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        let mut bounds = empty_lat_lng_bounds();
        let status = if unwrapped {
            unsafe {
                sys::mln_map_lat_lng_bounds_for_camera_unwrapped_take_result(
                    operation.0,
                    &mut bounds,
                )
            }
        } else {
            unsafe { sys::mln_map_lat_lng_bounds_for_camera_take_result(operation.0, &mut bounds) }
        };
        maplibre_core::check(status).map_err(map_error)?;
        lat_lng_bounds_to_py(py, bounds)
    }

    fn set_bounds(
        &self,
        bounds: Option<((f64, f64), (f64, f64))>,
        unbounded: bool,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        min_pitch: Option<f64>,
        max_pitch: Option<f64>,
    ) -> PyResult<u64> {
        let bounds =
            bound_options_from_parts(bounds, unbounded, min_zoom, max_zoom, min_pitch, max_pitch);
        self.run_style_command(|map, out| unsafe { sys::mln_map_set_bounds(map, &bounds, out) })
    }

    fn set_free_camera_options(
        &self,
        position: Option<(f64, f64, f64)>,
        orientation: Option<(f64, f64, f64, f64)>,
    ) -> PyResult<u64> {
        let options = free_camera_options_from_parts(position, orientation);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_free_camera_options(map, &options, out)
        })
    }

    fn set_projection_mode(
        &self,
        axonometric: Option<bool>,
        x_skew: Option<f64>,
        y_skew: Option<f64>,
    ) -> PyResult<u64> {
        let mode = projection_mode_from_parts(axonometric, x_skew, y_skew);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_projection_mode(map, &mode, out)
        })
    }

    fn pixel_for_lat_lng(
        &self,
        py: Python<'_>,
        latitude: f64,
        longitude: f64,
    ) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_pixel_for_lat_lng_start(
                state.handle(),
                sys::mln_lat_lng {
                    latitude,
                    longitude,
                },
                &mut operation,
            )
        })
        .map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        let mut point = sys::mln_screen_point { x: 0.0, y: 0.0 };
        maplibre_core::check(unsafe {
            sys::mln_map_pixel_for_lat_lng_take_result(operation.0, &mut point)
        })
        .map_err(map_error)?;
        screen_point_to_py(py, point)
    }

    fn lat_lng_for_pixel(&self, py: Python<'_>, x: f64, y: f64) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lng_for_pixel_start(
                state.handle(),
                sys::mln_screen_point { x, y },
                &mut operation,
            )
        })
        .map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        let mut coordinate = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lng_for_pixel_take_result(operation.0, &mut coordinate)
        })
        .map_err(map_error)?;
        lat_lng_to_py(py, coordinate)
    }

    fn pixels_for_lat_lngs(
        &self,
        py: Python<'_>,
        coordinates: Vec<(f64, f64)>,
    ) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let coordinates = lat_lngs_from_tuples(coordinates);
        let result_count = coordinates.len();
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_pixels_for_lat_lngs_start(
                state.handle(),
                coordinates.as_ptr(),
                coordinates.len(),
                &mut operation,
            )
        })
        .map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        let mut points = vec![sys::mln_screen_point { x: 0.0, y: 0.0 }; result_count];
        let mut point_count = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_pixels_for_lat_lngs_take_result(
                operation.0,
                points.as_mut_ptr(),
                points.len(),
                &mut point_count,
            )
        })
        .map_err(map_error)?;
        points.truncate(point_count);
        screen_point_list_to_py(py, &points)
    }

    fn lat_lngs_for_pixels(&self, py: Python<'_>, points: Vec<(f64, f64)>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let points: Vec<_> = points.into_iter().map(screen_point_from_tuple).collect();
        let result_count = points.len();
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lngs_for_pixels_start(
                state.handle(),
                points.as_ptr(),
                points.len(),
                &mut operation,
            )
        })
        .map_err(map_error)?;
        drop(state);
        let operation = OwnedOperation(operation);
        wait_operation(py, operation.0)?;
        let mut coordinates = vec![
            sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0
            };
            result_count
        ];
        let mut coordinate_count = 0;
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lngs_for_pixels_take_result(
                operation.0,
                coordinates.as_mut_ptr(),
                coordinates.len(),
                &mut coordinate_count,
            )
        })
        .map_err(map_error)?;
        coordinates.truncate(coordinate_count);
        lat_lng_list_to_py(py, &coordinates)
    }

    fn add_style_source_json(
        &self,
        source_id: String,
        source_json: &Bound<'_, PyBytes>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let source_json = maplibre_core::string::buffer_view(source_json.as_bytes());
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_add_style_source_json(map, source_id.raw(), source_json, out)
        })
    }

    #[allow(clippy::too_many_arguments)]
    fn add_geojson_source_url(
        &self,
        source_id: String,
        url: String,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        tolerance: Option<f64>,
        cluster_max_zoom: Option<f64>,
        cluster_properties: Option<Bound<'_, PyBytes>>,
        tile_size: Option<u32>,
        buffer: Option<u32>,
        cluster_radius: Option<u32>,
        cluster_min_points: Option<u32>,
        line_metrics: Option<bool>,
        cluster: Option<bool>,
        synchronous_tiling: Option<bool>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let url = maplibre_core::string::string_view(&url);
        let options = geojson_source_options_from_parts(
            min_zoom,
            max_zoom,
            tolerance,
            cluster_max_zoom,
            cluster_properties,
            tile_size,
            buffer,
            cluster_radius,
            cluster_min_points,
            line_metrics,
            cluster,
            synchronous_tiling,
        )?;
        let options =
            maplibre_core::style::geojson_source_options_to_native(&options).map_err(map_error)?;
        // SAFETY: The C API validates the map pointer, borrowed string views, and options.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_add_geojson_source_url(
                map,
                source_id.raw(),
                url.raw(),
                options.as_ptr(),
                out,
            )
        })
    }

    fn add_geojson_source_data(
        &self,
        source_id: String,
        data: &GeoJsonSourceDataHandle,
    ) -> PyResult<u64> {
        let data_state = data.state();
        let Some(data_handle) = data_state.live_handle() else {
            return Err(invalid_state_error("GeoJSON source data handle is closed"));
        };
        let source_id = maplibre_core::string::string_view(&source_id);
        // SAFETY: The C API validates the map pointer, source ID, and data
        // handle. The call borrows the data handle, which the held state mutex
        // keeps live for the duration of the call.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_add_geojson_source_data(map, source_id.raw(), data_handle, out)
        })
    }

    fn set_geojson_source_url(&self, source_id: String, url: String) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let url = maplibre_core::string::string_view(&url);
        // SAFETY: The C API validates the map pointer and borrowed string views.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_geojson_source_url(map, source_id.raw(), url.raw(), out)
        })
    }

    fn set_geojson_source_data(
        &self,
        source_id: String,
        data: &GeoJsonSourceDataHandle,
    ) -> PyResult<u64> {
        let data_state = data.state();
        let Some(data_handle) = data_state.live_handle() else {
            return Err(invalid_state_error("GeoJSON source data handle is closed"));
        };
        let source_id = maplibre_core::string::string_view(&source_id);
        // SAFETY: The C API validates the map pointer, source ID, and data
        // handle. The call borrows the data handle, which the held state mutex
        // keeps live for the duration of the call.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_geojson_source_data(map, source_id.raw(), data_handle, out)
        })
    }

    fn set_geojson_source_synchronous_tiling(
        &self,
        source_id: String,
        enabled: bool,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        // SAFETY: The C API validates the map pointer and source ID.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_geojson_source_synchronous_tiling(map, source_id.raw(), enabled, out)
        })
    }

    #[allow(clippy::too_many_arguments)]
    fn add_vector_source_url(
        &self,
        source_id: String,
        url: String,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        attribution: Option<String>,
        scheme: Option<u32>,
        bounds: Option<((f64, f64), (f64, f64))>,
        tile_size: Option<u32>,
        vector_encoding: Option<u32>,
        raster_dem_encoding: Option<u32>,
    ) -> PyResult<u64> {
        self.add_tile_source_url_with(
            source_id,
            url,
            min_zoom,
            max_zoom,
            attribution,
            scheme,
            bounds,
            tile_size,
            vector_encoding,
            raster_dem_encoding,
            sys::mln_map_add_vector_source_url,
        )
    }

    #[allow(clippy::too_many_arguments)]
    fn add_raster_source_url(
        &self,
        source_id: String,
        url: String,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        attribution: Option<String>,
        scheme: Option<u32>,
        bounds: Option<((f64, f64), (f64, f64))>,
        tile_size: Option<u32>,
        vector_encoding: Option<u32>,
        raster_dem_encoding: Option<u32>,
    ) -> PyResult<u64> {
        self.add_tile_source_url_with(
            source_id,
            url,
            min_zoom,
            max_zoom,
            attribution,
            scheme,
            bounds,
            tile_size,
            vector_encoding,
            raster_dem_encoding,
            sys::mln_map_add_raster_source_url,
        )
    }

    #[allow(clippy::too_many_arguments)]
    fn add_raster_dem_source_url(
        &self,
        source_id: String,
        url: String,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        attribution: Option<String>,
        scheme: Option<u32>,
        bounds: Option<((f64, f64), (f64, f64))>,
        tile_size: Option<u32>,
        vector_encoding: Option<u32>,
        raster_dem_encoding: Option<u32>,
    ) -> PyResult<u64> {
        self.add_tile_source_url_with(
            source_id,
            url,
            min_zoom,
            max_zoom,
            attribution,
            scheme,
            bounds,
            tile_size,
            vector_encoding,
            raster_dem_encoding,
            sys::mln_map_add_raster_dem_source_url,
        )
    }

    #[allow(clippy::too_many_arguments)]
    fn add_vector_source_tiles(
        &self,
        source_id: String,
        tiles: Vec<String>,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        attribution: Option<String>,
        scheme: Option<u32>,
        bounds: Option<((f64, f64), (f64, f64))>,
        tile_size: Option<u32>,
        vector_encoding: Option<u32>,
        raster_dem_encoding: Option<u32>,
    ) -> PyResult<u64> {
        self.add_tile_source_tiles_with(
            source_id,
            tiles,
            min_zoom,
            max_zoom,
            attribution,
            scheme,
            bounds,
            tile_size,
            vector_encoding,
            raster_dem_encoding,
            sys::mln_map_add_vector_source_tiles,
        )
    }

    #[allow(clippy::too_many_arguments)]
    fn add_raster_source_tiles(
        &self,
        source_id: String,
        tiles: Vec<String>,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        attribution: Option<String>,
        scheme: Option<u32>,
        bounds: Option<((f64, f64), (f64, f64))>,
        tile_size: Option<u32>,
        vector_encoding: Option<u32>,
        raster_dem_encoding: Option<u32>,
    ) -> PyResult<u64> {
        self.add_tile_source_tiles_with(
            source_id,
            tiles,
            min_zoom,
            max_zoom,
            attribution,
            scheme,
            bounds,
            tile_size,
            vector_encoding,
            raster_dem_encoding,
            sys::mln_map_add_raster_source_tiles,
        )
    }

    #[allow(clippy::too_many_arguments)]
    fn add_raster_dem_source_tiles(
        &self,
        source_id: String,
        tiles: Vec<String>,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        attribution: Option<String>,
        scheme: Option<u32>,
        bounds: Option<((f64, f64), (f64, f64))>,
        tile_size: Option<u32>,
        vector_encoding: Option<u32>,
        raster_dem_encoding: Option<u32>,
    ) -> PyResult<u64> {
        self.add_tile_source_tiles_with(
            source_id,
            tiles,
            min_zoom,
            max_zoom,
            attribution,
            scheme,
            bounds,
            tile_size,
            vector_encoding,
            raster_dem_encoding,
            sys::mln_map_add_raster_dem_source_tiles,
        )
    }

    fn remove_style_source(&self, source_id: String) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_remove_style_source(map, source_id.raw(), out)
        })
    }

    fn get_style_source_info(
        &self,
        py: Python<'_>,
        source_id: String,
    ) -> PyResult<Option<Py<PyAny>>> {
        let source_id_view = maplibre_core::string::string_view(&source_id);
        let (info, found) = self.run_style_operation(
            py,
            |map, out| unsafe {
                sys::mln_map_get_style_source_info_start(map, source_id_view.raw(), out)
            },
            |operation| {
                let mut info = maplibre_core::style::empty_style_source_info();
                let mut found = false;
                maplibre_core::check(unsafe {
                    sys::mln_map_get_style_source_info_take_result(operation, &mut info, &mut found)
                })
                .map_err(map_error)?;
                Ok((info, found))
            },
        )?;
        if !found {
            return Ok(None);
        }
        let attribution = if info.has_attribution {
            self.run_style_operation(
                py,
                |map, out| unsafe {
                    sys::mln_map_copy_style_source_attribution_start(map, source_id_view.raw(), out)
                },
                |operation| {
                    let mut buffer = sys::mln_buffer(0);
                    let mut found = false;
                    maplibre_core::check(unsafe {
                        sys::mln_map_copy_style_source_attribution_take_result(
                            operation,
                            &mut buffer,
                            &mut found,
                        )
                    })
                    .map_err(map_error)?;
                    found.then(|| owned_buffer_to_string(buffer)).transpose()
                },
            )?
        } else {
            None
        };
        let url = if info.fields & sys::MLN_STYLE_SOURCE_INFO_URL != 0 {
            self.run_style_operation(
                py,
                |map, out| unsafe {
                    sys::mln_map_copy_style_source_url_start(map, source_id_view.raw(), out)
                },
                |operation| {
                    let mut buffer = sys::mln_buffer(0);
                    let mut found = false;
                    maplibre_core::check(unsafe {
                        sys::mln_map_copy_style_source_url_take_result(
                            operation,
                            &mut buffer,
                            &mut found,
                        )
                    })
                    .map_err(map_error)?;
                    found.then(|| owned_buffer_to_string(buffer)).transpose()
                },
            )?
        } else {
            None
        };
        let tiles = if info.fields & sys::MLN_STYLE_SOURCE_INFO_TILEJSON != 0 {
            self.run_style_operation(
                py,
                |map, out| unsafe {
                    sys::mln_map_get_style_source_tile_urls_start(map, source_id_view.raw(), out)
                },
                |operation| {
                    let mut out =
                        maplibre_core::ptr::OutHandle::<sys::mln_style_string_list>::new();
                    let mut found = false;
                    maplibre_core::check(unsafe {
                        sys::mln_map_get_style_source_tile_urls_take_result(
                            operation,
                            out.as_mut_ptr(),
                            &mut found,
                        )
                    })
                    .map_err(map_error)?;
                    if !found {
                        return Ok(Vec::new());
                    }
                    let native = out.into_live("mln_style_string_list").map_err(map_error)?;
                    unsafe { maplibre_core::style::copy_style_string_list(native) }
                        .map_err(map_error)
                },
            )?
        } else {
            Vec::new()
        };
        let copied =
            maplibre_core::style::style_source_info_from_native(&info, attribution, url, tiles);
        source_info_to_py(py, copied).map(Some)
    }

    fn list_style_source_ids(&self, py: Python<'_>) -> PyResult<Vec<String>> {
        self.run_style_operation(
            py,
            |map, out| unsafe { sys::mln_map_list_style_source_ids_start(map, out) },
            |operation| {
                let mut out = maplibre_core::ptr::OutHandle::<sys::mln_style_id_list>::new();
                maplibre_core::check(unsafe {
                    sys::mln_map_list_style_source_ids_take_result(operation, out.as_mut_ptr())
                })
                .map_err(map_error)?;
                let native = out.into_live("mln_style_id_list").map_err(map_error)?;
                unsafe { maplibre_core::style::copy_style_id_list(native) }.map_err(map_error)
            },
        )
    }

    fn add_hillshade_layer(
        &self,
        layer_id: String,
        source_id: String,
        before_layer_id: Option<String>,
    ) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let source_id = maplibre_core::string::string_view(&source_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = maplibre_core::string::string_view(&before_layer_id);
        // SAFETY: The C API validates the map pointer and borrowed layer/source ID views.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_add_hillshade_layer(
                map,
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
                out,
            )
        })
    }

    fn add_color_relief_layer(
        &self,
        layer_id: String,
        source_id: String,
        before_layer_id: Option<String>,
    ) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let source_id = maplibre_core::string::string_view(&source_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = maplibre_core::string::string_view(&before_layer_id);
        // SAFETY: The C API validates the map pointer and borrowed layer/source ID views.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_add_color_relief_layer(
                map,
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
                out,
            )
        })
    }

    fn add_location_indicator_layer(
        &self,
        layer_id: String,
        before_layer_id: Option<String>,
    ) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = maplibre_core::string::string_view(&before_layer_id);
        // SAFETY: The C API validates the map pointer and borrowed layer ID views.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_add_location_indicator_layer(
                map,
                layer_id.raw(),
                before_layer_id.raw(),
                out,
            )
        })
    }

    fn set_location_indicator_location(
        &self,
        layer_id: String,
        latitude: f64,
        longitude: f64,
        altitude: f64,
    ) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer, layer ID, coordinate, and altitude.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_location_indicator_location(
                map,
                layer_id.raw(),
                sys::mln_lat_lng {
                    latitude,
                    longitude,
                },
                altitude,
                out,
            )
        })
    }

    fn set_location_indicator_bearing(&self, layer_id: String, bearing: f64) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer, layer ID, and bearing.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_location_indicator_bearing(map, layer_id.raw(), bearing, out)
        })
    }

    fn set_location_indicator_accuracy_radius(
        &self,
        layer_id: String,
        radius: f64,
    ) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer, layer ID, and radius.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_location_indicator_accuracy_radius(map, layer_id.raw(), radius, out)
        })
    }

    fn set_location_indicator_image_name(
        &self,
        layer_id: String,
        image_kind: u32,
        image_id: String,
    ) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let image_id = maplibre_core::string::string_view(&image_id);
        // SAFETY: The C API validates the map pointer, layer ID, image kind, and image ID.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_location_indicator_image_name(
                map,
                layer_id.raw(),
                image_kind,
                image_id.raw(),
                out,
            )
        })
    }

    fn add_style_layer_json(
        &self,
        layer_json: &Bound<'_, PyBytes>,
        before_layer_id: Option<String>,
    ) -> PyResult<u64> {
        let layer_json = maplibre_core::string::buffer_view(layer_json.as_bytes());
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = maplibre_core::string::string_view(&before_layer_id);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_add_style_layer_json(map, layer_json, before_layer_id.raw(), out)
        })
    }

    fn get_style_layer_json(
        &self,
        py: Python<'_>,
        layer_id: String,
    ) -> PyResult<Option<Py<PyBytes>>> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        self.run_style_operation(
            py,
            |map, out| unsafe { sys::mln_map_get_style_layer_json_start(map, layer_id.raw(), out) },
            |operation| {
                let mut buffer = sys::mln_buffer(0);
                let mut found = false;
                maplibre_core::check(unsafe {
                    sys::mln_map_get_style_layer_json_take_result(
                        operation,
                        &mut buffer,
                        &mut found,
                    )
                })
                .map_err(map_error)?;
                found.then(|| owned_buffer_to_py(py, buffer)).transpose()
            },
        )
    }

    fn set_style_light_json(&self, light_json: &Bound<'_, PyBytes>) -> PyResult<u64> {
        let light_json = maplibre_core::string::buffer_view(light_json.as_bytes());
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_style_light_json(map, light_json, out)
        })
    }

    fn set_style_light_property(
        &self,
        property_name: String,
        value: &Bound<'_, PyBytes>,
    ) -> PyResult<u64> {
        let property_name = maplibre_core::string::string_view(&property_name);
        let value = maplibre_core::string::buffer_view(value.as_bytes());
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_style_light_property(map, property_name.raw(), value, out)
        })
    }

    fn get_style_light_property(
        &self,
        py: Python<'_>,
        property_name: String,
    ) -> PyResult<Option<Py<PyBytes>>> {
        let property_name = maplibre_core::string::string_view(&property_name);
        self.run_style_operation(
            py,
            |map, out| unsafe {
                sys::mln_map_get_style_light_property_start(map, property_name.raw(), out)
            },
            |operation| {
                let mut buffer = sys::mln_buffer(0);
                maplibre_core::check(unsafe {
                    sys::mln_map_get_style_light_property_take_result(operation, &mut buffer)
                })
                .map_err(map_error)?;
                if buffer.0 == 0 {
                    Ok(None)
                } else {
                    owned_buffer_to_py(py, buffer).map(Some)
                }
            },
        )
    }

    fn set_style_transition_options(
        &self,
        duration_ms: Option<f64>,
        delay_ms: Option<f64>,
        enable_placement_transitions: Option<bool>,
    ) -> PyResult<u64> {
        let mut options = maplibre_core::StyleTransitionOptions::default();
        options.duration_ms = duration_ms;
        options.delay_ms = delay_ms;
        options.enable_placement_transitions = enable_placement_transitions;
        let options = maplibre_core::style::style_transition_options_to_native(&options);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_style_transition_options(map, &options, out)
        })
    }

    fn get_style_transition_options(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let options = self.run_style_operation(
            py,
            |map, out| unsafe { sys::mln_map_get_style_transition_options_start(map, out) },
            |operation| {
                let mut options = maplibre_core::style::empty_style_transition_options();
                maplibre_core::check(unsafe {
                    sys::mln_map_get_style_transition_options_take_result(operation, &mut options)
                })
                .map_err(map_error)?;
                Ok(options)
            },
        )?;
        let options = maplibre_core::style::style_transition_options_from_native(&options);
        let dict = PyDict::new(py);
        dict.set_item("duration_ms", options.duration_ms)?;
        dict.set_item("delay_ms", options.delay_ms)?;
        dict.set_item(
            "enable_placement_transitions",
            options.enable_placement_transitions,
        )?;
        Ok(dict.into_any().unbind())
    }

    fn set_layer_property(
        &self,
        layer_id: String,
        property_name: String,
        value: &Bound<'_, PyBytes>,
    ) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let property_name = maplibre_core::string::string_view(&property_name);
        let value = maplibre_core::string::buffer_view(value.as_bytes());
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_layer_property(map, layer_id.raw(), property_name.raw(), value, out)
        })
    }

    fn get_layer_property(
        &self,
        py: Python<'_>,
        layer_id: String,
        property_name: String,
    ) -> PyResult<Option<Py<PyBytes>>> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let property_name = maplibre_core::string::string_view(&property_name);
        self.run_style_operation(
            py,
            |map, out| unsafe {
                sys::mln_map_get_layer_property_start(map, layer_id.raw(), property_name.raw(), out)
            },
            |operation| {
                let mut buffer = sys::mln_buffer(0);
                maplibre_core::check(unsafe {
                    sys::mln_map_get_layer_property_take_result(operation, &mut buffer)
                })
                .map_err(map_error)?;
                if buffer.0 == 0 {
                    Ok(None)
                } else {
                    owned_buffer_to_py(py, buffer).map(Some)
                }
            },
        )
    }

    fn set_layer_source_layer(&self, layer_id: String, source_layer: String) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let source_layer = maplibre_core::string::string_view(&source_layer);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_layer_source_layer(map, layer_id.raw(), source_layer.raw(), out)
        })
    }

    fn copy_layer_source_layer(&self, py: Python<'_>, layer_id: String) -> PyResult<String> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        self.run_style_operation(
            py,
            |map, out| unsafe {
                sys::mln_map_copy_layer_source_layer_start(map, layer_id.raw(), out)
            },
            |operation| {
                let mut buffer = sys::mln_buffer(0);
                maplibre_core::check(unsafe {
                    sys::mln_map_copy_layer_source_layer_take_result(operation, &mut buffer)
                })
                .map_err(map_error)?;
                owned_buffer_to_string(buffer)
            },
        )
    }

    fn set_layer_source_id(&self, layer_id: String, source_id: String) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let source_id = maplibre_core::string::string_view(&source_id);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_layer_source_id(map, layer_id.raw(), source_id.raw(), out)
        })
    }

    fn copy_layer_source_id(&self, py: Python<'_>, layer_id: String) -> PyResult<String> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        self.run_style_operation(
            py,
            |map, out| unsafe { sys::mln_map_copy_layer_source_id_start(map, layer_id.raw(), out) },
            |operation| {
                let mut buffer = sys::mln_buffer(0);
                maplibre_core::check(unsafe {
                    sys::mln_map_copy_layer_source_id_take_result(operation, &mut buffer)
                })
                .map_err(map_error)?;
                owned_buffer_to_string(buffer)
            },
        )
    }

    fn set_layer_min_zoom(&self, layer_id: String, min_zoom: f64) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_layer_min_zoom(map, layer_id.raw(), min_zoom, out)
        })
    }

    fn set_layer_max_zoom(&self, layer_id: String, max_zoom: f64) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_layer_max_zoom(map, layer_id.raw(), max_zoom, out)
        })
    }

    fn set_layer_visibility(&self, layer_id: String, visibility: u32) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_layer_visibility(map, layer_id.raw(), visibility, out)
        })
    }

    fn set_layer_filter(
        &self,
        layer_id: String,
        filter: Option<&Bound<'_, PyBytes>>,
    ) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let filter = filter.map(|value| maplibre_core::string::buffer_view(value.as_bytes()));
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_layer_filter(
                map,
                layer_id.raw(),
                optional_ref_ptr(filter.as_ref()),
                out,
            )
        })
    }

    fn get_layer_filter(&self, py: Python<'_>, layer_id: String) -> PyResult<Option<Py<PyBytes>>> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        self.run_style_operation(
            py,
            |map, out| unsafe { sys::mln_map_get_layer_filter_start(map, layer_id.raw(), out) },
            |operation| {
                let mut buffer = sys::mln_buffer(0);
                maplibre_core::check(unsafe {
                    sys::mln_map_get_layer_filter_take_result(operation, &mut buffer)
                })
                .map_err(map_error)?;
                if buffer.0 == 0 {
                    Ok(None)
                } else {
                    owned_buffer_to_py(py, buffer).map(Some)
                }
            },
        )
    }

    fn remove_style_layer(&self, layer_id: String) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_remove_style_layer(map, layer_id.raw(), out)
        })
    }

    fn get_style_layer_info(
        &self,
        py: Python<'_>,
        layer_id: String,
    ) -> PyResult<Option<Py<PyAny>>> {
        let layer_id_view = maplibre_core::string::string_view(&layer_id);
        let (info, found) = self.run_style_operation(
            py,
            |map, out| unsafe {
                sys::mln_map_get_style_layer_info_start(map, layer_id_view.raw(), out)
            },
            |operation| {
                let mut info: sys::mln_style_layer_info = unsafe { std::mem::zeroed() };
                info.size = std::mem::size_of::<sys::mln_style_layer_info>() as u32;
                let mut found = false;
                maplibre_core::check(unsafe {
                    sys::mln_map_get_style_layer_info_take_result(operation, &mut info, &mut found)
                })
                .map_err(map_error)?;
                Ok((info, found))
            },
        )?;
        if !found {
            return Ok(None);
        }
        // The type view names a static style-spec string that stays valid for
        // the life of the process; copy it into a Python-owned string.
        let layer_type = if info.type_.data.is_null() || info.type_.size == 0 {
            String::new()
        } else {
            let bytes = unsafe {
                std::slice::from_raw_parts(info.type_.data.cast::<u8>(), info.type_.size)
            };
            String::from_utf8_lossy(bytes).into_owned()
        };
        // The field bits report whether the layer carries the strings and the
        // sizes report their lengths; the unchanged copy operations transfer
        // any non-empty contents.
        let source_id = if info.fields & sys::MLN_STYLE_LAYER_INFO_SOURCE_ID != 0 {
            Some(if info.source_id_size > 0 {
                self.copy_layer_source_id(py, layer_id.clone())?
            } else {
                String::new()
            })
        } else {
            None
        };
        let source_layer = if info.fields & sys::MLN_STYLE_LAYER_INFO_SOURCE_LAYER != 0 {
            Some(if info.source_layer_size > 0 {
                self.copy_layer_source_layer(py, layer_id.clone())?
            } else {
                String::new()
            })
        } else {
            None
        };
        let dict = PyDict::new(py);
        dict.set_item("layer_type", layer_type)?;
        dict.set_item("min_zoom", info.min_zoom)?;
        dict.set_item("max_zoom", info.max_zoom)?;
        dict.set_item("visibility", info.visibility)?;
        dict.set_item("source_id", source_id)?;
        dict.set_item("source_layer", source_layer)?;
        Ok(Some(dict.into_any().unbind()))
    }

    fn list_style_layer_ids(&self, py: Python<'_>) -> PyResult<Vec<String>> {
        self.run_style_operation(
            py,
            |map, out| unsafe { sys::mln_map_list_style_layer_ids_start(map, out) },
            |operation| {
                let mut out = maplibre_core::ptr::OutHandle::<sys::mln_style_id_list>::new();
                maplibre_core::check(unsafe {
                    sys::mln_map_list_style_layer_ids_take_result(operation, out.as_mut_ptr())
                })
                .map_err(map_error)?;
                let native = out.into_live("mln_style_id_list").map_err(map_error)?;
                unsafe { maplibre_core::style::copy_style_id_list(native) }.map_err(map_error)
            },
        )
    }

    fn move_style_layer(&self, layer_id: String, before_layer_id: Option<String>) -> PyResult<u64> {
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = maplibre_core::string::string_view(&before_layer_id);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_move_style_layer(map, layer_id.raw(), before_layer_id.raw(), out)
        })
    }

    #[allow(clippy::too_many_arguments)]
    fn set_style_image(
        &self,
        image_id: String,
        width: u32,
        height: u32,
        stride: u32,
        pixels: Vec<u8>,
        pixel_ratio: Option<f32>,
        sdf: Option<bool>,
        stretch_x: Option<Vec<(f32, f32)>>,
        stretch_y: Option<Vec<(f32, f32)>>,
        content: Option<(f32, f32, f32, f32)>,
        text_fit_width: Option<u32>,
        text_fit_height: Option<u32>,
    ) -> PyResult<u64> {
        let image_id = maplibre_core::string::string_view(&image_id);
        let mut image = unsafe { sys::mln_premultiplied_rgba8_image_default() };
        image.width = width;
        image.height = height;
        image.stride = stride;
        image.pixels = pixels.as_ptr();
        image.byte_length = pixels.len();
        let (mut options, native_stretch_x, native_stretch_y) = style_image_options_from_parts(
            pixel_ratio,
            sdf,
            stretch_x,
            stretch_y,
            content,
            text_fit_width,
            text_fit_height,
        );
        options.stretch_x = native_stretch_x.as_ptr();
        options.stretch_y = native_stretch_y.as_ptr();
        // SAFETY: The C API validates the map pointer, image ID, image descriptor,
        // options, and pixel storage. pixels is retained for this call.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_style_image(map, image_id.raw(), &image, &options, out)
        })
    }

    fn remove_style_image(&self, image_id: String) -> PyResult<u64> {
        let image_id = maplibre_core::string::string_view(&image_id);
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_remove_style_image(map, image_id.raw(), out)
        })
    }

    fn copy_style_image_stretches(
        &self,
        py: Python<'_>,
        image_id: String,
    ) -> PyResult<Option<(Vec<(f32, f32)>, Vec<(f32, f32)>)>> {
        let image_id = maplibre_core::string::string_view(&image_id);
        self.run_style_operation(
            py,
            |map, out| unsafe {
                sys::mln_map_copy_style_image_stretches_start(map, image_id.raw(), out)
            },
            |operation| {
                let mut x_count = 0;
                let mut y_count = 0;
                let mut found = false;
                maplibre_core::check(unsafe {
                    sys::mln_map_copy_style_image_stretches_take_result(
                        operation,
                        ptr::null_mut(),
                        0,
                        &mut x_count,
                        ptr::null_mut(),
                        0,
                        &mut y_count,
                        &mut found,
                    )
                })
                .map_err(map_error)?;
                if !found {
                    return Ok(None);
                }
                let mut raw_x = vec![sys::mln_image_stretch { from: 0.0, to: 0.0 }; x_count];
                let mut raw_y = vec![sys::mln_image_stretch { from: 0.0, to: 0.0 }; y_count];
                maplibre_core::check(unsafe {
                    sys::mln_map_copy_style_image_stretches_take_result(
                        operation,
                        raw_x.as_mut_ptr(),
                        raw_x.len(),
                        &mut x_count,
                        raw_y.as_mut_ptr(),
                        raw_y.len(),
                        &mut y_count,
                        &mut found,
                    )
                })
                .map_err(map_error)?;
                raw_x.truncate(x_count);
                raw_y.truncate(y_count);
                let copy = |values: &[sys::mln_image_stretch]| {
                    values.iter().map(|value| (value.from, value.to)).collect()
                };
                Ok(Some((copy(&raw_x), copy(&raw_y))))
            },
        )
    }

    fn get_style_image_info(
        &self,
        py: Python<'_>,
        image_id: String,
    ) -> PyResult<Option<Py<PyAny>>> {
        let image_id = maplibre_core::string::string_view(&image_id);
        self.run_style_operation(
            py,
            |map, out| unsafe { sys::mln_map_get_style_image_info_start(map, image_id.raw(), out) },
            |operation| {
                let mut info = maplibre_core::style::empty_style_image_info();
                let mut found = false;
                maplibre_core::check(unsafe {
                    sys::mln_map_get_style_image_info_take_result(operation, &mut info, &mut found)
                })
                .map_err(map_error)?;
                if found {
                    style_image_info_to_py(py, &info).map(Some)
                } else {
                    Ok(None)
                }
            },
        )
    }

    fn copy_style_image_premultiplied_rgba8(
        &self,
        py: Python<'_>,
        image_id: String,
    ) -> PyResult<Option<Py<PyAny>>> {
        let info = self.get_style_image_info(py, image_id.clone())?;
        let image_id = maplibre_core::string::string_view(&image_id);
        let Some(info) = info else {
            return Ok(None);
        };
        self.run_style_operation(
            py,
            |map, out| unsafe {
                sys::mln_map_copy_style_image_premultiplied_rgba8_start(map, image_id.raw(), out)
            },
            |operation| {
                let mut buffer = sys::mln_buffer(0);
                let mut found = false;
                maplibre_core::check(unsafe {
                    sys::mln_map_copy_style_image_premultiplied_rgba8_take_result(
                        operation,
                        &mut buffer,
                        &mut found,
                    )
                })
                .map_err(map_error)?;
                if !found {
                    return Ok(None);
                }
                let pixels = unsafe { maplibre_core::string::copy_owned_buffer(buffer) }
                    .map_err(map_error)?;
                let raw = info.bind(py);
                let dict = raw.cast::<PyDict>()?;
                dict.set_item("pixels", PyBytes::new(py, &pixels))?;
                Ok(Some(info))
            },
        )
    }

    fn add_image_source_url(
        &self,
        source_id: String,
        coordinates: Vec<(f64, f64)>,
        url: String,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let url = maplibre_core::string::string_view(&url);
        let coordinates = lat_lngs_from_tuples(coordinates);
        // SAFETY: The C API validates the map pointer, source ID, coordinate slice, and URL.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_add_image_source_url(
                map,
                source_id.raw(),
                coordinates.as_ptr(),
                coordinates.len(),
                url.raw(),
                out,
            )
        })
    }

    fn add_image_source_image(
        &self,
        source_id: String,
        coordinates: Vec<(f64, f64)>,
        width: u32,
        height: u32,
        stride: u32,
        pixels: Vec<u8>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let coordinates = lat_lngs_from_tuples(coordinates);
        let image = premultiplied_rgba8_image_from_parts(width, height, stride, &pixels);
        // SAFETY: The C API validates the map pointer, source ID, coordinates,
        // image descriptor, and pixel storage. pixels is retained for this call.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_add_image_source_image(
                map,
                source_id.raw(),
                coordinates.as_ptr(),
                coordinates.len(),
                &image,
                out,
            )
        })
    }

    fn set_image_source_url(&self, source_id: String, url: String) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let url = maplibre_core::string::string_view(&url);
        // SAFETY: The C API validates the map pointer and borrowed string views.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_image_source_url(map, source_id.raw(), url.raw(), out)
        })
    }

    fn set_image_source_image(
        &self,
        source_id: String,
        width: u32,
        height: u32,
        stride: u32,
        pixels: Vec<u8>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let image = premultiplied_rgba8_image_from_parts(width, height, stride, &pixels);
        // SAFETY: The C API validates the map pointer, source ID, image descriptor,
        // and pixel storage. pixels is retained for this call.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_image_source_image(map, source_id.raw(), &image, out)
        })
    }

    fn set_image_source_coordinates(
        &self,
        source_id: String,
        coordinates: Vec<(f64, f64)>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let coordinates = lat_lngs_from_tuples(coordinates);
        // SAFETY: The C API validates the map pointer, source ID, and coordinate slice.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_image_source_coordinates(
                map,
                source_id.raw(),
                coordinates.as_ptr(),
                coordinates.len(),
                out,
            )
        })
    }

    fn get_image_source_coordinates(
        &self,
        py: Python<'_>,
        source_id: String,
    ) -> PyResult<Option<Py<PyAny>>> {
        let source_id = maplibre_core::string::string_view(&source_id);
        self.run_style_operation(
            py,
            |map, out| unsafe {
                sys::mln_map_get_image_source_coordinates_start(map, source_id.raw(), out)
            },
            |operation| {
                let mut coordinates = vec![
                    sys::mln_lat_lng {
                        latitude: 0.0,
                        longitude: 0.0,
                    };
                    4
                ];
                let mut count = 0;
                let mut found = false;
                maplibre_core::check(unsafe {
                    sys::mln_map_get_image_source_coordinates_take_result(
                        operation,
                        coordinates.as_mut_ptr(),
                        coordinates.len(),
                        &mut count,
                        &mut found,
                    )
                })
                .map_err(map_error)?;
                if !found {
                    return Ok(None);
                }
                coordinates.truncate(count);
                lat_lng_list_to_py(py, &coordinates).map(Some)
            },
        )
    }

    #[allow(clippy::too_many_arguments)]
    fn add_custom_geometry_source(
        &self,
        source_id: String,
        max_queued_events: usize,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        tolerance: Option<f64>,
        tile_size: Option<u32>,
        buffer: Option<u32>,
        clip: Option<bool>,
        wrap: Option<bool>,
        has_cancel_tile: bool,
    ) -> PyResult<(CustomGeometrySourceHandle, u64)> {
        if max_queued_events == 0 {
            return Err(invalid_argument_error(
                "max_queued_events must be greater than zero",
            ));
        }
        let state = PyCustomGeometrySourceState::new(
            max_queued_events,
            min_zoom,
            max_zoom,
            tolerance,
            tile_size,
            buffer,
            clip,
            wrap,
            has_cancel_tile,
        );
        let descriptor = state.descriptor();
        let handle = CustomGeometrySourceHandle {
            shared: Arc::clone(&state.shared),
        };
        let source_id_view = maplibre_core::string::string_view(&source_id);
        let map_state = self.state();
        let mut command_id = 0;
        // SAFETY: map_state owns or has released the map pointer. The C API
        // validates that it is live. source_id_view and descriptor are valid for
        // this call, and descriptor's release callback takes the leaked state
        // back once the C API stops referencing it.
        maplibre_core::check(unsafe {
            sys::mln_map_add_custom_geometry_source(
                map_state.handle(),
                source_id_view.raw(),
                &descriptor,
                &mut command_id,
            )
        })
        .map_err(map_error)?;
        // A rejected add owes no release, so the state only leaves this box
        // after the C API accepted it.
        Box::leak(state);
        Ok((handle, command_id))
    }

    fn set_custom_geometry_source_tile_data(
        &self,
        source_id: String,
        z: u32,
        x: u32,
        y: u32,
        data: &Bound<'_, PyBytes>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let data = maplibre_core::string::buffer_view(data.as_bytes());
        // SAFETY: The C API validates the map pointer, source ID, tile ID, and GeoJSON buffer view.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_set_custom_geometry_source_tile_data(
                map,
                source_id.raw(),
                sys::mln_canonical_tile_id { z, x, y },
                data,
                out,
            )
        })
    }

    fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: String,
        z: u32,
        x: u32,
        y: u32,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        // SAFETY: The C API validates the map pointer, source ID, and tile ID.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_tile(
                map,
                source_id.raw(),
                sys::mln_canonical_tile_id { z, x, y },
                out,
            )
        })
    }

    fn invalidate_custom_geometry_source_region(
        &self,
        source_id: String,
        southwest: (f64, f64),
        northeast: (f64, f64),
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let bounds = lat_lng_bounds_from_tuple((southwest, northeast));
        // SAFETY: The C API validates the map pointer, source ID, and bounds.
        self.run_style_command(|map, out| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(map, source_id.raw(), bounds, out)
        })
    }

    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
}

// Every post-creation projection call is a synchronous immediate that takes
// the projection's internal serialization mutex, so each call copies the raw
// handle out of the binding's state lock and releases the GIL around the
// native call. A stalled call on another thread therefore cannot deadlock the
// interpreter, and a concurrent close is safe because retired handles report
// MLN_STATUS_INVALID_ARGUMENT rather than dangling.
impl MapProjectionHandle {
    fn native_handle(&self) -> sys::mln_map_projection {
        self.state().handle()
    }
}

#[pymethods]
impl MapProjectionHandle {
    fn close(&self, py: Python<'_>) -> PyResult<()> {
        let handle = {
            let state = self.state();
            let Some(handle) = state.live_handle() else {
                return Ok(());
            };
            state.mark_closed();
            handle
        };
        let status = py.detach(|| unsafe { sys::mln_map_projection_close(handle) });
        maplibre_core::check(status).map_err(map_error)
    }

    fn get_camera(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let handle = self.native_handle();
        let mut camera = unsafe { sys::mln_camera_options_default() };
        let status =
            py.detach(|| unsafe { sys::mln_map_projection_get_camera(handle, &mut camera) });
        maplibre_core::check(status).map_err(map_error)?;
        camera_options_to_py(py, &camera)
    }

    #[allow(clippy::too_many_arguments)]
    fn set_camera(
        &self,
        py: Python<'_>,
        center: Option<(f64, f64)>,
        zoom: Option<f64>,
        bearing: Option<f64>,
        pitch: Option<f64>,
        center_altitude: Option<f64>,
        padding: Option<(f64, f64, f64, f64)>,
        anchor: Option<(f64, f64)>,
        roll: Option<f64>,
        field_of_view: Option<f64>,
    ) -> PyResult<()> {
        let handle = self.native_handle();
        let camera = camera_options_from_parts(
            center,
            zoom,
            bearing,
            pitch,
            center_altitude,
            padding,
            anchor,
            roll,
            field_of_view,
        );
        let status = py.detach(|| unsafe { sys::mln_map_projection_set_camera(handle, &camera) });
        maplibre_core::check(status).map_err(map_error)
    }

    fn set_visible_coordinates(
        &self,
        py: Python<'_>,
        coordinates: Vec<(f64, f64)>,
        padding: (f64, f64, f64, f64),
    ) -> PyResult<()> {
        let handle = self.native_handle();
        let coordinates: Vec<sys::mln_lat_lng> = coordinates
            .into_iter()
            .map(|(latitude, longitude)| sys::mln_lat_lng {
                latitude,
                longitude,
            })
            .collect();
        let padding = edge_insets_from_tuple(padding);
        let status = py.detach(|| unsafe {
            sys::mln_map_projection_set_visible_coordinates(
                handle,
                coordinates.as_ptr(),
                coordinates.len(),
                padding,
            )
        });
        maplibre_core::check(status).map_err(map_error)
    }

    fn set_visible_geometry(
        &self,
        py: Python<'_>,
        geometry: &Bound<'_, PyBytes>,
        padding: (f64, f64, f64, f64),
    ) -> PyResult<()> {
        let handle = self.native_handle();
        // Copy the bytes out so the borrowed buffer cannot move while the GIL
        // is released.
        let geometry = geometry.as_bytes().to_vec();
        let padding = edge_insets_from_tuple(padding);
        let status = py.detach(|| unsafe {
            sys::mln_map_projection_set_visible_geometry(
                handle,
                maplibre_core::string::buffer_view(&geometry),
                padding,
            )
        });
        maplibre_core::check(status).map_err(map_error)
    }

    fn pixel_for_lat_lng(
        &self,
        py: Python<'_>,
        latitude: f64,
        longitude: f64,
    ) -> PyResult<Py<PyAny>> {
        let handle = self.native_handle();
        let coordinate = sys::mln_lat_lng {
            latitude,
            longitude,
        };
        let mut point = sys::mln_screen_point { x: 0.0, y: 0.0 };
        let status = py.detach(|| unsafe {
            sys::mln_map_projection_pixel_for_lat_lng(handle, coordinate, &mut point)
        });
        maplibre_core::check(status).map_err(map_error)?;
        screen_point_to_py(py, point)
    }

    fn lat_lng_for_pixel(&self, py: Python<'_>, x: f64, y: f64) -> PyResult<Py<PyAny>> {
        let handle = self.native_handle();
        let mut coordinate = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        let status = py.detach(|| unsafe {
            sys::mln_map_projection_lat_lng_for_pixel(
                handle,
                sys::mln_screen_point { x, y },
                &mut coordinate,
            )
        });
        maplibre_core::check(status).map_err(map_error)?;
        lat_lng_to_py(py, coordinate)
    }

    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
}

impl RenderSessionHandle {
    fn native(&self) -> sys::mln_render_session {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .native()
    }

    fn start_target<D>(
        &self,
        raw: D,
        start: impl FnOnce(sys::mln_render_session, &D, *mut sys::mln_operation) -> sys::mln_status,
    ) -> PyResult<u64> {
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(start(self.native(), &raw, &mut operation)).map_err(map_error)?;
        Ok(operation.0)
    }
}

#[pymethods]
impl RenderSessionHandle {
    fn close(&self) -> PyResult<()> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if state.is_closed() {
            return Ok(());
        }
        unsafe { state.handle.close_status(sys::mln_render_session_destroy) }.map_err(map_error)
    }

    fn resize_start(&self, width: u32, height: u32, scale_factor: f64) -> PyResult<u64> {
        let extent = sys::mln_render_target_extent {
            size: std::mem::size_of::<sys::mln_render_target_extent>() as u32,
            width,
            height,
            scale_factor,
        };
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_resize_start(self.native(), &extent, &mut operation)
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn barrier_start(&self, min_update_generation: u64) -> PyResult<u64> {
        start_session_operation(self.native(), |session, operation| unsafe {
            sys::mln_render_session_barrier_start(session, min_update_generation, operation)
        })
    }

    fn reduce_memory_use_start(&self) -> PyResult<u64> {
        start_session_operation(self.native(), |session, operation| unsafe {
            sys::mln_render_session_reduce_memory_use_start(session, operation)
        })
    }

    fn clear_data_start(&self) -> PyResult<u64> {
        start_session_operation(self.native(), |session, operation| unsafe {
            sys::mln_render_session_clear_data_start(session, operation)
        })
    }

    fn dump_debug_logs_start(&self) -> PyResult<u64> {
        start_session_operation(self.native(), |session, operation| unsafe {
            sys::mln_render_session_dump_debug_logs_start(session, operation)
        })
    }

    fn detach_start(&self) -> PyResult<u64> {
        start_session_operation(self.native(), |session, operation| unsafe {
            sys::mln_render_session_detach_start(session, operation)
        })
    }

    fn abandon(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut result: sys::mln_render_abandon_result = unsafe { std::mem::zeroed() };
        result.size = std::mem::size_of::<sys::mln_render_abandon_result>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_render_session_abandon(self.native(), &mut result)
        })
        .map_err(map_error)?;
        let dict = PyDict::new(py);
        dict.set_item("disposition", result.disposition)?;
        dict.set_item(
            "quarantined_resource_count",
            result.quarantined_resource_count,
        )?;
        Ok(dict.into_any().unbind())
    }

    fn service_driver_work(&self, max_work: usize) -> PyResult<usize> {
        let mut serviced = 0;
        maplibre_core::check(unsafe {
            sys::mln_render_session_service_driver_work(self.native(), max_work, &mut serviced)
        })
        .map_err(map_error)?;
        Ok(serviced)
    }

    fn request_frame(
        &self,
        flags: u32,
        token: u64,
        coalescing_boundary: u64,
        deadline_ns: i64,
    ) -> PyResult<()> {
        let demand = sys::mln_frame_demand {
            size: std::mem::size_of::<sys::mln_frame_demand>() as u32,
            flags,
            token,
            coalescing_boundary,
            deadline_ns,
        };
        maplibre_core::check(unsafe {
            sys::mln_render_session_request_frame(self.native(), &demand)
        })
        .map_err(map_error)
    }

    fn capabilities(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut value: sys::mln_render_session_capabilities = unsafe { std::mem::zeroed() };
        value.size = std::mem::size_of::<sys::mln_render_session_capabilities>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_render_session_get_capabilities(self.native(), &mut value)
        })
        .map_err(map_error)?;
        let dict = PyDict::new(py);
        dict.set_item("driver", value.driver)?;
        dict.set_item("texture_ring_depth", value.texture_ring_depth)?;
        dict.set_item("flags", value.flags)?;
        Ok(dict.into_any().unbind())
    }

    fn snapshot(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let mut value: sys::mln_render_session_snapshot = unsafe { std::mem::zeroed() };
        value.size = std::mem::size_of::<sys::mln_render_session_snapshot>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_render_session_get_snapshot(self.native(), &mut value)
        })
        .map_err(map_error)?;
        render_session_snapshot_to_py(py, &value)
    }

    fn drain_frame_results(&self, py: Python<'_>) -> PyResult<Py<PyList>> {
        let mut batch = sys::mln_render_frame_batch(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_drain_frame_results(self.native(), &mut batch)
        })
        .map_err(map_error)?;
        let batch = OwnedRenderFrameBatch(batch);
        let mut count = 0;
        maplibre_core::check(unsafe { sys::mln_render_frame_batch_count(batch.0, &mut count) })
            .map_err(map_error)?;
        let results = PyList::empty(py);
        for index in 0..count {
            let mut value: sys::mln_render_frame_result = unsafe { std::mem::zeroed() };
            value.size = std::mem::size_of::<sys::mln_render_frame_result>() as u32;
            maplibre_core::check(unsafe {
                sys::mln_render_frame_batch_get(batch.0, index, &mut value)
            })
            .map_err(map_error)?;
            results.append(render_frame_result_to_py(py, &value)?)?;
        }
        Ok(results.unbind())
    }

    fn set_metal_surface_target(
        &self,
        width: u32,
        height: u32,
        scale_factor: f64,
        device_address: usize,
        layer_address: usize,
    ) -> PyResult<u64> {
        let descriptor = maplibre_core::render::metal_surface_descriptor_to_native(
            maplibre_core::render::MetalSurfaceDescriptorFields {
                extent: maplibre_core::render::RenderTargetExtentFields {
                    width,
                    height,
                    scale_factor,
                },
                context: maplibre_core::render::MetalContextDescriptorFields {
                    device: device_address as *mut c_void,
                },
                layer: layer_address as *mut c_void,
            },
        );
        self.start_target(descriptor, |session, raw, operation| {
            // SAFETY: raw and operation are valid for this copied submission.
            unsafe { sys::mln_metal_surface_set_target_start(session, raw, operation) }
        })
    }

    #[allow(clippy::too_many_arguments)]
    fn set_vulkan_surface_target(
        &self,
        width: u32,
        height: u32,
        scale_factor: f64,
        instance_address: usize,
        physical_device_address: usize,
        device_address: usize,
        graphics_queue_address: usize,
        graphics_queue_family_index: u32,
        get_instance_proc_addr: usize,
        get_device_proc_addr: usize,
        surface_address: usize,
    ) -> PyResult<u64> {
        let descriptor = maplibre_core::render::vulkan_surface_descriptor_to_native(
            maplibre_core::render::VulkanSurfaceDescriptorFields {
                extent: maplibre_core::render::RenderTargetExtentFields {
                    width,
                    height,
                    scale_factor,
                },
                context: vulkan_context_fields(
                    instance_address,
                    physical_device_address,
                    device_address,
                    graphics_queue_address,
                    graphics_queue_family_index,
                    get_instance_proc_addr,
                    get_device_proc_addr,
                ),
                surface: surface_address as *mut c_void,
            },
        );
        self.start_target(descriptor, |session, raw, operation| {
            // SAFETY: raw and operation are valid for this copied submission.
            unsafe { sys::mln_vulkan_surface_set_target_start(session, raw, operation) }
        })
    }

    #[allow(clippy::too_many_arguments)]
    fn set_webgpu_surface_target(
        &self,
        width: u32,
        height: u32,
        scale_factor: f64,
        instance_address: usize,
        device_address: usize,
        queue_address: usize,
        surface_address: usize,
        format: u32,
    ) -> PyResult<u64> {
        let descriptor = maplibre_core::render::webgpu_surface_descriptor_to_native(
            maplibre_core::render::WebGpuSurfaceDescriptorFields {
                extent: maplibre_core::render::RenderTargetExtentFields {
                    width,
                    height,
                    scale_factor,
                },
                context: webgpu_context_fields(instance_address, device_address, queue_address),
                surface: surface_address as *mut c_void,
                format,
            },
        );
        self.start_target(descriptor, |session, raw, operation| unsafe {
            sys::mln_webgpu_surface_set_target_start(session, raw, operation)
        })
    }

    #[allow(clippy::too_many_arguments)]
    fn set_opengl_surface_target(
        &self,
        width: u32,
        height: u32,
        scale_factor: f64,
        context_platform: u32,
        context_ownership: u32,
        context_address_1: usize,
        context_address_2: usize,
        share_context_address: usize,
        client_api: u32,
        get_proc_address: usize,
        surface_address: usize,
    ) -> PyResult<u64> {
        let descriptor = maplibre_core::render::opengl_surface_descriptor_to_native(
            maplibre_core::render::OpenGLSurfaceDescriptorFields {
                extent: maplibre_core::render::RenderTargetExtentFields {
                    width,
                    height,
                    scale_factor,
                },
                context: opengl_context_fields(
                    context_platform,
                    context_ownership,
                    context_address_1,
                    context_address_2,
                    share_context_address,
                    client_api,
                    get_proc_address,
                )?,
                surface: surface_address as *mut c_void,
            },
        );
        self.start_target(descriptor, |session, raw, operation| {
            // SAFETY: raw and operation are valid for this copied submission.
            unsafe { sys::mln_opengl_surface_set_target_start(session, raw, operation) }
        })
    }

    fn set_metal_borrowed_texture_target(
        &self,
        width: u32,
        height: u32,
        scale_factor: f64,
        physical_width: u32,
        physical_height: u32,
        texture_address: usize,
    ) -> PyResult<u64> {
        let descriptor = maplibre_core::render::metal_borrowed_texture_descriptor_to_native(
            maplibre_core::render::MetalBorrowedTextureDescriptorFields {
                extent: maplibre_core::render::RenderTargetExtentFields {
                    width,
                    height,
                    scale_factor,
                },
                physical_width,
                physical_height,
                texture: texture_address as *mut c_void,
            },
        );
        self.start_target(descriptor, |session, raw, operation| {
            // SAFETY: raw and operation are valid for this copied submission.
            unsafe { sys::mln_metal_borrowed_texture_set_target_start(session, raw, operation) }
        })
    }

    #[allow(clippy::too_many_arguments)]
    fn set_vulkan_borrowed_texture_target(
        &self,
        width: u32,
        height: u32,
        scale_factor: f64,
        physical_width: u32,
        physical_height: u32,
        instance_address: usize,
        physical_device_address: usize,
        device_address: usize,
        graphics_queue_address: usize,
        graphics_queue_family_index: u32,
        get_instance_proc_addr: usize,
        get_device_proc_addr: usize,
        image_address: usize,
        image_view_address: usize,
        format: u32,
        initial_layout: u32,
        final_layout: u32,
    ) -> PyResult<u64> {
        let descriptor = maplibre_core::render::vulkan_borrowed_texture_descriptor_to_native(
            maplibre_core::render::VulkanBorrowedTextureDescriptorFields {
                extent: maplibre_core::render::RenderTargetExtentFields {
                    width,
                    height,
                    scale_factor,
                },
                physical_width,
                physical_height,
                context: vulkan_context_fields(
                    instance_address,
                    physical_device_address,
                    device_address,
                    graphics_queue_address,
                    graphics_queue_family_index,
                    get_instance_proc_addr,
                    get_device_proc_addr,
                ),
                image: image_address as *mut c_void,
                image_view: image_view_address as *mut c_void,
                format,
                initial_layout,
                final_layout,
            },
        );
        self.start_target(descriptor, |session, raw, operation| {
            // SAFETY: raw and operation are valid for this copied submission.
            unsafe { sys::mln_vulkan_borrowed_texture_set_target_start(session, raw, operation) }
        })
    }

    #[allow(clippy::too_many_arguments)]
    fn set_webgpu_borrowed_texture_target(
        &self,
        width: u32,
        height: u32,
        scale_factor: f64,
        physical_width: u32,
        physical_height: u32,
        instance_address: usize,
        device_address: usize,
        queue_address: usize,
        texture_address: usize,
        texture_view_address: usize,
        format: u32,
    ) -> PyResult<u64> {
        let descriptor = maplibre_core::render::webgpu_borrowed_texture_descriptor_to_native(
            maplibre_core::render::WebGpuBorrowedTextureDescriptorFields {
                extent: maplibre_core::render::RenderTargetExtentFields {
                    width,
                    height,
                    scale_factor,
                },
                physical_width,
                physical_height,
                context: webgpu_context_fields(instance_address, device_address, queue_address),
                texture: texture_address as *mut c_void,
                texture_view: texture_view_address as *mut c_void,
                format,
            },
        );
        self.start_target(descriptor, |session, raw, operation| unsafe {
            sys::mln_webgpu_borrowed_texture_set_target_start(session, raw, operation)
        })
    }

    #[allow(clippy::too_many_arguments)]
    fn set_opengl_borrowed_texture_target(
        &self,
        width: u32,
        height: u32,
        scale_factor: f64,
        physical_width: u32,
        physical_height: u32,
        context_platform: u32,
        context_ownership: u32,
        context_address_1: usize,
        context_address_2: usize,
        share_context_address: usize,
        client_api: u32,
        get_proc_address: usize,
        texture: u32,
        target: u32,
    ) -> PyResult<u64> {
        let descriptor = maplibre_core::render::opengl_borrowed_texture_descriptor_to_native(
            maplibre_core::render::OpenGLBorrowedTextureDescriptorFields {
                extent: maplibre_core::render::RenderTargetExtentFields {
                    width,
                    height,
                    scale_factor,
                },
                physical_width,
                physical_height,
                context: opengl_context_fields(
                    context_platform,
                    context_ownership,
                    context_address_1,
                    context_address_2,
                    share_context_address,
                    client_api,
                    get_proc_address,
                )?,
                texture,
                target,
            },
        );
        self.start_target(descriptor, |session, raw, operation| {
            // SAFETY: raw and operation are valid for this copied submission.
            unsafe { sys::mln_opengl_borrowed_texture_set_target_start(session, raw, operation) }
        })
    }

    fn query_rendered_features_start(
        &self,
        geometry: &Bound<'_, PyAny>,
        layer_ids: Option<Vec<String>>,
        filter: Option<&Bound<'_, PyBytes>>,
    ) -> PyResult<u64> {
        let geometry = rendered_query_geometry_from_wire(geometry)?;
        let geometry = maplibre_core::query::rendered_query_geometry_to_native(&geometry);
        let mut options = maplibre_core::RenderedFeatureQueryOptions::default();
        options.layer_ids = layer_ids;
        options.filter = filter.map(|value| value.as_bytes().to_vec());
        let options = maplibre_core::query::rendered_feature_query_options_to_native(&options)
            .map_err(map_error)?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_rendered_features_start(
                self.native(),
                geometry.as_ptr(),
                options.as_ptr(),
                &mut operation,
            )
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn query_source_features_start(
        &self,
        source_id: String,
        source_layer_ids: Option<Vec<String>>,
        filter: Option<&Bound<'_, PyBytes>>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let mut options = maplibre_core::SourceFeatureQueryOptions::default();
        options.source_layer_ids = source_layer_ids;
        options.filter = filter.map(|value| value.as_bytes().to_vec());
        let options = maplibre_core::query::source_feature_query_options_to_native(&options)
            .map_err(map_error)?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_source_features_start(
                self.native(),
                source_id.raw(),
                options.as_ptr(),
                &mut operation,
            )
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn query_feature_extensions_start(
        &self,
        source_id: String,
        feature: &Bound<'_, PyBytes>,
        extension: String,
        extension_field: String,
        arguments: Option<&Bound<'_, PyBytes>>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let feature = maplibre_core::string::buffer_view(feature.as_bytes());
        let extension = maplibre_core::string::string_view(&extension);
        let extension_field = maplibre_core::string::string_view(&extension_field);
        let arguments = arguments.map(|value| maplibre_core::string::buffer_view(value.as_bytes()));
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_feature_extensions_start(
                self.native(),
                source_id.raw(),
                feature,
                extension.raw(),
                extension_field.raw(),
                optional_ref_ptr(arguments.as_ref()),
                &mut operation,
            )
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn render_query_take_result(&self, py: Python<'_>, operation: u64) -> PyResult<Py<PyBytes>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        maplibre_core::check(unsafe {
            sys::mln_render_query_take_result(sys::mln_operation(operation), out.as_mut_ptr())
        })
        .map_err(map_error)
        .map_err(operation_result_consumed)?;
        owned_buffer_to_py(py, out.get())
    }

    fn render_query_features_take_result(
        &self,
        py: Python<'_>,
        operation: u64,
    ) -> PyResult<Py<PyList>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_queried_feature_list>::new();
        maplibre_core::check(unsafe {
            sys::mln_render_query_features_take_result(
                sys::mln_operation(operation),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)
        .map_err(operation_result_consumed)?;
        let native = out
            .into_live("mln_queried_feature_list")
            .map_err(map_error)?;
        // SAFETY: native is an owned queried-feature list returned by native.
        let features = unsafe { maplibre_core::query::copy_queried_feature_list(native) }
            .map_err(map_error)?;
        queried_features_to_py(py, features)
    }

    fn set_feature_state_start(
        &self,
        source_id: String,
        source_layer_id: Option<String>,
        feature_id: Option<String>,
        state_value: &Bound<'_, PyBytes>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let source_layer_id =
            maplibre_core::string::string_view(source_layer_id.as_deref().unwrap_or(""));
        let feature_id = maplibre_core::string::string_view(feature_id.as_deref().unwrap_or(""));
        let state_value = maplibre_core::string::buffer_view(state_value.as_bytes());
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_set_feature_state_start(
                self.native(),
                source_id.raw(),
                source_layer_id.raw(),
                feature_id.raw(),
                state_value,
                &mut operation,
            )
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn get_feature_state_start(
        &self,
        source_id: String,
        source_layer_id: Option<String>,
        feature_id: Option<String>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let source_layer_id =
            maplibre_core::string::string_view(source_layer_id.as_deref().unwrap_or(""));
        let feature_id = maplibre_core::string::string_view(feature_id.as_deref().unwrap_or(""));
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_get_feature_state_start(
                self.native(),
                source_id.raw(),
                source_layer_id.raw(),
                feature_id.raw(),
                &mut operation,
            )
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn get_feature_state_take_result(
        &self,
        py: Python<'_>,
        operation: u64,
    ) -> PyResult<Py<PyBytes>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        maplibre_core::check(unsafe {
            sys::mln_render_session_get_feature_state_take_result(
                sys::mln_operation(operation),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)
        .map_err(operation_result_consumed)?;
        owned_buffer_to_py(py, out.get())
    }

    fn remove_feature_state_start(
        &self,
        source_id: String,
        source_layer_id: Option<String>,
        feature_id: Option<String>,
        state_key: Option<String>,
    ) -> PyResult<u64> {
        let source_id = maplibre_core::string::string_view(&source_id);
        let source_layer_id =
            maplibre_core::string::string_view(source_layer_id.as_deref().unwrap_or(""));
        let feature_id = maplibre_core::string::string_view(feature_id.as_deref().unwrap_or(""));
        let state_key = maplibre_core::string::string_view(state_key.as_deref().unwrap_or(""));
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_remove_feature_state_start(
                self.native(),
                source_id.raw(),
                source_layer_id.raw(),
                feature_id.raw(),
                state_key.raw(),
                &mut operation,
            )
        })
        .map_err(map_error)?;
        Ok(operation.0)
    }

    fn read_premultiplied_rgba8_start(&self) -> PyResult<u64> {
        start_session_operation(self.native(), |session, operation| unsafe {
            sys::mln_texture_read_premultiplied_rgba8_start(session, operation)
        })
    }

    fn read_premultiplied_rgba8_take_result(
        &self,
        py: Python<'_>,
        operation: u64,
    ) -> PyResult<Py<PyAny>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        let mut info = unsafe { sys::mln_texture_image_info_default() };
        maplibre_core::check(unsafe {
            sys::mln_texture_read_premultiplied_rgba8_take_result(
                sys::mln_operation(operation),
                out.as_mut_ptr(),
                &mut info,
            )
        })
        .map_err(map_error)
        .map_err(operation_result_consumed)?;
        let bytes = owned_buffer_to_py(py, out.get())?;
        let dict = PyDict::new(py);
        dict.set_item(
            "info",
            texture_image_info_to_py(
                py,
                maplibre_core::values::texture_image_info_from_native(&info),
            )?
            .bind(py),
        )?;
        dict.set_item("data", bytes.bind(py))?;
        Ok(dict.into_any().unbind())
    }

    fn acquire_metal_owned_texture_frame(
        &self,
        py: Python<'_>,
    ) -> PyResult<Py<MetalOwnedTextureFrameHandle>> {
        let frame = acquire_frame(self.native())?;
        Py::new(
            py,
            MetalOwnedTextureFrameHandle {
                frame: Mutex::new(frame),
            },
        )
    }

    fn acquire_vulkan_owned_texture_frame(
        &self,
        py: Python<'_>,
    ) -> PyResult<Py<VulkanOwnedTextureFrameHandle>> {
        let frame = acquire_frame(self.native())?;
        Py::new(
            py,
            VulkanOwnedTextureFrameHandle {
                frame: Mutex::new(frame),
            },
        )
    }

    fn acquire_opengl_owned_texture_frame(
        &self,
        py: Python<'_>,
    ) -> PyResult<Py<OpenGLOwnedTextureFrameHandle>> {
        let frame = acquire_frame(self.native())?;
        Py::new(
            py,
            OpenGLOwnedTextureFrameHandle {
                frame: Mutex::new(frame),
            },
        )
    }

    fn acquire_webgpu_owned_texture_frame(
        &self,
        py: Python<'_>,
    ) -> PyResult<Py<WebGPUOwnedTextureFrameHandle>> {
        let frame = acquire_frame(self.native())?;
        Py::new(
            py,
            WebGPUOwnedTextureFrameHandle {
                frame: Mutex::new(frame),
            },
        )
    }

    #[getter]
    fn closed(&self) -> bool {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .is_closed()
    }
}

impl Drop for RenderSessionHandle {
    fn drop(&mut self) {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let Some(native) = state.handle.live_handle() else {
            return;
        };
        let mut result: sys::mln_render_abandon_result = unsafe { std::mem::zeroed() };
        result.size = std::mem::size_of::<sys::mln_render_abandon_result>() as u32;
        let status = unsafe { sys::mln_render_session_abandon(native, &mut result) };
        if status == sys::MLN_STATUS_OK || status == sys::MLN_STATUS_TARGET_LOST {
            let _ = unsafe { state.handle.close_status(sys::mln_render_session_destroy) };
        }
    }
}

#[pymethods]
impl LogReceiver {
    fn poll_record(&self, py: Python<'_>) -> PyResult<Option<Py<PyAny>>> {
        let mut queue = self
            .state
            .queue
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let Some(record) = queue.pop_front() else {
            return Ok(None);
        };
        drop(queue);
        log_record_to_py(py, record).map(Some)
    }

    #[getter]
    fn dropped_record_count(&self) -> usize {
        self.state.dropped_records.load(Ordering::Acquire)
    }
}

#[pymethods]
impl CustomGeometrySourceHandle {
    fn close(&self) {
        self.shared.close();
    }

    fn poll_event(&self, py: Python<'_>) -> PyResult<Option<Py<PyAny>>> {
        let mut queue = self
            .shared
            .queue
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let Some(event) = queue.events.pop_front() else {
            return Ok(None);
        };
        drop(queue);
        custom_geometry_event_to_py(py, event).map(Some)
    }

    fn push_fetch_for_test(&self, z: u32, x: u32, y: u32) {
        self.shared
            .enqueue(0, sys::mln_canonical_tile_id { z, x, y });
    }

    fn push_cancel_for_test(&self, z: u32, x: u32, y: u32) {
        self.shared
            .enqueue(1, sys::mln_canonical_tile_id { z, x, y });
    }

    #[getter]
    fn dropped_event_count(&self) -> u64 {
        self.shared
            .queue
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .dropped_events
    }

    #[getter]
    fn closed(&self) -> bool {
        self.shared
            .queue
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .closed
    }
}

#[pymethods]
impl ResourceRequestHandle {
    fn validate_completion_response(&self, response: &Bound<'_, PyAny>) -> PyResult<()> {
        let response = resource_response_from_py(response)?;
        let _native =
            maplibre_core::resource::resource_response_to_native(&response).map_err(map_error)?;
        Ok(())
    }

    fn complete(&self, response: &Bound<'_, PyAny>) -> PyResult<()> {
        let response = resource_response_from_py(response)?;
        self.state.complete(&response).map_err(map_error)
    }

    fn is_cancelled(&self) -> PyResult<bool> {
        self.state.is_cancelled().map_err(map_error)
    }

    fn close(&self) {
        self.state.close();
    }
}

struct CallbackPermit<'a> {
    pending: &'a AtomicUsize,
}

impl Drop for CallbackPermit<'_> {
    fn drop(&mut self) {
        self.pending.fetch_sub(1, Ordering::AcqRel);
    }
}

fn try_acquire_callback_permit(
    pending: &AtomicUsize,
    max_pending_callbacks: usize,
) -> Option<CallbackPermit<'_>> {
    let mut current = pending.load(Ordering::Acquire);
    loop {
        if current >= max_pending_callbacks {
            return None;
        }
        match pending.compare_exchange_weak(
            current,
            current + 1,
            Ordering::AcqRel,
            Ordering::Acquire,
        ) {
            Ok(_) => return Some(CallbackPermit { pending }),
            Err(actual) => current = actual,
        }
    }
}

impl PyResourceProviderState {
    fn new(callback: Py<PyAny>, max_pending_callbacks: usize) -> Self {
        Self {
            callback,
            pending_callbacks: AtomicUsize::new(0),
            max_pending_callbacks,
        }
    }

    fn descriptor(&self) -> sys::mln_resource_provider {
        let mut descriptor = maplibre_core::resource::resource_provider_descriptor(
            Some(resource_provider_trampoline),
            ptr::from_ref(self).cast_mut().cast::<c_void>(),
        );
        descriptor.release_user_data = Some(release_py_resource_provider_state);
        descriptor
    }

    fn invoke(
        &self,
        request: *const sys::mln_resource_request,
        handle: sys::mln_resource_request_handle,
    ) -> u32 {
        let Some(_permit) =
            try_acquire_callback_permit(&self.pending_callbacks, self.max_pending_callbacks)
        else {
            return maplibre_core::resource::UNKNOWN_PROVIDER_DECISION;
        };
        let Some(raw_request) = ptr::NonNull::new(request.cast_mut()) else {
            return maplibre_core::resource::UNKNOWN_PROVIDER_DECISION;
        };
        // SAFETY: handle is received from the C provider callback and paired
        // with the native request-handle functions.
        let handle_state = match unsafe {
            maplibre_core::resource::ResourceRequestHandleState::new(
                handle,
                maplibre_core::resource::ResourceRequestHandleFns::NATIVE,
            )
        } {
            Ok(handle_state) => handle_state,
            Err(_) => return maplibre_core::resource::UNKNOWN_PROVIDER_DECISION,
        };
        // SAFETY: raw_request is non-null and borrowed for callback duration.
        let request =
            match unsafe { maplibre_core::resource::copy_resource_request(raw_request.as_ref()) } {
                Ok(request) => request,
                Err(_) => return handle_state.finish_provider_exception(),
            };

        Python::attach(|py| -> PyResult<u32> {
            let py_request = resource_request_to_py(py, request)?;
            let py_handle = Py::new(
                py,
                ResourceRequestHandle {
                    state: Arc::clone(&handle_state),
                },
            )?;
            let decision = self
                .callback
                .bind(py)
                .call1((py_request, py_handle))?
                .extract::<u32>()?;
            Ok(match decision {
                sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE => handle_state
                    .finish_provider_decision(maplibre_core::ResourceProviderDecision::Handle),
                _ => handle_state
                    .finish_provider_decision(maplibre_core::ResourceProviderDecision::PassThrough),
            })
        })
        .unwrap_or_else(|_| handle_state.finish_provider_exception())
    }
}

unsafe extern "C" fn release_py_resource_provider_state(user_data: *mut c_void) {
    if !user_data.is_null() {
        // SAFETY: successful registration transfers this Box to native exactly once.
        drop(unsafe { Box::from_raw(user_data.cast::<PyResourceProviderState>()) });
    }
}

unsafe extern "C" fn resource_provider_trampoline(
    user_data: *mut c_void,
    request: *const sys::mln_resource_request,
    handle: sys::mln_resource_request_handle,
) -> u32 {
    catch_unwind(AssertUnwindSafe(|| {
        let Some(state) = ptr::NonNull::new(user_data.cast::<PyResourceProviderState>()) else {
            return maplibre_core::resource::UNKNOWN_PROVIDER_DECISION;
        };
        // SAFETY: user_data points to PyResourceProviderState retained by RuntimeHandle
        // until replacement, clear, or runtime teardown; native waits for in-flight callbacks.
        unsafe { state.as_ref() }.invoke(request, handle)
    }))
    .unwrap_or(maplibre_core::resource::UNKNOWN_PROVIDER_DECISION)
}

impl PyResourceTransformState {
    fn new(callback: Py<PyAny>, max_pending_callbacks: usize) -> Self {
        Self {
            callback,
            pending_callbacks: AtomicUsize::new(0),
            max_pending_callbacks,
        }
    }

    fn descriptor(&self) -> sys::mln_resource_transform {
        let mut descriptor = maplibre_core::resource::resource_transform_descriptor(
            Some(resource_transform_trampoline),
            ptr::from_ref(self).cast_mut().cast::<c_void>(),
        );
        descriptor.release_user_data = Some(release_py_resource_transform_state);
        descriptor
    }

    fn invoke(
        &self,
        raw_kind: u32,
        url: *const c_char,
        out_response: *mut sys::mln_resource_transform_response,
    ) -> sys::mln_status {
        // SAFETY: out_response is callback-duration output storage provided by native.
        let status = unsafe {
            maplibre_core::resource::initialize_resource_transform_response(out_response)
        };
        if status != sys::MLN_STATUS_OK {
            return status;
        }
        let Some(_permit) =
            try_acquire_callback_permit(&self.pending_callbacks, self.max_pending_callbacks)
        else {
            return sys::MLN_STATUS_OK;
        };
        // SAFETY: url is borrowed for callback duration by native.
        let request = match unsafe {
            maplibre_core::resource::copy_resource_transform_request(raw_kind, url)
        } {
            Ok(request) => request,
            Err(error) => return maplibre_core::resource::status_for_error(&error),
        };
        let replacement = Python::attach(|py| -> PyResult<Option<String>> {
            let py_request = resource_transform_request_to_py(py, request)?;
            self.callback
                .bind(py)
                .call1((py_request,))?
                .extract::<Option<String>>()
        });
        let Ok(Some(replacement)) = replacement else {
            return sys::MLN_STATUS_OK;
        };
        if replacement.is_empty() {
            return sys::MLN_STATUS_OK;
        }
        // SAFETY: out_response was initialized above and is non-null. The helper
        // copies the string into C API-managed scratch storage that stays live
        // after this trampoline returns.
        unsafe {
            sys::mln_resource_transform_response_set_url(
                out_response,
                replacement.as_ptr().cast(),
                replacement.len(),
            )
        }
    }
}

unsafe extern "C" fn release_py_resource_transform_state(user_data: *mut c_void) {
    if !user_data.is_null() {
        // SAFETY: successful registration transfers this Box to native exactly once.
        drop(unsafe { Box::from_raw(user_data.cast::<PyResourceTransformState>()) });
    }
}

unsafe extern "C" fn resource_transform_trampoline(
    user_data: *mut c_void,
    kind: u32,
    url: *const c_char,
    out_response: *mut sys::mln_resource_transform_response,
) -> sys::mln_status {
    catch_unwind(AssertUnwindSafe(|| {
        let Some(state) = ptr::NonNull::new(user_data.cast::<PyResourceTransformState>()) else {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        };
        // SAFETY: user_data points to PyResourceTransformState retained by RuntimeHandle
        // until replacement, clear, or runtime teardown; native waits for in-flight callbacks.
        unsafe { state.as_ref() }.invoke(kind, url, out_response)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

impl PyHttpHeaderTransformState {
    fn new(callback: Py<PyAny>, max_pending_callbacks: usize) -> Self {
        Self {
            callback,
            pending_callbacks: AtomicUsize::new(0),
            max_pending_callbacks,
        }
    }

    fn descriptor(&self) -> sys::mln_http_header_transform {
        let mut descriptor = maplibre_core::resource::http_header_transform_descriptor(
            Some(http_header_transform_trampoline),
            ptr::from_ref(self).cast_mut().cast::<c_void>(),
        );
        descriptor.release_user_data = Some(release_py_http_header_transform_state);
        descriptor
    }

    fn invoke(
        &self,
        raw_kind: u32,
        url: *const c_char,
        out_response: *mut sys::mln_http_header_transform_response,
    ) -> sys::mln_status {
        // SAFETY: native supplies callback-duration writable response storage.
        let status = unsafe {
            maplibre_core::resource::initialize_http_header_transform_response(out_response)
        };
        if status != sys::MLN_STATUS_OK {
            return status;
        }
        let Some(_permit) =
            try_acquire_callback_permit(&self.pending_callbacks, self.max_pending_callbacks)
        else {
            return sys::MLN_STATUS_OK;
        };
        // SAFETY: url is borrowed for the callback duration.
        let request = match unsafe {
            maplibre_core::resource::copy_http_header_transform_request(raw_kind, url)
        } {
            Ok(request) => request,
            Err(error) => return maplibre_core::resource::status_for_error(&error),
        };
        let headers = Python::attach(|py| -> PyResult<Vec<(String, String)>> {
            let py_request = http_header_transform_request_to_py(py, request)?;
            let returned = self.callback.bind(py).call1((py_request,))?;
            let mut headers = Vec::new();
            for item in returned.try_iter()? {
                let item = item?;
                headers.push((
                    item.get_item("name")?.extract()?,
                    item.get_item("value")?.extract()?,
                ));
            }
            Ok(headers)
        });
        let Ok(headers) = headers else {
            return sys::MLN_STATUS_NATIVE_ERROR;
        };
        let mut names = Vec::<String>::with_capacity(headers.len());
        for (name, value) in headers {
            if names
                .iter()
                .any(|existing| existing.eq_ignore_ascii_case(&name))
            {
                return sys::MLN_STATUS_INVALID_ARGUMENT;
            }
            names.push(name.clone());
            // SAFETY: native copies both strings during the call.
            let status = unsafe {
                sys::mln_http_header_transform_response_set(
                    out_response,
                    name.as_ptr().cast(),
                    name.len(),
                    value.as_ptr().cast(),
                    value.len(),
                )
            };
            if status != sys::MLN_STATUS_OK {
                return status;
            }
        }
        sys::MLN_STATUS_OK
    }
}

unsafe extern "C" fn release_py_http_header_transform_state(user_data: *mut c_void) {
    if !user_data.is_null() {
        // SAFETY: successful registration transfers this Box to native exactly once.
        drop(unsafe { Box::from_raw(user_data.cast::<PyHttpHeaderTransformState>()) });
    }
}

unsafe extern "C" fn http_header_transform_trampoline(
    user_data: *mut c_void,
    kind: u32,
    url: *const c_char,
    out_response: *mut sys::mln_http_header_transform_response,
) -> sys::mln_status {
    catch_unwind(AssertUnwindSafe(|| {
        let Some(state) = ptr::NonNull::new(user_data.cast::<PyHttpHeaderTransformState>()) else {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        };
        // SAFETY: RuntimeHandle retains state until native retires it.
        unsafe { state.as_ref() }.invoke(kind, url, out_response)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

fn event_batch_to_py(
    py: Python<'_>,
    events: Vec<maplibre_core::CopiedRuntimeEvent>,
) -> PyResult<Py<PyAny>> {
    let list = PyList::empty(py);
    for event in events {
        list.append(event_to_py(py, event)?)?;
    }
    let dict = PyDict::new(py);
    dict.set_item("events", list)?;
    Ok(dict.into_any().unbind())
}

fn event_to_py(py: Python<'_>, event: maplibre_core::CopiedRuntimeEvent) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("event_type", event_type_raw(event.event_type))?;
    dict.set_item("source_type", event.source.source_type)?;
    dict.set_item("source_id", event.source.source_id)?;
    dict.set_item("code", event.code)?;
    dict.set_item("message", event.message)?;
    dict.set_item("payload", payload_to_py(py, event.payload)?)?;
    Ok(dict.into_any().unbind())
}

fn payload_to_py(py: Python<'_>, payload: RuntimeEventPayload) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    match payload {
        RuntimeEventPayload::None => {
            dict.set_item("kind", "none")?;
        }
        RuntimeEventPayload::RenderFrame(payload) => {
            dict.set_item("kind", "render_frame")?;
            dict.set_item("mode", render_mode_raw(payload.mode))?;
            dict.set_item("needs_repaint", payload.needs_repaint)?;
            dict.set_item("placement_changed", payload.placement_changed)?;
            dict.set_item("stats", rendering_stats_to_py(py, &payload.stats)?)?;
        }
        RuntimeEventPayload::RenderMap(payload) => {
            dict.set_item("kind", "render_map")?;
            dict.set_item("mode", render_mode_raw(payload.mode))?;
        }
        RuntimeEventPayload::TileAction(payload) => {
            dict.set_item("kind", "tile_action")?;
            dict.set_item("operation", tile_operation_raw(payload.operation))?;
            dict.set_item("tile_id", tile_id_to_py(py, &payload.tile_id)?)?;
        }
        RuntimeEventPayload::OfflineRegionStatus(payload) => {
            dict.set_item("kind", "offline_region_status")?;
            dict.set_item("region_id", payload.region_id)?;
            dict.set_item(
                "status",
                copied_offline_region_status_to_py(py, &payload.status)?,
            )?;
        }
        RuntimeEventPayload::OfflineRegionResponseError(payload) => {
            dict.set_item("kind", "offline_region_response_error")?;
            dict.set_item("region_id", payload.region_id)?;
            dict.set_item("reason", payload.reason.raw_value())?;
        }
        RuntimeEventPayload::OfflineRegionTileCountLimit(payload) => {
            dict.set_item("kind", "offline_region_tile_count_limit")?;
            dict.set_item("region_id", payload.region_id)?;
            dict.set_item("limit", payload.limit)?;
        }
        RuntimeEventPayload::CameraTransitionFinished(payload) => {
            dict.set_item("kind", "camera_transition_finished")?;
            dict.set_item("transition_id", payload.transition_id)?;
        }
        RuntimeEventPayload::CommandFinished(payload) => {
            dict.set_item("kind", "command_finished")?;
            dict.set_item("command_id", payload.command_id)?;
            let disposition = match payload.disposition {
                maplibre_core::events::CommandDisposition::Committed => {
                    sys::MLN_COMMAND_DISPOSITION_COMMITTED
                }
                maplibre_core::events::CommandDisposition::Superseded => {
                    sys::MLN_COMMAND_DISPOSITION_SUPERSEDED
                }
                maplibre_core::events::CommandDisposition::Failed => {
                    sys::MLN_COMMAND_DISPOSITION_FAILED
                }
                maplibre_core::events::CommandDisposition::Cancelled => {
                    sys::MLN_COMMAND_DISPOSITION_CANCELLED
                }
                maplibre_core::events::CommandDisposition::Unknown(value) => value,
            };
            dict.set_item("disposition", disposition)?;
            dict.set_item("generation", payload.generation)?;
        }
        RuntimeEventPayload::Unknown(payload) => {
            dict.set_item("kind", "unknown")?;
            dict.set_item("raw_type", payload.raw_type)?;
            dict.set_item("bytes", PyBytes::new(py, &payload.bytes))?;
        }
        _ => {
            dict.set_item("kind", "unknown")?;
        }
    }
    Ok(dict.into_any().unbind())
}

fn rendering_stats_to_py(
    py: Python<'_>,
    stats: &maplibre_core::RenderingStats,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("encoding_time", stats.encoding_time)?;
    dict.set_item("rendering_time", stats.rendering_time)?;
    dict.set_item("frame_count", stats.frame_count)?;
    dict.set_item("draw_call_count", stats.draw_call_count)?;
    dict.set_item("total_draw_call_count", stats.total_draw_call_count)?;
    Ok(dict.into_any().unbind())
}

fn tile_id_to_py(py: Python<'_>, tile_id: &maplibre_core::TileId) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("overscaled_z", tile_id.overscaled_z)?;
    dict.set_item("wrap", tile_id.wrap)?;
    dict.set_item("canonical_z", tile_id.canonical_z)?;
    dict.set_item("canonical_x", tile_id.canonical_x)?;
    dict.set_item("canonical_y", tile_id.canonical_y)?;
    Ok(dict.into_any().unbind())
}

fn copied_offline_region_status_to_py(
    py: Python<'_>,
    status: &maplibre_core::OfflineRegionStatus,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item(
        "download_state",
        offline_region_download_state_raw(status.download_state),
    )?;
    dict.set_item("completed_resource_count", status.completed_resource_count)?;
    dict.set_item("completed_resource_size", status.completed_resource_size)?;
    dict.set_item("completed_tile_count", status.completed_tile_count)?;
    dict.set_item("required_tile_count", status.required_tile_count)?;
    dict.set_item("completed_tile_size", status.completed_tile_size)?;
    dict.set_item("required_resource_count", status.required_resource_count)?;
    dict.set_item(
        "required_resource_count_is_precise",
        status.required_resource_count_is_precise,
    )?;
    dict.set_item("complete", status.complete)?;
    Ok(dict.into_any().unbind())
}

fn render_mode_raw(mode: RenderMode) -> u32 {
    match mode {
        RenderMode::Partial => sys::MLN_RENDER_MODE_PARTIAL,
        RenderMode::Full => sys::MLN_RENDER_MODE_FULL,
        RenderMode::Unknown(raw) => raw,
        _ => 0,
    }
}

fn tile_operation_raw(operation: TileOperation) -> u32 {
    match operation {
        TileOperation::RequestedFromCache => sys::MLN_TILE_OPERATION_REQUESTED_FROM_CACHE,
        TileOperation::RequestedFromNetwork => sys::MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK,
        TileOperation::LoadFromNetwork => sys::MLN_TILE_OPERATION_LOAD_FROM_NETWORK,
        TileOperation::LoadFromCache => sys::MLN_TILE_OPERATION_LOAD_FROM_CACHE,
        TileOperation::StartParse => sys::MLN_TILE_OPERATION_START_PARSE,
        TileOperation::EndParse => sys::MLN_TILE_OPERATION_END_PARSE,
        TileOperation::Error => sys::MLN_TILE_OPERATION_ERROR,
        TileOperation::Cancelled => sys::MLN_TILE_OPERATION_CANCELLED,
        TileOperation::Null => sys::MLN_TILE_OPERATION_NULL,
        TileOperation::Unknown(raw) => raw,
        _ => 0,
    }
}

fn offline_region_download_state_raw(state: maplibre_core::OfflineRegionDownloadState) -> u32 {
    match state {
        maplibre_core::OfflineRegionDownloadState::Inactive => {
            sys::MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE
        }
        maplibre_core::OfflineRegionDownloadState::Active => {
            sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE
        }
        maplibre_core::OfflineRegionDownloadState::Unknown(raw) => raw,
        _ => 0,
    }
}

fn event_type_raw(event_type: RuntimeEventType) -> u32 {
    match event_type {
        RuntimeEventType::MapCameraWillChange => sys::MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE,
        RuntimeEventType::MapCameraIsChanging => sys::MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING,
        RuntimeEventType::MapCameraDidChange => sys::MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE,
        RuntimeEventType::MapCameraTransitionFinished => {
            sys::MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED
        }
        RuntimeEventType::MapStyleLoaded => sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED,
        RuntimeEventType::MapLoadingStarted => sys::MLN_RUNTIME_EVENT_MAP_LOADING_STARTED,
        RuntimeEventType::MapLoadingFinished => sys::MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED,
        RuntimeEventType::MapLoadingFailed => sys::MLN_RUNTIME_EVENT_MAP_LOADING_FAILED,
        RuntimeEventType::MapIdle => sys::MLN_RUNTIME_EVENT_MAP_IDLE,
        RuntimeEventType::MapRenderUpdateAvailable => {
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
        }
        RuntimeEventType::MapRenderError => sys::MLN_RUNTIME_EVENT_MAP_RENDER_ERROR,
        RuntimeEventType::MapStillImageFinished => sys::MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED,
        RuntimeEventType::MapStillImageFailed => sys::MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED,
        RuntimeEventType::MapRenderFrameStarted => sys::MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED,
        RuntimeEventType::MapRenderFrameFinished => {
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED
        }
        RuntimeEventType::MapRenderMapStarted => sys::MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED,
        RuntimeEventType::MapRenderMapFinished => sys::MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED,
        RuntimeEventType::MapStyleImageMissing => sys::MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING,
        RuntimeEventType::MapTileAction => sys::MLN_RUNTIME_EVENT_MAP_TILE_ACTION,
        RuntimeEventType::OfflineRegionStatusChanged => {
            sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED
        }
        RuntimeEventType::OfflineRegionResponseError => {
            sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR
        }
        RuntimeEventType::OfflineRegionTileCountLimitExceeded => {
            sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED
        }
        RuntimeEventType::Unknown(raw) => raw,
        _ => 0,
    }
}

fn invalid_argument_error(diagnostic: impl Into<String>) -> PyErr {
    py_errors::InvalidArgumentError::new_err((Option::<i32>::None, diagnostic.into()))
}

fn invalid_state_error(diagnostic: impl Into<String>) -> PyErr {
    py_errors::InvalidStateError::new_err((Option::<i32>::None, diagnostic.into()))
}

fn texture_image_info_to_py(
    py: Python<'_>,
    info: maplibre_core::TextureImageInfo,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("width", info.width)?;
    dict.set_item("height", info.height)?;
    dict.set_item("stride", info.stride)?;
    dict.set_item("byte_length", info.byte_length)?;
    Ok(dict.into_any().unbind())
}

fn image_to_py(
    py: Python<'_>,
    info: maplibre_core::TextureImageInfo,
    data: &[u8],
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("info", texture_image_info_to_py(py, info)?)?;
    dict.set_item("data", PyBytes::new(py, data))?;
    Ok(dict.into_any().unbind())
}

fn log_record_to_py(py: Python<'_>, record: CopiedLogRecordRaw) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("severity", record.severity)?;
    dict.set_item("event", record.event)?;
    dict.set_item("code", record.code)?;
    dict.set_item("message", record.message)?;
    Ok(dict.into_any().unbind())
}

fn log_severity_raw(severity: LogSeverity) -> u32 {
    match severity {
        LogSeverity::Info => sys::MLN_LOG_SEVERITY_INFO,
        LogSeverity::Warning => sys::MLN_LOG_SEVERITY_WARNING,
        LogSeverity::Error => sys::MLN_LOG_SEVERITY_ERROR,
        LogSeverity::Unknown(raw) => raw,
        _ => 0,
    }
}

fn log_event_raw(event: LogEvent) -> u32 {
    match event {
        LogEvent::General => sys::MLN_LOG_EVENT_GENERAL,
        LogEvent::Setup => sys::MLN_LOG_EVENT_SETUP,
        LogEvent::Shader => sys::MLN_LOG_EVENT_SHADER,
        LogEvent::ParseStyle => sys::MLN_LOG_EVENT_PARSE_STYLE,
        LogEvent::ParseTile => sys::MLN_LOG_EVENT_PARSE_TILE,
        LogEvent::Render => sys::MLN_LOG_EVENT_RENDER,
        LogEvent::Style => sys::MLN_LOG_EVENT_STYLE,
        LogEvent::Database => sys::MLN_LOG_EVENT_DATABASE,
        LogEvent::HttpRequest => sys::MLN_LOG_EVENT_HTTP_REQUEST,
        LogEvent::Sprite => sys::MLN_LOG_EVENT_SPRITE,
        LogEvent::Image => sys::MLN_LOG_EVENT_IMAGE,
        LogEvent::OpenGl => sys::MLN_LOG_EVENT_OPENGL,
        LogEvent::Jni => sys::MLN_LOG_EVENT_JNI,
        LogEvent::Android => sys::MLN_LOG_EVENT_ANDROID,
        LogEvent::Crash => sys::MLN_LOG_EVENT_CRASH,
        LogEvent::Glyph => sys::MLN_LOG_EVENT_GLYPH,
        LogEvent::Timing => sys::MLN_LOG_EVENT_TIMING,
        LogEvent::Unknown(raw) => raw,
        _ => 0,
    }
}

fn optional_ref_ptr<T>(value: Option<&T>) -> *const T {
    value.map_or(ptr::null(), |value| value as *const T)
}

fn screen_point_from_tuple((x, y): (f64, f64)) -> sys::mln_screen_point {
    sys::mln_screen_point { x, y }
}

fn edge_insets_from_tuple(
    (top, left, bottom, right): (f64, f64, f64, f64),
) -> sys::mln_edge_insets {
    sys::mln_edge_insets {
        top,
        left,
        bottom,
        right,
    }
}

fn edge_insets_core_from_tuple(
    (top, left, bottom, right): (f64, f64, f64, f64),
) -> maplibre_core::EdgeInsets {
    maplibre_core::EdgeInsets::new(top, left, bottom, right)
}

fn lat_lngs_from_tuples(coordinates: Vec<(f64, f64)>) -> Vec<sys::mln_lat_lng> {
    coordinates
        .into_iter()
        .map(|(latitude, longitude)| sys::mln_lat_lng {
            latitude,
            longitude,
        })
        .collect()
}

fn lat_lng_bounds_from_tuple(
    (southwest, northeast): ((f64, f64), (f64, f64)),
) -> sys::mln_lat_lng_bounds {
    sys::mln_lat_lng_bounds {
        southwest: sys::mln_lat_lng {
            latitude: southwest.0,
            longitude: southwest.1,
        },
        northeast: sys::mln_lat_lng {
            latitude: northeast.0,
            longitude: northeast.1,
        },
    }
}

fn lat_lng_bounds_core_from_tuple(
    (southwest, northeast): ((f64, f64), (f64, f64)),
) -> maplibre_core::LatLngBounds {
    maplibre_core::LatLngBounds::new(
        maplibre_core::LatLng::new(southwest.0, southwest.1),
        maplibre_core::LatLng::new(northeast.0, northeast.1),
    )
}

fn empty_lat_lng_bounds() -> sys::mln_lat_lng_bounds {
    sys::mln_lat_lng_bounds {
        southwest: sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        },
        northeast: sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        },
    }
}

fn lat_lng_to_py(py: Python<'_>, coordinate: sys::mln_lat_lng) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("latitude", coordinate.latitude)?;
    dict.set_item("longitude", coordinate.longitude)?;
    Ok(dict.into_any().unbind())
}

fn lat_lng_bounds_to_py(py: Python<'_>, bounds: sys::mln_lat_lng_bounds) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("southwest", lat_lng_to_py(py, bounds.southwest)?)?;
    dict.set_item("northeast", lat_lng_to_py(py, bounds.northeast)?)?;
    Ok(dict.into_any().unbind())
}

fn lat_lng_list_to_py(py: Python<'_>, coordinates: &[sys::mln_lat_lng]) -> PyResult<Py<PyAny>> {
    let list = PyList::empty(py);
    for coordinate in coordinates {
        list.append(lat_lng_to_py(py, *coordinate)?)?;
    }
    Ok(list.into_any().unbind())
}

fn screen_point_to_py(py: Python<'_>, point: sys::mln_screen_point) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("x", point.x)?;
    dict.set_item("y", point.y)?;
    Ok(dict.into_any().unbind())
}

fn screen_point_list_to_py(
    py: Python<'_>,
    points: &[sys::mln_screen_point],
) -> PyResult<Py<PyAny>> {
    let list = PyList::empty(py);
    for point in points {
        list.append(screen_point_to_py(py, *point)?)?;
    }
    Ok(list.into_any().unbind())
}

#[allow(clippy::too_many_arguments)]
fn tile_source_options_from_parts(
    min_zoom: Option<f64>,
    max_zoom: Option<f64>,
    attribution: Option<String>,
    scheme: Option<u32>,
    bounds: Option<((f64, f64), (f64, f64))>,
    tile_size: Option<u32>,
    vector_encoding: Option<u32>,
    raster_dem_encoding: Option<u32>,
) -> PyResult<maplibre_core::TileSourceOptions> {
    let mut options = maplibre_core::TileSourceOptions::default();
    if let Some(min_zoom) = min_zoom {
        options.min_zoom = Some(min_zoom);
    }
    if let Some(max_zoom) = max_zoom {
        options.max_zoom = Some(max_zoom);
    }
    if let Some(attribution) = attribution {
        options.attribution = Some(attribution);
    }
    if let Some(scheme) = scheme {
        options.scheme = Some(maplibre_core::TileScheme::from_raw(scheme));
    }
    if let Some((
        (southwest_latitude, southwest_longitude),
        (northeast_latitude, northeast_longitude),
    )) = bounds
    {
        options.bounds = Some(maplibre_core::LatLngBounds::new(
            maplibre_core::LatLng::new(southwest_latitude, southwest_longitude),
            maplibre_core::LatLng::new(northeast_latitude, northeast_longitude),
        ));
    }
    if let Some(tile_size) = tile_size {
        options.tile_size = Some(tile_size);
    }
    if let Some(vector_encoding) = vector_encoding {
        options.vector_encoding =
            Some(maplibre_core::VectorTileEncoding::from_raw(vector_encoding));
    }
    if let Some(raster_dem_encoding) = raster_dem_encoding {
        options.raster_dem_encoding = Some(maplibre_core::RasterDemEncoding::from_raw(
            raster_dem_encoding,
        ));
    }
    Ok(options)
}

#[allow(clippy::too_many_arguments)]
fn geojson_source_options_from_parts(
    min_zoom: Option<f64>,
    max_zoom: Option<f64>,
    tolerance: Option<f64>,
    cluster_max_zoom: Option<f64>,
    cluster_properties: Option<Bound<'_, PyBytes>>,
    tile_size: Option<u32>,
    buffer: Option<u32>,
    cluster_radius: Option<u32>,
    cluster_min_points: Option<u32>,
    line_metrics: Option<bool>,
    cluster: Option<bool>,
    synchronous_tiling: Option<bool>,
) -> PyResult<maplibre_core::GeoJsonSourceOptions> {
    let mut options = maplibre_core::GeoJsonSourceOptions::default();
    if let Some(min_zoom) = min_zoom {
        options.min_zoom = Some(min_zoom);
    }
    if let Some(max_zoom) = max_zoom {
        options.max_zoom = Some(max_zoom);
    }
    if let Some(tolerance) = tolerance {
        options.tolerance = Some(tolerance);
    }
    if let Some(cluster_max_zoom) = cluster_max_zoom {
        options.cluster_max_zoom = Some(cluster_max_zoom);
    }
    if let Some(cluster_properties) = cluster_properties {
        options.cluster_properties = Some(cluster_properties.as_bytes().to_vec());
    }
    if let Some(tile_size) = tile_size {
        options.tile_size = Some(tile_size);
    }
    if let Some(buffer) = buffer {
        options.buffer = Some(buffer);
    }
    if let Some(cluster_radius) = cluster_radius {
        options.cluster_radius = Some(cluster_radius);
    }
    if let Some(cluster_min_points) = cluster_min_points {
        options.cluster_min_points = Some(cluster_min_points);
    }
    if let Some(line_metrics) = line_metrics {
        options.line_metrics = Some(line_metrics);
    }
    if let Some(cluster) = cluster {
        options.cluster = Some(cluster);
    }
    if let Some(synchronous_tiling) = synchronous_tiling {
        options.synchronous_tiling = Some(synchronous_tiling);
    }
    Ok(options)
}

/// Prepares one GeoJSON document into an owned prepared-data handle.
///
/// Parsing and tiling run without the GIL, so worker threads preparing
/// documents genuinely parallelize.
#[pyfunction]
#[allow(clippy::too_many_arguments)]
fn create_geojson_source_data(
    py: Python<'_>,
    data: &Bound<'_, PyBytes>,
    min_zoom: Option<f64>,
    max_zoom: Option<f64>,
    tolerance: Option<f64>,
    cluster_max_zoom: Option<f64>,
    cluster_properties: Option<Bound<'_, PyBytes>>,
    tile_size: Option<u32>,
    buffer: Option<u32>,
    cluster_radius: Option<u32>,
    cluster_min_points: Option<u32>,
    line_metrics: Option<bool>,
    cluster: Option<bool>,
    synchronous_tiling: Option<bool>,
) -> PyResult<GeoJsonSourceDataHandle> {
    let options = geojson_source_options_from_parts(
        min_zoom,
        max_zoom,
        tolerance,
        cluster_max_zoom,
        cluster_properties,
        tile_size,
        buffer,
        cluster_radius,
        cluster_min_points,
        line_metrics,
        cluster,
        synchronous_tiling,
    )?;
    let data = data.as_bytes().to_vec();
    let native = py
        .detach(move || -> Result<sys::mln_geojson_source_data, Error> {
            let options = maplibre_core::style::geojson_source_options_to_native(&options)?;
            let data = maplibre_core::string::buffer_view(&data);
            let mut out = maplibre_core::ptr::OutHandle::<sys::mln_geojson_source_data>::new();
            // SAFETY: data and options are materialized native input owned by
            // this closure, and out is a null-initialized out-pointer owned by
            // this call. The C API accepts this call from any thread.
            maplibre_core::check(unsafe {
                sys::mln_geojson_source_data_create(data, options.as_ptr(), out.as_mut_ptr())
            })?;
            out.into_live("mln_geojson_source_data")
        })
        .map_err(map_error)?;
    // SAFETY: native came from successful mln_geojson_source_data_create and is
    // paired with the matching destroy function in GeoJsonSourceDataHandle.close.
    let state = unsafe {
        maplibre_core::handle::NativeHandleState::from_handle(native, "mln_geojson_source_data")
    }
    .map_err(map_error)?;
    Ok(GeoJsonSourceDataHandle {
        state: Mutex::new(state),
    })
}

fn viewport_options_from_parts(
    north_orientation: Option<u32>,
    constrain_mode: Option<u32>,
    viewport_mode: Option<u32>,
    frustum_offset: Option<(f64, f64, f64, f64)>,
) -> sys::mln_map_viewport_options {
    let mut options = maplibre_core::MapViewportOptions::default();
    options.north_orientation = north_orientation.map(maplibre_core::NorthOrientation::from_raw);
    options.constrain_mode = constrain_mode.map(maplibre_core::ConstrainMode::from_raw);
    options.viewport_mode = viewport_mode.map(maplibre_core::ViewportMode::from_raw);
    options.frustum_offset = frustum_offset.map(edge_insets_core_from_tuple);
    maplibre_core::options::map_viewport_options_to_native(&options)
}

#[allow(clippy::too_many_arguments)]
fn tile_options_from_parts(
    prefetch_zoom_delta: Option<u32>,
    lod_min_radius: Option<f64>,
    lod_scale: Option<f64>,
    lod_pitch_threshold: Option<f64>,
    lod_zoom_shift: Option<f64>,
    lod_mode: Option<u32>,
) -> sys::mln_map_tile_options {
    let mut options = maplibre_core::MapTileOptions::default();
    options.prefetch_zoom_delta = prefetch_zoom_delta;
    options.lod_min_radius = lod_min_radius;
    options.lod_scale = lod_scale;
    options.lod_pitch_threshold = lod_pitch_threshold;
    options.lod_zoom_shift = lod_zoom_shift;
    options.lod_mode = lod_mode.map(maplibre_core::TileLodMode::from_raw);
    maplibre_core::options::map_tile_options_to_native(&options)
}

fn free_camera_options_from_parts(
    position: Option<(f64, f64, f64)>,
    orientation: Option<(f64, f64, f64, f64)>,
) -> sys::mln_free_camera_options {
    let mut options = maplibre_core::FreeCameraOptions::default();
    options.position = position.map(|(x, y, z)| maplibre_core::Vec3::new(x, y, z));
    options.orientation =
        orientation.map(|(x, y, z, w)| maplibre_core::Quaternion::new(x, y, z, w));
    maplibre_core::camera::free_camera_options_to_native(&options)
}

fn projection_mode_from_parts(
    axonometric: Option<bool>,
    x_skew: Option<f64>,
    y_skew: Option<f64>,
) -> sys::mln_projection_mode {
    let mut mode = maplibre_core::ProjectionMode::default();
    mode.axonometric = axonometric;
    mode.x_skew = x_skew;
    mode.y_skew = y_skew;
    maplibre_core::camera::projection_mode_to_native(&mode)
}

fn animation_options_from_parts(
    (duration_ms, velocity, min_zoom, easing, transition_id): AnimationParts,
) -> sys::mln_animation_options {
    let mut options = maplibre_core::AnimationOptions::default();
    options.duration_ms = duration_ms;
    options.velocity = velocity;
    options.min_zoom = min_zoom;
    options.easing = easing.map(|(x1, y1, x2, y2)| maplibre_core::UnitBezier::new(x1, y1, x2, y2));
    options.transition_id = transition_id;
    maplibre_core::camera::animation_options_to_native(&options)
}

fn camera_options_from_parts(
    center: Option<(f64, f64)>,
    zoom: Option<f64>,
    bearing: Option<f64>,
    pitch: Option<f64>,
    center_altitude: Option<f64>,
    padding: Option<(f64, f64, f64, f64)>,
    anchor: Option<(f64, f64)>,
    roll: Option<f64>,
    field_of_view: Option<f64>,
) -> sys::mln_camera_options {
    let mut camera = maplibre_core::CameraOptions::default();
    camera.center =
        center.map(|(latitude, longitude)| maplibre_core::LatLng::new(latitude, longitude));
    camera.zoom = zoom;
    camera.bearing = bearing;
    camera.pitch = pitch;
    camera.center_altitude = center_altitude;
    camera.padding = padding.map(edge_insets_core_from_tuple);
    camera.anchor = anchor.map(|(x, y)| maplibre_core::ScreenPoint::new(x, y));
    camera.roll = roll;
    camera.field_of_view = field_of_view;
    maplibre_core::camera::camera_options_to_native(&camera)
}

fn camera_fit_options_from_parts(
    padding: Option<(f64, f64, f64, f64)>,
    bearing: Option<f64>,
    pitch: Option<f64>,
) -> sys::mln_camera_fit_options {
    let mut fit = maplibre_core::CameraFitOptions::default();
    fit.padding = padding.map(edge_insets_core_from_tuple);
    fit.bearing = bearing;
    fit.pitch = pitch;
    maplibre_core::camera::camera_fit_options_to_native(&fit)
}

fn bound_options_from_parts(
    bounds: Option<((f64, f64), (f64, f64))>,
    unbounded: bool,
    min_zoom: Option<f64>,
    max_zoom: Option<f64>,
    min_pitch: Option<f64>,
    max_pitch: Option<f64>,
) -> sys::mln_bound_options {
    let mut options = maplibre_core::BoundOptions::default();
    options.bounds = match bounds {
        Some(bounds) => Some(maplibre_core::BoundsConstraint::Bounded(
            lat_lng_bounds_core_from_tuple(bounds),
        )),
        None if unbounded => Some(maplibre_core::BoundsConstraint::Unbounded),
        None => None,
    };
    options.min_zoom = min_zoom;
    options.max_zoom = max_zoom;
    options.min_pitch = min_pitch;
    options.max_pitch = max_pitch;
    maplibre_core::camera::bound_options_to_native(&options)
}

fn edge_insets_core_to_py(
    py: Python<'_>,
    insets: maplibre_core::EdgeInsets,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("top", insets.top)?;
    dict.set_item("left", insets.left)?;
    dict.set_item("bottom", insets.bottom)?;
    dict.set_item("right", insets.right)?;
    Ok(dict.into_any().unbind())
}

fn screen_point_core_to_py(
    py: Python<'_>,
    point: maplibre_core::ScreenPoint,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("x", point.x)?;
    dict.set_item("y", point.y)?;
    Ok(dict.into_any().unbind())
}

fn vec3_core_to_py(py: Python<'_>, value: maplibre_core::Vec3) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("x", value.x)?;
    dict.set_item("y", value.y)?;
    dict.set_item("z", value.z)?;
    Ok(dict.into_any().unbind())
}

fn quaternion_core_to_py(py: Python<'_>, value: maplibre_core::Quaternion) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("x", value.x)?;
    dict.set_item("y", value.y)?;
    dict.set_item("z", value.z)?;
    dict.set_item("w", value.w)?;
    Ok(dict.into_any().unbind())
}

fn viewport_options_to_py(
    py: Python<'_>,
    options: &sys::mln_map_viewport_options,
) -> PyResult<Py<PyAny>> {
    let options = maplibre_core::options::map_viewport_options_from_native(*options);
    let dict = PyDict::new(py);
    dict.set_item(
        "north_orientation",
        options.north_orientation.map(|value| value.as_raw()),
    )?;
    dict.set_item(
        "constrain_mode",
        options.constrain_mode.map(|value| value.as_raw()),
    )?;
    dict.set_item(
        "viewport_mode",
        options.viewport_mode.map(|value| value.as_raw()),
    )?;
    if let Some(frustum_offset) = options.frustum_offset {
        dict.set_item(
            "frustum_offset",
            edge_insets_core_to_py(py, frustum_offset)?,
        )?;
    } else {
        dict.set_item("frustum_offset", py.None())?;
    }
    Ok(dict.into_any().unbind())
}

fn tile_options_to_py(py: Python<'_>, options: &sys::mln_map_tile_options) -> PyResult<Py<PyAny>> {
    let options = maplibre_core::options::map_tile_options_from_native(*options);
    let dict = PyDict::new(py);
    dict.set_item("prefetch_zoom_delta", options.prefetch_zoom_delta)?;
    dict.set_item("lod_min_radius", options.lod_min_radius)?;
    dict.set_item("lod_scale", options.lod_scale)?;
    dict.set_item("lod_pitch_threshold", options.lod_pitch_threshold)?;
    dict.set_item("lod_zoom_shift", options.lod_zoom_shift)?;
    dict.set_item("lod_mode", options.lod_mode.map(|value| value.as_raw()))?;
    Ok(dict.into_any().unbind())
}

fn free_camera_options_to_py(
    py: Python<'_>,
    options: &sys::mln_free_camera_options,
) -> PyResult<Py<PyAny>> {
    let options = maplibre_core::camera::free_camera_options_from_native(*options);
    let dict = PyDict::new(py);
    if let Some(position) = options.position {
        dict.set_item("position", vec3_core_to_py(py, position)?)?;
    } else {
        dict.set_item("position", py.None())?;
    }
    if let Some(orientation) = options.orientation {
        dict.set_item("orientation", quaternion_core_to_py(py, orientation)?)?;
    } else {
        dict.set_item("orientation", py.None())?;
    }
    Ok(dict.into_any().unbind())
}

fn bound_options_to_py(py: Python<'_>, options: &sys::mln_bound_options) -> PyResult<Py<PyAny>> {
    let options = maplibre_core::camera::bound_options_from_native(*options);
    let dict = PyDict::new(py);
    match options.bounds {
        Some(maplibre_core::BoundsConstraint::Bounded(bounds)) => {
            dict.set_item("bounds", lat_lng_bounds_core_to_py(py, &bounds)?)?;
            dict.set_item("unbounded", false)?;
        }
        Some(maplibre_core::BoundsConstraint::Unbounded) => {
            dict.set_item("bounds", py.None())?;
            dict.set_item("unbounded", true)?;
        }
        None => {
            dict.set_item("bounds", py.None())?;
            dict.set_item("unbounded", false)?;
        }
    }
    dict.set_item("min_zoom", options.min_zoom)?;
    dict.set_item("max_zoom", options.max_zoom)?;
    dict.set_item("min_pitch", options.min_pitch)?;
    dict.set_item("max_pitch", options.max_pitch)?;
    Ok(dict.into_any().unbind())
}

fn projection_mode_to_py(py: Python<'_>, mode: &sys::mln_projection_mode) -> PyResult<Py<PyAny>> {
    let mode = maplibre_core::camera::projection_mode_from_native(*mode);
    let dict = PyDict::new(py);
    dict.set_item("axonometric", mode.axonometric)?;
    dict.set_item("x_skew", mode.x_skew)?;
    dict.set_item("y_skew", mode.y_skew)?;
    Ok(dict.into_any().unbind())
}

fn camera_options_to_py(py: Python<'_>, camera: &sys::mln_camera_options) -> PyResult<Py<PyAny>> {
    let camera = maplibre_core::camera::camera_options_from_native(*camera);
    let dict = PyDict::new(py);
    if let Some(center) = camera.center {
        dict.set_item("center", lat_lng_core_to_py(py, &center)?)?;
    } else {
        dict.set_item("center", py.None())?;
    }
    dict.set_item("zoom", camera.zoom)?;
    dict.set_item("bearing", camera.bearing)?;
    dict.set_item("pitch", camera.pitch)?;
    dict.set_item("center_altitude", camera.center_altitude)?;
    if let Some(padding) = camera.padding {
        dict.set_item("padding", edge_insets_core_to_py(py, padding)?)?;
    } else {
        dict.set_item("padding", py.None())?;
    }
    if let Some(anchor) = camera.anchor {
        dict.set_item("anchor", screen_point_core_to_py(py, anchor)?)?;
    } else {
        dict.set_item("anchor", py.None())?;
    }
    dict.set_item("roll", camera.roll)?;
    dict.set_item("field_of_view", camera.field_of_view)?;
    Ok(dict.into_any().unbind())
}

fn custom_geometry_event_to_py(py: Python<'_>, event: CustomGeometryEvent) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("kind", event.kind)?;
    dict.set_item("z", event.tile_id.z)?;
    dict.set_item("x", event.tile_id.x)?;
    dict.set_item("y", event.tile_id.y)?;
    Ok(dict.into_any().unbind())
}

fn empty_offline_region_status() -> sys::mln_offline_region_status {
    sys::mln_offline_region_status {
        size: std::mem::size_of::<sys::mln_offline_region_status>() as u32,
        download_state: 0,
        completed_resource_count: 0,
        completed_resource_size: 0,
        completed_tile_count: 0,
        required_tile_count: 0,
        completed_tile_size: 0,
        required_resource_count: 0,
        required_resource_count_is_precise: false,
        complete: false,
    }
}

fn offline_region_status_to_py(
    py: Python<'_>,
    status: &sys::mln_offline_region_status,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("download_state", status.download_state)?;
    dict.set_item("completed_resource_count", status.completed_resource_count)?;
    dict.set_item("completed_resource_size", status.completed_resource_size)?;
    dict.set_item("completed_tile_count", status.completed_tile_count)?;
    dict.set_item("required_tile_count", status.required_tile_count)?;
    dict.set_item("completed_tile_size", status.completed_tile_size)?;
    dict.set_item("required_resource_count", status.required_resource_count)?;
    dict.set_item(
        "required_resource_count_is_precise",
        status.required_resource_count_is_precise,
    )?;
    dict.set_item("complete", status.complete)?;
    Ok(dict.into_any().unbind())
}

fn offline_region_list_to_py(
    py: Python<'_>,
    regions: &[maplibre_core::OfflineRegionInfo],
) -> PyResult<Py<PyAny>> {
    let list = PyList::empty(py);
    for region in regions {
        list.append(offline_region_info_to_py(py, region)?)?;
    }
    Ok(list.into_any().unbind())
}

fn offline_region_info_to_py(
    py: Python<'_>,
    info: &maplibre_core::OfflineRegionInfo,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("id", info.id)?;
    dict.set_item(
        "definition",
        offline_region_definition_to_py(py, &info.definition)?,
    )?;
    dict.set_item("metadata", PyBytes::new(py, &info.metadata))?;
    Ok(dict.into_any().unbind())
}

fn offline_region_definition_to_py(
    py: Python<'_>,
    definition: &maplibre_core::OfflineRegionDefinition,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    match definition {
        maplibre_core::OfflineRegionDefinition::TilePyramid {
            style_url,
            bounds,
            min_zoom,
            max_zoom,
            pixel_ratio,
            include_ideographs,
        } => {
            dict.set_item("type", "tile_pyramid")?;
            dict.set_item("style_url", style_url)?;
            dict.set_item("bounds", lat_lng_bounds_core_to_py(py, bounds)?)?;
            dict.set_item("min_zoom", *min_zoom)?;
            dict.set_item("max_zoom", *max_zoom)?;
            dict.set_item("pixel_ratio", *pixel_ratio)?;
            dict.set_item("include_ideographs", *include_ideographs)?;
        }
        maplibre_core::OfflineRegionDefinition::GeometryRegion {
            style_url,
            geometry,
            min_zoom,
            max_zoom,
            pixel_ratio,
            include_ideographs,
        } => {
            dict.set_item("type", "geometry")?;
            dict.set_item("style_url", style_url)?;
            dict.set_item("geometry", PyBytes::new(py, geometry))?;
            dict.set_item("min_zoom", *min_zoom)?;
            dict.set_item("max_zoom", *max_zoom)?;
            dict.set_item("pixel_ratio", *pixel_ratio)?;
            dict.set_item("include_ideographs", *include_ideographs)?;
        }
        _ => {
            return Err(invalid_argument_error(
                "unsupported offline region definition",
            ));
        }
    }
    Ok(dict.into_any().unbind())
}

fn offline_region_definition_from_wire(
    raw: &Bound<'_, PyAny>,
) -> PyResult<maplibre_core::OfflineRegionDefinition> {
    let type_name = raw.get_type().name()?;
    match type_name.to_str()? {
        "OfflineTilePyramidRegionDefinition" => {
            Ok(maplibre_core::OfflineRegionDefinition::TilePyramid {
                style_url: raw.getattr("style_url")?.extract()?,
                bounds: lat_lng_bounds_core_from_wire(&raw.getattr("bounds")?)?,
                min_zoom: raw.getattr("min_zoom")?.extract()?,
                max_zoom: raw.getattr("max_zoom")?.extract()?,
                pixel_ratio: raw.getattr("pixel_ratio")?.extract()?,
                include_ideographs: raw.getattr("include_ideographs")?.extract()?,
            })
        }
        "OfflineGeometryRegionDefinition" => {
            Ok(maplibre_core::OfflineRegionDefinition::GeometryRegion {
                style_url: raw.getattr("style_url")?.extract()?,
                geometry: raw
                    .getattr("geometry")?
                    .cast::<PyBytes>()?
                    .as_bytes()
                    .to_vec(),
                min_zoom: raw.getattr("min_zoom")?.extract()?,
                max_zoom: raw.getattr("max_zoom")?.extract()?,
                pixel_ratio: raw.getattr("pixel_ratio")?.extract()?,
                include_ideographs: raw.getattr("include_ideographs")?.extract()?,
            })
        }
        _ => offline_region_definition_wire_dict_from_py(raw),
    }
}

fn offline_region_definition_wire_dict_from_py(
    raw: &Bound<'_, PyAny>,
) -> PyResult<maplibre_core::OfflineRegionDefinition> {
    let dict = raw.cast::<PyDict>()?;
    let kind: String = required_dict_item(dict, "type")?.extract()?;
    let style_url: String = required_dict_item(dict, "style_url")?.extract()?;
    let min_zoom: f64 = required_dict_item(dict, "min_zoom")?.extract()?;
    let max_zoom: f64 = required_dict_item(dict, "max_zoom")?.extract()?;
    let pixel_ratio: f32 = required_dict_item(dict, "pixel_ratio")?.extract()?;
    let include_ideographs: bool = required_dict_item(dict, "include_ideographs")?.extract()?;
    match kind.as_str() {
        "tile_pyramid" => Ok(maplibre_core::OfflineRegionDefinition::TilePyramid {
            style_url,
            bounds: lat_lng_bounds_core_from_wire(&required_dict_item(dict, "bounds")?)?,
            min_zoom,
            max_zoom,
            pixel_ratio,
            include_ideographs,
        }),
        "geometry" => Ok(maplibre_core::OfflineRegionDefinition::GeometryRegion {
            style_url,
            geometry: required_dict_item(dict, "geometry")?
                .cast::<PyBytes>()?
                .as_bytes()
                .to_vec(),
            min_zoom,
            max_zoom,
            pixel_ratio,
            include_ideographs,
        }),
        _ => Err(invalid_argument_error(format!(
            "unsupported offline region definition: {kind}"
        ))),
    }
}

fn lat_lng_bounds_core_from_wire(raw: &Bound<'_, PyAny>) -> PyResult<maplibre_core::LatLngBounds> {
    if let Ok((southwest, northeast)) = raw.extract::<((f64, f64), (f64, f64))>() {
        return Ok(maplibre_core::LatLngBounds::new(
            maplibre_core::LatLng::new(southwest.0, southwest.1),
            maplibre_core::LatLng::new(northeast.0, northeast.1),
        ));
    }
    Ok(maplibre_core::LatLngBounds::new(
        lat_lng_from_wire(&raw.getattr("southwest")?)?,
        lat_lng_from_wire(&raw.getattr("northeast")?)?,
    ))
}

fn lat_lng_from_wire(raw: &Bound<'_, PyAny>) -> PyResult<maplibre_core::LatLng> {
    if let Ok((latitude, longitude)) = raw.extract::<(f64, f64)>() {
        return Ok(maplibre_core::LatLng::new(latitude, longitude));
    }
    Ok(maplibre_core::LatLng::new(
        raw.getattr("latitude")?.extract()?,
        raw.getattr("longitude")?.extract()?,
    ))
}

fn rendered_query_geometry_from_wire(
    raw: &Bound<'_, PyAny>,
) -> PyResult<maplibre_core::RenderedQueryGeometry> {
    let dict = raw.cast::<PyDict>()?;
    let kind: String = required_dict_item(dict, "type")?.extract()?;
    match kind.as_str() {
        "point" => Ok(maplibre_core::RenderedQueryGeometry::point(
            screen_point_core_from_wire(&required_dict_item(dict, "point")?)?,
        )),
        "box" => Ok(maplibre_core::RenderedQueryGeometry::box_(
            maplibre_core::ScreenBox::new(
                screen_point_core_from_wire(&required_dict_item(dict, "min")?)?,
                screen_point_core_from_wire(&required_dict_item(dict, "max")?)?,
            ),
        )),
        "line_string" => {
            let points = required_dict_item(dict, "points")?;
            let points = points.cast::<PyList>()?;
            let mut copied = Vec::with_capacity(points.len());
            for point in points.iter() {
                copied.push(screen_point_core_from_wire(&point)?);
            }
            Ok(maplibre_core::RenderedQueryGeometry::line_string(copied))
        }
        _ => Err(invalid_argument_error(format!(
            "unsupported rendered query geometry wire type: {kind}"
        ))),
    }
}

fn screen_point_core_from_wire(raw: &Bound<'_, PyAny>) -> PyResult<maplibre_core::ScreenPoint> {
    let (x, y): (f64, f64) = raw.extract()?;
    Ok(maplibre_core::ScreenPoint::new(x, y))
}

fn required_dict_item<'py>(dict: &Bound<'py, PyDict>, key: &str) -> PyResult<Bound<'py, PyAny>> {
    dict.get_item(key)?
        .ok_or_else(|| invalid_argument_error(format!("missing required field: {key}")))
}

fn lat_lng_core_to_py(py: Python<'_>, coordinate: &maplibre_core::LatLng) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("latitude", coordinate.latitude)?;
    dict.set_item("longitude", coordinate.longitude)?;
    Ok(dict.into_any().unbind())
}

fn lat_lng_bounds_core_to_py(
    py: Python<'_>,
    bounds: &maplibre_core::LatLngBounds,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("southwest", lat_lng_core_to_py(py, &bounds.southwest)?)?;
    dict.set_item("northeast", lat_lng_core_to_py(py, &bounds.northeast)?)?;
    Ok(dict.into_any().unbind())
}

fn queried_feature_to_py(
    py: Python<'_>,
    feature: maplibre_core::QueriedFeature,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("feature", PyBytes::new(py, &feature.feature))?;
    dict.set_item("source_id", feature.source_id)?;
    dict.set_item("source_layer_id", feature.source_layer_id)?;
    dict.set_item(
        "state",
        feature
            .state
            .as_deref()
            .map(|bytes| PyBytes::new(py, bytes)),
    )?;
    Ok(dict.into_any().unbind())
}

fn queried_features_to_py(
    py: Python<'_>,
    features: Vec<maplibre_core::QueriedFeature>,
) -> PyResult<Py<PyList>> {
    let list = PyList::empty(py);
    for feature in features {
        list.append(queried_feature_to_py(py, feature)?)?;
    }
    Ok(list.unbind())
}

fn source_info_to_py(py: Python<'_>, info: maplibre_core::SourceInfo) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("source_type", info.raw_source_type)?;
    dict.set_item("is_volatile", info.is_volatile)?;
    dict.set_item("attribution", info.attribution)?;
    dict.set_item("url", info.url)?;
    if let Some(tile_json) = info.tile_json {
        let raw = PyDict::new(py);
        raw.set_item("tiles", tile_json.tiles)?;
        raw.set_item("min_zoom", tile_json.min_zoom)?;
        raw.set_item("max_zoom", tile_json.max_zoom)?;
        raw.set_item("scheme", tile_json.scheme.raw_value())?;
        raw.set_item(
            "bounds",
            tile_json
                .bounds
                .as_ref()
                .map(|bounds| lat_lng_bounds_core_to_py(py, bounds))
                .transpose()?,
        )?;
        dict.set_item("tile_json", raw)?;
    } else {
        dict.set_item("tile_json", py.None())?;
    }
    dict.set_item("tile_size", info.tile_size)?;
    dict.set_item(
        "vector_encoding",
        info.vector_encoding
            .map(maplibre_core::VectorTileEncoding::raw_value),
    )?;
    dict.set_item(
        "raster_dem_encoding",
        info.raster_dem_encoding
            .map(maplibre_core::RasterDemEncoding::raw_value),
    )?;
    Ok(dict.into_any().unbind())
}

fn premultiplied_rgba8_image_from_parts(
    width: u32,
    height: u32,
    stride: u32,
    pixels: &[u8],
) -> sys::mln_premultiplied_rgba8_image {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut image = unsafe { sys::mln_premultiplied_rgba8_image_default() };
    image.width = width;
    image.height = height;
    image.stride = stride;
    image.pixels = pixels.as_ptr();
    image.byte_length = pixels.len();
    image
}

/// Materializes style image options plus the stretch storage native borrows.
/// The caller keeps both alive until the C call returns.
#[allow(clippy::too_many_arguments)]
fn style_image_options_from_parts(
    pixel_ratio: Option<f32>,
    sdf: Option<bool>,
    stretch_x: Option<Vec<(f32, f32)>>,
    stretch_y: Option<Vec<(f32, f32)>>,
    content: Option<(f32, f32, f32, f32)>,
    text_fit_width: Option<u32>,
    text_fit_height: Option<u32>,
) -> (
    sys::mln_style_image_options,
    Vec<sys::mln_image_stretch>,
    Vec<sys::mln_image_stretch>,
) {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut options = unsafe { sys::mln_style_image_options_default() };
    if let Some(pixel_ratio) = pixel_ratio {
        options.fields |= sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
        options.pixel_ratio = pixel_ratio;
    }
    if let Some(sdf) = sdf {
        options.fields |= sys::MLN_STYLE_IMAGE_OPTION_SDF;
        options.sdf = sdf;
    }
    let to_native = |stretches: &[(f32, f32)]| -> Vec<sys::mln_image_stretch> {
        stretches
            .iter()
            .map(|(from, to)| sys::mln_image_stretch {
                from: *from,
                to: *to,
            })
            .collect()
    };
    let native_stretch_x = stretch_x.as_deref().map(to_native).unwrap_or_default();
    let native_stretch_y = stretch_y.as_deref().map(to_native).unwrap_or_default();
    if stretch_x.is_some() {
        options.fields |= sys::MLN_STYLE_IMAGE_OPTION_STRETCH_X;
        options.stretch_x_count = native_stretch_x.len();
    }
    if stretch_y.is_some() {
        options.fields |= sys::MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
        options.stretch_y_count = native_stretch_y.len();
    }
    if let Some((left, top, right, bottom)) = content {
        options.fields |= sys::MLN_STYLE_IMAGE_OPTION_CONTENT;
        options.content = sys::mln_image_content {
            left,
            top,
            right,
            bottom,
        };
    }
    if let Some(value) = text_fit_width {
        options.fields |= sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
        options.text_fit_width = value;
    }
    if let Some(value) = text_fit_height {
        options.fields |= sys::MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
        options.text_fit_height = value;
    }
    (options, native_stretch_x, native_stretch_y)
}

fn style_image_info_to_py(py: Python<'_>, info: &sys::mln_style_image_info) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("width", info.width)?;
    dict.set_item("height", info.height)?;
    dict.set_item("stride", info.stride)?;
    dict.set_item("byte_length", info.byte_length)?;
    dict.set_item("pixel_ratio", info.pixel_ratio)?;
    dict.set_item("sdf", info.sdf)?;
    dict.set_item("stretch_x_count", info.stretch_x_count)?;
    dict.set_item("stretch_y_count", info.stretch_y_count)?;
    dict.set_item(
        "content",
        info.has_content.then_some((
            info.content.left,
            info.content.top,
            info.content.right,
            info.content.bottom,
        )),
    )?;
    dict.set_item(
        "text_fit_width",
        info.has_text_fit_width.then_some(info.text_fit_width),
    )?;
    dict.set_item(
        "text_fit_height",
        info.has_text_fit_height.then_some(info.text_fit_height),
    )?;
    Ok(dict.into_any().unbind())
}

fn resource_request_to_py(
    py: Python<'_>,
    request: maplibre_core::ResourceRequest,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("requested_url", request.requested_url)?;
    dict.set_item("resolved_url", request.resolved_url)?;
    dict.set_item("kind", request.raw_kind)?;
    dict.set_item("loading_method", request.raw_loading_method)?;
    dict.set_item("priority", request.raw_priority)?;
    dict.set_item("usage", request.raw_usage)?;
    dict.set_item("storage_policy", request.raw_storage_policy)?;
    if let Some(range) = request.range {
        let range_dict = PyDict::new(py);
        range_dict.set_item("start", range.start)?;
        range_dict.set_item("end", range.end)?;
        dict.set_item("range", range_dict)?;
    } else {
        dict.set_item("range", py.None())?;
    }
    dict.set_item("prior_modified_unix_ms", request.prior_modified_unix_ms)?;
    dict.set_item("prior_expires_unix_ms", request.prior_expires_unix_ms)?;
    dict.set_item("prior_etag", request.prior_etag)?;
    dict.set_item("prior_data", PyBytes::new(py, &request.prior_data))?;
    Ok(dict.into_any().unbind())
}

fn resource_transform_request_to_py(
    py: Python<'_>,
    request: maplibre_core::ResourceTransformRequest,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("kind", request.raw_kind)?;
    dict.set_item("url", request.url)?;
    Ok(dict.into_any().unbind())
}

fn http_header_transform_request_to_py(
    py: Python<'_>,
    request: maplibre_core::HttpHeaderTransformRequest,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("kind", request.raw_kind)?;
    dict.set_item("url", request.url)?;
    Ok(dict.into_any().unbind())
}

fn resource_response_from_py(raw: &Bound<'_, PyAny>) -> PyResult<maplibre_core::ResourceResponse> {
    let mut response = maplibre_core::ResourceResponse::default();
    response.status = resource_response_status_from_raw(raw.get_item("status")?.extract::<u32>()?)?;
    response.error_reason = ResourceErrorReason::from_raw(raw.get_item("error_reason")?.extract()?);
    response.bytes = raw.get_item("bytes")?.extract()?;
    response.error_message = raw.get_item("error_message")?.extract()?;
    response.must_revalidate = raw.get_item("must_revalidate")?.extract()?;
    response.modified_unix_ms = raw.get_item("modified_unix_ms")?.extract()?;
    response.expires_unix_ms = raw.get_item("expires_unix_ms")?.extract()?;
    response.etag = raw.get_item("etag")?.extract()?;
    response.retry_after_unix_ms = raw.get_item("retry_after_unix_ms")?.extract()?;
    Ok(response)
}

fn resource_response_status_from_raw(raw: u32) -> PyResult<ResourceResponseStatus> {
    match raw {
        sys::MLN_RESOURCE_RESPONSE_STATUS_OK => Ok(ResourceResponseStatus::Ok),
        sys::MLN_RESOURCE_RESPONSE_STATUS_ERROR => Ok(ResourceResponseStatus::Error),
        sys::MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT => Ok(ResourceResponseStatus::NoContent),
        sys::MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED => Ok(ResourceResponseStatus::NotModified),
        _ => Err(invalid_argument_error(format!(
            "unknown resource response status cannot be set: {raw}"
        ))),
    }
}

fn start_session_operation(
    session: sys::mln_render_session,
    start: impl FnOnce(sys::mln_render_session, *mut sys::mln_operation) -> sys::mln_status,
) -> PyResult<u64> {
    let mut operation = sys::mln_operation(0);
    maplibre_core::check(start(session, &mut operation)).map_err(map_error)?;
    Ok(operation.0)
}

fn acquire_frame(session: sys::mln_render_session) -> PyResult<sys::mln_acquired_frame> {
    let mut frame = sys::mln_acquired_frame(0);
    maplibre_core::check(unsafe { sys::mln_render_session_acquire_frame(session, &mut frame) })
        .map_err(map_error)?;
    Ok(frame)
}

fn live_acquired_frame(
    frame: &Mutex<sys::mln_acquired_frame>,
) -> PyResult<sys::mln_acquired_frame> {
    let frame = *frame
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    if frame.0 == 0 {
        Err(invalid_state_error("acquired frame handle is closed"))
    } else {
        Ok(frame)
    }
}

fn release_acquired_frame(
    frame: &Mutex<sys::mln_acquired_frame>,
    kind: u32,
    object: usize,
    value: u64,
) -> PyResult<u64> {
    let mut frame = frame
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    if frame.0 == 0 {
        return Err(invalid_state_error("acquired frame handle is closed"));
    }
    let mut sync = unsafe { sys::mln_gpu_sync_default() };
    sync.kind = kind;
    sync.object = object as *mut c_void;
    sync.value = value;
    let mut operation = sys::mln_operation(0);
    maplibre_core::check(unsafe {
        sys::mln_acquired_frame_release_start(&mut *frame, &sync, &mut operation)
    })
    .map_err(map_error)?;
    Ok(operation.0)
}

fn release_acquired_frame_on_drop(frame: &Mutex<sys::mln_acquired_frame>) {
    let Ok(operation) = release_acquired_frame(frame, sys::MLN_GPU_SYNC_CPU_COMPLETE, 0, 0) else {
        return;
    };
    unsafe { sys::mln_operation_release(sys::mln_operation(operation)) };
}

fn producer_sync_to_py(py: Python<'_>, frame: sys::mln_acquired_frame) -> PyResult<Py<PyAny>> {
    let mut sync: sys::mln_gpu_sync = unsafe { std::mem::zeroed() };
    sync.size = std::mem::size_of::<sys::mln_gpu_sync>() as u32;
    maplibre_core::check(unsafe { sys::mln_acquired_frame_get_producer_sync(frame, &mut sync) })
        .map_err(map_error)?;
    let dict = PyDict::new(py);
    dict.set_item("kind", sync.kind)?;
    dict.set_item("object_address", sync.object as usize)?;
    dict.set_item("value", sync.value)?;
    Ok(dict.into_any().unbind())
}

fn acquired_frame_result_to_py(
    py: Python<'_>,
    frame: sys::mln_acquired_frame,
) -> PyResult<Py<PyAny>> {
    let mut result: sys::mln_render_frame_result = unsafe { std::mem::zeroed() };
    result.size = std::mem::size_of::<sys::mln_render_frame_result>() as u32;
    maplibre_core::check(unsafe { sys::mln_acquired_frame_get_result(frame, &mut result) })
        .map_err(map_error)?;
    render_frame_result_to_py(py, &result)
}

#[pymethods]
impl MetalOwnedTextureFrameHandle {
    fn release_start(&self, kind: u32, object_address: usize, value: u64) -> PyResult<u64> {
        release_acquired_frame(&self.frame, kind, object_address, value)
    }

    fn producer_sync(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        producer_sync_to_py(py, live_acquired_frame(&self.frame)?)
    }

    fn result(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        acquired_frame_result_to_py(py, live_acquired_frame(&self.frame)?)
    }

    fn frame(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let frame = live_acquired_frame(&self.frame)?;
        let mut raw: sys::mln_metal_owned_texture_frame = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_metal_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe { sys::mln_acquired_frame_get_metal_texture(frame, &mut raw) })
            .map_err(map_error)?;
        let dict = PyDict::new(py);
        dict.set_item("generation", raw.generation)?;
        dict.set_item("width", raw.width)?;
        dict.set_item("height", raw.height)?;
        dict.set_item("scale_factor", raw.scale_factor)?;
        dict.set_item("frame_id", raw.frame_id)?;
        dict.set_item("pixel_format", raw.pixel_format)?;
        Ok(dict.into_any().unbind())
    }

    fn texture_address(&self) -> PyResult<usize> {
        let frame = live_acquired_frame(&self.frame)?;
        let mut raw: sys::mln_metal_owned_texture_frame = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_metal_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe { sys::mln_acquired_frame_get_metal_texture(frame, &mut raw) })
            .map_err(map_error)?;
        Ok(raw.texture as usize)
    }

    fn device_address(&self) -> PyResult<usize> {
        let frame = live_acquired_frame(&self.frame)?;
        let mut raw: sys::mln_metal_owned_texture_frame = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_metal_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe { sys::mln_acquired_frame_get_metal_texture(frame, &mut raw) })
            .map_err(map_error)?;
        Ok(raw.device as usize)
    }

    #[getter]
    fn closed(&self) -> bool {
        self.frame
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .0
            == 0
    }
}

impl Drop for MetalOwnedTextureFrameHandle {
    fn drop(&mut self) {
        release_acquired_frame_on_drop(&self.frame);
    }
}

#[pymethods]
impl VulkanOwnedTextureFrameHandle {
    fn release_start(&self, kind: u32, object_address: usize, value: u64) -> PyResult<u64> {
        release_acquired_frame(&self.frame, kind, object_address, value)
    }

    fn producer_sync(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        producer_sync_to_py(py, live_acquired_frame(&self.frame)?)
    }

    fn result(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        acquired_frame_result_to_py(py, live_acquired_frame(&self.frame)?)
    }

    fn frame(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let frame = live_acquired_frame(&self.frame)?;
        let mut raw: sys::mln_vulkan_owned_texture_frame = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_vulkan_texture(frame, &mut raw)
        })
        .map_err(map_error)?;
        let dict = PyDict::new(py);
        dict.set_item("generation", raw.generation)?;
        dict.set_item("width", raw.width)?;
        dict.set_item("height", raw.height)?;
        dict.set_item("scale_factor", raw.scale_factor)?;
        dict.set_item("frame_id", raw.frame_id)?;
        dict.set_item("format", raw.format)?;
        dict.set_item("layout", raw.layout)?;
        Ok(dict.into_any().unbind())
    }

    fn image_address(&self) -> PyResult<usize> {
        let frame = live_acquired_frame(&self.frame)?;
        let mut raw: sys::mln_vulkan_owned_texture_frame = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_vulkan_texture(frame, &mut raw)
        })
        .map_err(map_error)?;
        Ok(raw.image as usize)
    }

    fn image_view_address(&self) -> PyResult<usize> {
        let frame = live_acquired_frame(&self.frame)?;
        let mut raw: sys::mln_vulkan_owned_texture_frame = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_vulkan_texture(frame, &mut raw)
        })
        .map_err(map_error)?;
        Ok(raw.image_view as usize)
    }

    fn device_address(&self) -> PyResult<usize> {
        let frame = live_acquired_frame(&self.frame)?;
        let mut raw: sys::mln_vulkan_owned_texture_frame = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_vulkan_texture(frame, &mut raw)
        })
        .map_err(map_error)?;
        Ok(raw.device as usize)
    }

    #[getter]
    fn closed(&self) -> bool {
        self.frame
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .0
            == 0
    }
}

impl Drop for VulkanOwnedTextureFrameHandle {
    fn drop(&mut self) {
        release_acquired_frame_on_drop(&self.frame);
    }
}

#[pymethods]
impl WebGPUOwnedTextureFrameHandle {
    fn release_start(&self, kind: u32, object_address: usize, value: u64) -> PyResult<u64> {
        release_acquired_frame(&self.frame, kind, object_address, value)
    }

    fn producer_sync(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        producer_sync_to_py(py, live_acquired_frame(&self.frame)?)
    }

    fn result(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        acquired_frame_result_to_py(py, live_acquired_frame(&self.frame)?)
    }

    fn frame(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let raw = webgpu_owned_texture_frame(&self.frame)?;
        let dict = PyDict::new(py);
        dict.set_item("generation", raw.generation)?;
        dict.set_item("width", raw.width)?;
        dict.set_item("height", raw.height)?;
        dict.set_item("scale_factor", raw.scale_factor)?;
        dict.set_item("frame_id", raw.frame_id)?;
        dict.set_item("format", raw.format)?;
        Ok(dict.into_any().unbind())
    }

    fn texture_address(&self) -> PyResult<usize> {
        Ok(webgpu_owned_texture_frame(&self.frame)?.texture as usize)
    }

    fn texture_view_address(&self) -> PyResult<usize> {
        Ok(webgpu_owned_texture_frame(&self.frame)?.texture_view as usize)
    }

    fn device_address(&self) -> PyResult<usize> {
        Ok(webgpu_owned_texture_frame(&self.frame)?.device as usize)
    }

    #[getter]
    fn closed(&self) -> bool {
        self.frame
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .0
            == 0
    }
}

impl Drop for WebGPUOwnedTextureFrameHandle {
    fn drop(&mut self) {
        release_acquired_frame_on_drop(&self.frame);
    }
}

fn webgpu_owned_texture_frame(
    frame: &Mutex<sys::mln_acquired_frame>,
) -> PyResult<sys::mln_webgpu_owned_texture_frame> {
    let frame = live_acquired_frame(frame)?;
    let mut raw: sys::mln_webgpu_owned_texture_frame = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_webgpu_owned_texture_frame>() as u32;
    maplibre_core::check(unsafe { sys::mln_acquired_frame_get_webgpu_texture(frame, &mut raw) })
        .map_err(map_error)?;
    Ok(raw)
}

#[pymethods]
impl OpenGLOwnedTextureFrameHandle {
    fn release_start(&self, kind: u32, object_address: usize, value: u64) -> PyResult<u64> {
        release_acquired_frame(&self.frame, kind, object_address, value)
    }

    fn producer_sync(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        producer_sync_to_py(py, live_acquired_frame(&self.frame)?)
    }

    fn result(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        acquired_frame_result_to_py(py, live_acquired_frame(&self.frame)?)
    }

    fn frame(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let frame = live_acquired_frame(&self.frame)?;
        let mut raw: sys::mln_opengl_owned_texture_frame = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_opengl_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_opengl_texture(frame, &mut raw)
        })
        .map_err(map_error)?;
        let dict = PyDict::new(py);
        dict.set_item("generation", raw.generation)?;
        dict.set_item("width", raw.width)?;
        dict.set_item("height", raw.height)?;
        dict.set_item("scale_factor", raw.scale_factor)?;
        dict.set_item("frame_id", raw.frame_id)?;
        dict.set_item("target", raw.target)?;
        dict.set_item("internal_format", raw.internal_format)?;
        dict.set_item("format", raw.format)?;
        dict.set_item("type", raw.type_)?;
        Ok(dict.into_any().unbind())
    }

    fn texture(&self) -> PyResult<u32> {
        let frame = live_acquired_frame(&self.frame)?;
        let mut raw: sys::mln_opengl_owned_texture_frame = unsafe { std::mem::zeroed() };
        raw.size = std::mem::size_of::<sys::mln_opengl_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_opengl_texture(frame, &mut raw)
        })
        .map_err(map_error)?;
        Ok(raw.texture)
    }

    #[getter]
    fn closed(&self) -> bool {
        self.frame
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .0
            == 0
    }
}

impl Drop for OpenGLOwnedTextureFrameHandle {
    fn drop(&mut self) {
        release_acquired_frame_on_drop(&self.frame);
    }
}

fn render_frame_result_to_py(
    py: Python<'_>,
    value: &sys::mln_render_frame_result,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("disposition", value.disposition)?;
    dict.set_item("token", value.token)?;
    dict.set_item("map_update_generation", value.map_update_generation)?;
    dict.set_item("extent_generation", value.extent_generation)?;
    dict.set_item("frame_generation", value.frame_generation)?;
    dict.set_item("needs_repaint", value.needs_repaint)?;
    Ok(dict.into_any().unbind())
}

fn render_session_snapshot_to_py(
    py: Python<'_>,
    value: &sys::mln_render_session_snapshot,
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("state", value.state)?;
    dict.set_item("driver", value.driver)?;
    dict.set_item("latest_result", value.latest_result)?;
    dict.set_item(
        "extent",
        (
            value.extent.width,
            value.extent.height,
            value.extent.scale_factor,
        ),
    )?;
    dict.set_item("generation", value.generation)?;
    dict.set_item("map_update_generation", value.map_update_generation)?;
    dict.set_item(
        "rendered_update_generation",
        value.rendered_update_generation,
    )?;
    dict.set_item("extent_generation", value.extent_generation)?;
    dict.set_item("frame_generation", value.frame_generation)?;
    dict.set_item("latest_demand_token", value.latest_demand_token)?;
    dict.set_item("pending_demand_count", value.pending_demand_count)?;
    dict.set_item("acquired_frame_count", value.acquired_frame_count)?;
    dict.set_item("target_ready", value.target_ready)?;
    dict.set_item("pending_changes", value.pending_changes)?;
    Ok(dict.into_any().unbind())
}

fn map_error(error: Error) -> PyErr {
    let raw_status = error.raw_status();
    let diagnostic = error.diagnostic().to_owned();
    match error.kind() {
        ErrorKind::NotFound => py_errors::NotFoundError::new_err((raw_status, diagnostic)),
        ErrorKind::InvalidArgument => {
            py_errors::InvalidArgumentError::new_err((raw_status, diagnostic))
        }
        ErrorKind::InvalidState => py_errors::InvalidStateError::new_err((raw_status, diagnostic)),
        ErrorKind::WrongThread => py_errors::WrongThreadError::new_err((raw_status, diagnostic)),
        ErrorKind::Unsupported => {
            py_errors::UnsupportedFeatureError::new_err((raw_status, diagnostic))
        }
        ErrorKind::NativeError => py_errors::NativeError::new_err((raw_status, diagnostic)),
        ErrorKind::Cancelled => py_errors::CancelledError::new_err((raw_status, diagnostic)),
        ErrorKind::Busy => py_errors::BusyError::new_err((raw_status, diagnostic)),
        ErrorKind::TargetLost => py_errors::TargetLostError::new_err((raw_status, diagnostic)),
        ErrorKind::NotReady => py_errors::NotReadyError::new_err((raw_status, diagnostic)),
        ErrorKind::UnknownStatus => {
            py_errors::UnknownStatusError::new_err((raw_status.unwrap_or_default(), diagnostic))
        }
        ErrorKind::AbiVersionMismatch => {
            py_errors::UnsupportedFeatureError::new_err((raw_status, diagnostic))
        }
        _ => py_errors::NativeError::new_err((raw_status, diagnostic)),
    }
}

/// Returns the C ABI version expected by the shared Rust adaptation layer.
#[pyfunction]
fn expected_c_abi_version() -> u32 {
    maplibre_core::EXPECTED_C_ABI_VERSION
}

/// Returns the native C ABI contract version reported by the linked library.
#[pyfunction]
fn c_version() -> u32 {
    // SAFETY: mln_c_version takes no arguments and returns the process-global C
    // ABI version for the linked native library.
    unsafe { sys::mln_c_version() }
}

/// Returns the raw render-backend support mask reported by the linked library.
#[pyfunction]
fn supported_render_backends_raw() -> u32 {
    // SAFETY: mln_supported_render_backend_mask takes no arguments and returns
    // a value mask. The Python layer preserves unknown future bits.
    unsafe { sys::mln_supported_render_backend_mask() }
}

/// Returns the raw OpenGL context-provider support mask reported by the linked library.
#[pyfunction]
fn supported_opengl_context_providers_raw() -> u32 {
    // SAFETY: mln_opengl_supported_context_provider_mask takes no arguments and returns
    // a value mask. The Python layer preserves unknown future bits.
    unsafe { sys::mln_opengl_supported_context_provider_mask() }
}

/// Returns the physical device-pixel size for a logical render target extent.
#[pyfunction]
fn render_target_extent_physical_size(
    width: u32,
    height: u32,
    scale_factor: f64,
) -> PyResult<(u32, u32)> {
    let extent = maplibre_core::render::render_target_extent_to_native(
        maplibre_core::render::RenderTargetExtentFields {
            width,
            height,
            scale_factor,
        },
    );
    let mut out_width = 0u32;
    let mut out_height = 0u32;
    // SAFETY: extent is fully initialized and both out pointers reference live
    // locals for the duration of the call.
    maplibre_core::check(unsafe {
        sys::mln_render_target_extent_physical_size(&extent, &mut out_width, &mut out_height)
    })
    .map_err(map_error)?;
    Ok((out_width, out_height))
}

/// Returns the raw process-global network status reported by the linked library.
#[pyfunction]
fn network_status_raw() -> PyResult<u32> {
    maplibre_core::network_status()
        .map(NetworkStatus::raw_value)
        .map_err(map_error)
}

#[pyfunction]
fn set_log_callback(max_queued_records: usize, consume: bool) -> PyResult<LogReceiver> {
    if max_queued_records == 0 {
        return Err(invalid_argument_error(
            "max_queued_records must be greater than zero",
        ));
    }
    let replacement = PyLogCallbackState::new(max_queued_records, consume);
    let user_data = Arc::as_ptr(&replacement).cast_mut().cast::<c_void>();
    // SAFETY: log_callback_trampoline has the C callback ABI. user_data points
    // to replacement, which is retained by LOG_CALLBACK_STATE after success.
    maplibre_core::check(unsafe {
        sys::mln_log_set_callback(Some(log_callback_trampoline), user_data)
    })
    .map_err(map_error)?;
    let mut state = LOG_CALLBACK_STATE
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    if let Some(previous) = state.current.take() {
        state.retired.push(previous);
    }
    state.current = Some(Arc::clone(&replacement));
    Ok(LogReceiver { state: replacement })
}

#[pyfunction]
fn clear_log_callback() -> PyResult<()> {
    // SAFETY: mln_log_clear_callback takes no arguments and clears native's
    // process-global callback slot.
    maplibre_core::check(unsafe { sys::mln_log_clear_callback() }).map_err(map_error)?;
    let mut state = LOG_CALLBACK_STATE
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    if let Some(previous) = state.current.take() {
        state.retired.push(previous);
    }
    Ok(())
}

#[pyfunction]
fn set_async_log_severity_mask(mask: u32) -> PyResult<()> {
    // SAFETY: mask is passed by value and validated by the C API.
    maplibre_core::check(unsafe { sys::mln_log_set_async_severity_mask(mask) }).map_err(map_error)
}

unsafe extern "C" fn log_callback_trampoline(
    user_data: *mut c_void,
    severity: u32,
    event: u32,
    code: i64,
    message: *const c_char,
) -> u32 {
    catch_unwind(AssertUnwindSafe(|| {
        let Some(state) = ptr::NonNull::new(user_data.cast::<PyLogCallbackState>()) else {
            return 0;
        };
        // SAFETY: user_data points to PyLogCallbackState retained after successful
        // callback installation.
        let state = unsafe { state.as_ref() };
        // SAFETY: message follows the C logging callback contract.
        let Ok(record) =
            (unsafe { maplibre_core::logging::copy_log_record(severity, event, code, message) })
        else {
            return 0;
        };
        state.push(CopiedLogRecordRaw {
            severity: log_severity_raw(record.severity),
            event: log_event_raw(record.event),
            code: record.code,
            message: record.message,
        })
    }))
    .unwrap_or(0)
}

#[pyfunction]
fn projected_meters_for_lat_lng(
    py: Python<'_>,
    latitude: f64,
    longitude: f64,
) -> PyResult<Py<PyAny>> {
    let mut meters = sys::mln_projected_meters {
        northing: 0.0,
        easting: 0.0,
    };
    // SAFETY: The C API validates the coordinate and output pointer.
    maplibre_core::check(unsafe {
        sys::mln_projected_meters_for_lat_lng(
            sys::mln_lat_lng {
                latitude,
                longitude,
            },
            &mut meters,
        )
    })
    .map_err(map_error)?;
    let dict = PyDict::new(py);
    dict.set_item("northing", meters.northing)?;
    dict.set_item("easting", meters.easting)?;
    Ok(dict.into_any().unbind())
}

#[pyfunction]
fn lat_lng_for_projected_meters(
    py: Python<'_>,
    northing: f64,
    easting: f64,
) -> PyResult<Py<PyAny>> {
    let mut coordinate = sys::mln_lat_lng {
        latitude: 0.0,
        longitude: 0.0,
    };
    // SAFETY: The C API validates the meters value and output pointer.
    maplibre_core::check(unsafe {
        sys::mln_lat_lng_for_projected_meters(
            sys::mln_projected_meters { northing, easting },
            &mut coordinate,
        )
    })
    .map_err(map_error)?;
    lat_lng_to_py(py, coordinate)
}

/// Sets the process-global network status from a raw C enum value.
#[pyfunction]
fn set_network_status_raw(raw_status: u32) -> PyResult<()> {
    maplibre_core::set_network_status(NetworkStatus::from_raw(raw_status)).map_err(map_error)
}

/// Test helper that snapshots a raw map id, which the safe API cannot express.
#[pyfunction]
fn map_size_by_id_for_test(id: u64) -> PyResult<(u32, u32, f64)> {
    let mut snapshot: sys::mln_map_snapshot = unsafe { std::mem::zeroed() };
    snapshot.size = std::mem::size_of::<sys::mln_map_snapshot>() as u32;
    maplibre_core::check(unsafe { sys::mln_map_snapshot_get(sys::mln_map(id), &mut snapshot) })
        .map_err(map_error)?;
    Ok((
        snapshot.logical_extent.width,
        snapshot.logical_extent.height,
        snapshot.logical_extent.scale_factor,
    ))
}

/// Test helper that lets native status conversion see C validation failures.
#[pyfunction]
fn set_network_status_raw_unchecked_for_test(raw_status: u32) -> PyResult<()> {
    maplibre_core::set_network_status_raw(raw_status).map_err(map_error)
}

/// Test helper that maps synthetic native statuses through the public error shape.
#[pyfunction]
fn status_error_for_test(raw_status: i32, diagnostic: String) -> PyResult<()> {
    if raw_status == sys::MLN_STATUS_OK {
        Ok(())
    } else {
        Err(map_error(Error::from_status_and_diagnostic(
            raw_status, diagnostic,
        )))
    }
}

/// Test helper that verifies later support work does not replace a copied error.
#[pyfunction]
fn status_error_after_support_call_for_test(raw_status: i32, diagnostic: String) -> PyResult<()> {
    if raw_status == sys::MLN_STATUS_OK {
        return Ok(());
    }
    let error = Error::from_status_and_diagnostic(raw_status, diagnostic);
    // SAFETY: The raw value is passed by value. The intentionally invalid value
    // forces a later diagnostic-writing native call for cleanup/support tests.
    let _ = unsafe { sys::mln_network_status_set(u32::MAX) };
    Err(map_error(error))
}

/// Test helper that validates an injected ABI version before creating a runtime.
#[pyfunction]
fn create_runtime_with_abi_version_for_test(
    py: Python<'_>,
    actual_abi_version: u32,
    asset_path: Option<String>,
    cache_path: Option<String>,
) -> PyResult<RuntimeHandle> {
    maplibre_core::validate_abi_version_value(actual_abi_version).map_err(map_error)?;
    create_runtime(
        py,
        asset_path,
        cache_path,
        maplibre_core::RuntimeEventMask::ALL.bits(),
    )
}

/// Field values for one event of a synthetic batch built by a test helper.
struct SyntheticEvent {
    type_: u32,
    source_type: u32,
    source: u64,
    code: i32,
    payload_type: u32,
    payload: sys::mln_runtime_event_payload,
    message: &'static str,
}

/// Returns a payload union whose bytes are zero, which is what the C API writes
/// for an event that carries no payload.
fn zeroed_event_payload() -> sys::mln_runtime_event_payload {
    // SAFETY: Every union member is a POD struct with no niche, so an all-zero
    // bit pattern is a valid value of the union.
    unsafe { std::mem::zeroed() }
}

/// Returns a payload union whose leading bytes are `bytes`, standing in for a
/// payload type a later library version defines.
fn opaque_event_payload(bytes: &[u8]) -> sys::mln_runtime_event_payload {
    let mut payload = zeroed_event_payload();
    let len = bytes
        .len()
        .min(std::mem::size_of::<sys::mln_runtime_event_payload>());
    // SAFETY: len is at most the size of the union, payload is a live writable
    // union, and union bytes have no validity requirement.
    unsafe {
        ptr::copy_nonoverlapping(bytes.as_ptr(), (&raw mut payload).cast::<u8>(), len);
    }
    payload
}

/// Builds the event array and message arena of a synthetic batch.
fn synthetic_event_storage(specs: &[SyntheticEvent]) -> (Vec<sys::mln_runtime_event>, Vec<u8>) {
    let mut events = Vec::with_capacity(specs.len());
    let mut messages = Vec::new();
    for spec in specs {
        let (message_offset, message_size) = if spec.message.is_empty() {
            (0, 0)
        } else {
            let offset = messages.len() as u64;
            messages.extend_from_slice(spec.message.as_bytes());
            messages.push(0);
            (offset, spec.message.len() as u32)
        };
        events.push(sys::mln_runtime_event {
            type_: spec.type_,
            source_type: spec.source_type,
            source: spec.source,
            code: spec.code,
            payload_type: spec.payload_type,
            message_offset,
            message_size,
            payload: spec.payload,
        });
    }
    (events, messages)
}

/// Decodes a synthetic batch that covers every payload shape plus an event whose
/// type, source type, and payload type this version does not define.
///
/// A live runtime never queues the unknown event, and the offline payloads it
/// queues need a populated cache database, so this is how the Python suite
/// reaches the whole wire contract. The backing array and arena are overwritten
/// before the wire batch is returned, so the values it carries prove the decode
/// copied them.
#[pyfunction]
fn synthetic_runtime_event_batch_for_test(py: Python<'_>) -> PyResult<Py<PyAny>> {
    let mut status = empty_offline_region_status();
    status.download_state = sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE;
    status.completed_resource_count = 7;
    status.completed_resource_size = 8;
    status.completed_tile_count = 9;
    status.required_tile_count = 10;
    status.completed_tile_size = 11;
    status.required_resource_count = 12;
    status.required_resource_count_is_precise = true;

    let specs = [
        SyntheticEvent {
            type_: sys::MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_MAP,
            source: 0,
            code: 0,
            payload_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME,
            payload: sys::mln_runtime_event_payload {
                render_frame: sys::mln_runtime_event_render_frame {
                    mode: sys::MLN_RENDER_MODE_FULL,
                    needs_repaint: true,
                    placement_changed: false,
                    stats: sys::mln_rendering_stats {
                        encoding_time: 1.25,
                        rendering_time: 2.5,
                        frame_count: 3,
                        draw_call_count: 4,
                        total_draw_call_count: 5,
                    },
                },
            },
            message: "",
        },
        SyntheticEvent {
            type_: sys::MLN_RUNTIME_EVENT_MAP_TILE_ACTION,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_MAP,
            // A map handle id that names no live map, so the decode has to carry
            // the raw identity through with nothing to resolve it against.
            source: 0x0100_0000_0000_0007,
            code: 0,
            payload_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION,
            payload: sys::mln_runtime_event_payload {
                tile_action: sys::mln_runtime_event_tile_action {
                    operation: sys::MLN_TILE_OPERATION_LOAD_FROM_NETWORK,
                    tile_id: sys::mln_tile_id {
                        overscaled_z: 6,
                        wrap: -1,
                        canonical_z: 5,
                        canonical_x: 12,
                        canonical_y: 34,
                    },
                },
            },
            message: "source-a",
        },
        SyntheticEvent {
            type_: sys::MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_MAP,
            source: 0,
            code: 0,
            payload_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE,
            payload: zeroed_event_payload(),
            message: "missing-image",
        },
        SyntheticEvent {
            type_: sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
            source: 0,
            code: 0,
            payload_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS,
            payload: sys::mln_runtime_event_payload {
                offline_region_status: sys::mln_runtime_event_offline_region_status {
                    region_id: 42,
                    status,
                },
            },
            message: "",
        },
        SyntheticEvent {
            type_: sys::MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_MAP,
            source: 0,
            code: 0,
            payload_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED,
            payload: sys::mln_runtime_event_payload {
                camera_transition_finished: sys::mln_runtime_event_camera_transition_finished {
                    transition_id: 909,
                },
            },
            message: "",
        },
        SyntheticEvent {
            type_: 999_001,
            source_type: 999_003,
            // A source identity of a kind this version does not name.
            source: 0x0200_0000_0000_002a,
            code: -7,
            payload_type: 999_002,
            payload: opaque_event_payload(&[1, 2, 3, 4]),
            message: "future payload",
        },
    ];

    let (mut events, mut messages) = synthetic_event_storage(&specs);
    let batch = sys::mln_runtime_event_batch_view {
        size: std::mem::size_of::<sys::mln_runtime_event_batch_view>() as u32,
        event_size: std::mem::size_of::<sys::mln_runtime_event>() as u32,
        events: events.as_ptr(),
        event_count: events.len(),
        messages: messages.as_ptr().cast(),
        messages_size: messages.len(),
    };

    // SAFETY: batch names the array and arena built above, which outlive this
    // call, and its event_size is the stride of that array.
    let mut copied = Vec::with_capacity(batch.event_count);
    for event in unsafe { maplibre_core::events::drain_batch(&batch) } {
        copied.push(event.map_err(map_error)?);
    }

    events.fill(sys::mln_runtime_event {
        type_: 0,
        source_type: 0,
        source: 0,
        code: 0,
        payload_type: 0,
        message_offset: 0,
        message_size: 0,
        payload: opaque_event_payload(&[0xFF; 8]),
    });
    messages.fill(b'x');
    event_batch_to_py(py, copied)
}

/// Reports the event stride a drain of `runtime` names and the stride this
/// extension compiled against, so the Python suite can assert the decode steps
/// by the batch's own value.
#[pyfunction]
fn runtime_event_stride_for_test(runtime: &RuntimeHandle) -> PyResult<(u32, u32)> {
    let state = runtime.state_for_operation()?;
    let mut batch = sys::mln_event_batch(0);
    maplibre_core::check(unsafe { sys::mln_runtime_drain_events(state.handle(), &mut batch) })
        .map_err(map_error)?;
    let mut view: sys::mln_runtime_event_batch_view = unsafe { std::mem::zeroed() };
    view.size = std::mem::size_of::<sys::mln_runtime_event_batch_view>() as u32;
    let status = unsafe { sys::mln_event_batch_get(batch, &mut view) };
    unsafe { sys::mln_event_batch_release(batch) };
    maplibre_core::check(status).map_err(map_error)?;
    Ok((
        view.event_size,
        std::mem::size_of::<sys::mln_runtime_event>() as u32,
    ))
}

fn owned_buffer_to_py(py: Python<'_>, buffer: sys::mln_buffer) -> PyResult<Py<PyBytes>> {
    // SAFETY: The buffer is an owned handle returned by the C API.
    let bytes = unsafe { maplibre_core::string::copy_owned_buffer(buffer) }.map_err(map_error)?;
    Ok(PyBytes::new(py, &bytes).unbind())
}

fn owned_buffer_to_string(buffer: sys::mln_buffer) -> PyResult<String> {
    let bytes = unsafe { maplibre_core::string::copy_owned_buffer(buffer) }.map_err(map_error)?;
    String::from_utf8(bytes)
        .map_err(|error| invalid_argument_error(format!("native text is not UTF-8: {error}")))
}

/// Returns the raw event mask a runtime selects by default.
///
/// The value carries every bit the linked library's creation default selects,
/// including bits this build does not name, so the Python layer keeps them
/// rather than substituting its own constant.
#[pyfunction]
fn runtime_options_default_event_mask() -> u64 {
    maplibre_core::RuntimeOptions::default().event_mask.bits()
}

/// Returns the raw event mask a map selects by default.
///
/// The value carries every bit the linked library's creation default selects,
/// including bits this build does not name, so the Python layer keeps them
/// rather than substituting its own constant.
#[pyfunction]
fn map_options_default_event_mask() -> u64 {
    maplibre_core::MapOptions::default().event_mask.bits()
}

/// Creates a runtime and blocks without holding the GIL until native startup completes.
#[pyfunction]
fn create_runtime(
    py: Python<'_>,
    asset_path: Option<String>,
    cache_path: Option<String>,
    event_mask: u64,
) -> PyResult<RuntimeHandle> {
    maplibre_core::validate_abi_version().map_err(map_error)?;
    let mut options = maplibre_core::RuntimeOptions::default();
    options.asset_path = asset_path;
    options.cache_path = cache_path;
    let native_options =
        maplibre_core::runtime::runtime_options_to_native(&options).map_err(map_error)?;
    let mut raw_options = native_options.to_raw();
    raw_options.event_mask = event_mask;
    let mut source = sys::mln_notification_source(0);
    maplibre_core::check(unsafe { sys::mln_notification_source_create(&mut source) })
        .map_err(map_error)?;
    raw_options.notification_source = source;

    let mut out = maplibre_core::ptr::OutHandle::<sys::mln_runtime>::new();
    let options_address = std::ptr::addr_of!(raw_options) as usize;
    let out_address = out.as_mut_ptr() as usize;
    let create_status = py.detach(move || unsafe {
        sys::mln_runtime_create(
            options_address as *const sys::mln_runtime_options,
            out_address as *mut sys::mln_runtime,
        )
    });
    if let Err(error) = maplibre_core::check(create_status) {
        unsafe { sys::mln_notification_source_release(source) };
        return Err(map_error(error));
    }
    let native = out.into_live("mln_runtime").map_err(map_error)?;
    let state =
        unsafe { maplibre_core::handle::NativeHandleState::from_handle(native, "mln_runtime") }
            .map_err(map_error)?;
    Ok(RuntimeHandle {
        state: Mutex::new(state),
        notification_source: Mutex::new(source),
        operation_gate: RuntimeOperationGate::new(),
        notification_callback: Mutex::new(None),
    })
}

/// Creates a map handle owned by a runtime.
#[pyfunction]
fn create_map(
    py: Python<'_>,
    runtime: &RuntimeHandle,
    width: Option<u32>,
    height: Option<u32>,
    scale_factor: Option<f64>,
    map_mode: Option<u32>,
    fast_pfor_enabled: Option<bool>,
    event_mask: u64,
) -> PyResult<MapHandle> {
    let mut options = maplibre_core::MapOptions::default();
    if let Some(width) = width {
        options.width = width;
    }
    if let Some(height) = height {
        options.height = height;
    }
    if let Some(scale_factor) = scale_factor {
        options.scale_factor = scale_factor;
    }
    if let Some(map_mode) = map_mode {
        options.mode = maplibre_core::MapMode::from_raw(map_mode);
    }
    if let Some(fast_pfor_enabled) = fast_pfor_enabled {
        options.fast_pfor_enabled = fast_pfor_enabled;
    }
    let mut raw_options =
        maplibre_core::options::map_options_to_native(&options).map_err(map_error)?;
    raw_options.event_mask = event_mask;
    let runtime_state = runtime.state_for_operation()?;
    let mut operation = sys::mln_operation(0);
    maplibre_core::check(unsafe {
        sys::mln_map_create_start(runtime_state.handle(), &raw_options, &mut operation)
    })
    .map_err(map_error)?;
    drop(runtime_state);
    let operation = OwnedOperation(operation);
    wait_operation(py, operation.0)?;
    let mut out = maplibre_core::ptr::OutHandle::<sys::mln_map>::new();
    maplibre_core::check(unsafe { sys::mln_map_create_take_result(operation.0, out.as_mut_ptr()) })
        .map_err(map_error)?;
    let native = out.into_live("mln_map").map_err(map_error)?;
    let state = unsafe { maplibre_core::handle::NativeHandleState::from_handle(native, "mln_map") }
        .map_err(map_error)?;
    Ok(MapHandle {
        state: Mutex::new(state),
    })
}

fn attach_render_session<F>(
    map: &MapHandle,
    driver: u32,
    texture_ring_depth: u32,
    attach: F,
) -> PyResult<(RenderSessionHandle, u64)>
where
    F: FnOnce(
        sys::mln_map,
        *const sys::mln_render_session_attach_options,
        *mut sys::mln_render_session,
        *mut sys::mln_operation,
    ) -> sys::mln_status,
{
    let map_state = map.state();
    let mut options = unsafe { sys::mln_render_session_attach_options_default() };
    options.driver = driver;
    options.requested_texture_ring_depth = texture_ring_depth;
    let mut out = maplibre_core::ptr::OutHandle::<sys::mln_render_session>::new();
    let mut operation = sys::mln_operation(0);
    maplibre_core::check(attach(
        map_state.handle(),
        &options,
        out.as_mut_ptr(),
        &mut operation,
    ))
    .map_err(map_error)?;
    let native = out.into_live("mln_render_session").map_err(map_error)?;
    Ok((
        RenderSessionHandle {
            state: Arc::new(Mutex::new(RenderSessionState::new(native)?)),
        },
        operation.0,
    ))
}

#[pyfunction]
fn attach_metal_surface(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    device_address: usize,
    layer_address: usize,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::metal_surface_descriptor_to_native(
        maplibre_core::render::MetalSurfaceDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            context: maplibre_core::render::MetalContextDescriptorFields {
                device: device_address as *mut c_void,
            },
            layer: layer_address as *mut c_void,
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_metal_surface_attach_start(map, &descriptor, options, session, operation)
        },
    )
}

#[pyfunction]
fn attach_vulkan_surface(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    instance_address: usize,
    physical_device_address: usize,
    device_address: usize,
    graphics_queue_address: usize,
    graphics_queue_family_index: u32,
    get_instance_proc_addr: usize,
    get_device_proc_addr: usize,
    surface_address: usize,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::vulkan_surface_descriptor_to_native(
        maplibre_core::render::VulkanSurfaceDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            context: vulkan_context_fields(
                instance_address,
                physical_device_address,
                device_address,
                graphics_queue_address,
                graphics_queue_family_index,
                get_instance_proc_addr,
                get_device_proc_addr,
            ),
            surface: surface_address as *mut c_void,
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_vulkan_surface_attach_start(map, &descriptor, options, session, operation)
        },
    )
}

#[pyfunction]
fn attach_metal_owned_texture(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    device_address: usize,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::metal_owned_texture_descriptor_to_native(
        maplibre_core::render::MetalOwnedTextureDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            context: maplibre_core::render::MetalContextDescriptorFields {
                device: device_address as *mut c_void,
            },
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_metal_owned_texture_attach_start(map, &descriptor, options, session, operation)
        },
    )
}

#[pyfunction]
fn attach_metal_borrowed_texture(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    physical_width: u32,
    physical_height: u32,
    texture_address: usize,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::metal_borrowed_texture_descriptor_to_native(
        maplibre_core::render::MetalBorrowedTextureDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            physical_width,
            physical_height,
            texture: texture_address as *mut c_void,
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_metal_borrowed_texture_attach_start(
                map,
                &descriptor,
                options,
                session,
                operation,
            )
        },
    )
}

#[pyfunction]
fn attach_vulkan_owned_texture(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    instance_address: usize,
    physical_device_address: usize,
    device_address: usize,
    graphics_queue_address: usize,
    graphics_queue_family_index: u32,
    get_instance_proc_addr: usize,
    get_device_proc_addr: usize,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::vulkan_owned_texture_descriptor_to_native(
        maplibre_core::render::VulkanOwnedTextureDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            context: vulkan_context_fields(
                instance_address,
                physical_device_address,
                device_address,
                graphics_queue_address,
                graphics_queue_family_index,
                get_instance_proc_addr,
                get_device_proc_addr,
            ),
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_vulkan_owned_texture_attach_start(
                map,
                &descriptor,
                options,
                session,
                operation,
            )
        },
    )
}

#[pyfunction]
#[allow(clippy::too_many_arguments)]
fn attach_vulkan_borrowed_texture(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    physical_width: u32,
    physical_height: u32,
    instance_address: usize,
    physical_device_address: usize,
    device_address: usize,
    graphics_queue_address: usize,
    graphics_queue_family_index: u32,
    get_instance_proc_addr: usize,
    get_device_proc_addr: usize,
    image_address: usize,
    image_view_address: usize,
    format: u32,
    initial_layout: u32,
    final_layout: u32,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::vulkan_borrowed_texture_descriptor_to_native(
        maplibre_core::render::VulkanBorrowedTextureDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            physical_width,
            physical_height,
            context: vulkan_context_fields(
                instance_address,
                physical_device_address,
                device_address,
                graphics_queue_address,
                graphics_queue_family_index,
                get_instance_proc_addr,
                get_device_proc_addr,
            ),
            image: image_address as *mut c_void,
            image_view: image_view_address as *mut c_void,
            format,
            initial_layout,
            final_layout,
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_vulkan_borrowed_texture_attach_start(
                map,
                &descriptor,
                options,
                session,
                operation,
            )
        },
    )
}

#[pyfunction]
#[allow(clippy::too_many_arguments)]
fn attach_webgpu_surface(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    instance_address: usize,
    device_address: usize,
    queue_address: usize,
    surface_address: usize,
    format: u32,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::webgpu_surface_descriptor_to_native(
        maplibre_core::render::WebGpuSurfaceDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            context: webgpu_context_fields(instance_address, device_address, queue_address),
            surface: surface_address as *mut c_void,
            format,
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_webgpu_surface_attach_start(map, &descriptor, options, session, operation)
        },
    )
}

#[pyfunction]
fn attach_webgpu_owned_texture(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    instance_address: usize,
    device_address: usize,
    queue_address: usize,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::webgpu_owned_texture_descriptor_to_native(
        maplibre_core::render::WebGpuOwnedTextureDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            context: webgpu_context_fields(instance_address, device_address, queue_address),
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_webgpu_owned_texture_attach_start(
                map,
                &descriptor,
                options,
                session,
                operation,
            )
        },
    )
}

#[pyfunction]
#[allow(clippy::too_many_arguments)]
fn attach_webgpu_borrowed_texture(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    physical_width: u32,
    physical_height: u32,
    instance_address: usize,
    device_address: usize,
    queue_address: usize,
    texture_address: usize,
    texture_view_address: usize,
    format: u32,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::webgpu_borrowed_texture_descriptor_to_native(
        maplibre_core::render::WebGpuBorrowedTextureDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            physical_width,
            physical_height,
            context: webgpu_context_fields(instance_address, device_address, queue_address),
            texture: texture_address as *mut c_void,
            texture_view: texture_view_address as *mut c_void,
            format,
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_webgpu_borrowed_texture_attach_start(
                map,
                &descriptor,
                options,
                session,
                operation,
            )
        },
    )
}

#[pyfunction]
#[allow(clippy::too_many_arguments)]
fn attach_opengl_surface(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    context_platform: u32,
    context_ownership: u32,
    context_address_1: usize,
    context_address_2: usize,
    share_context_address: usize,
    client_api: u32,
    get_proc_address: usize,
    surface_address: usize,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::opengl_surface_descriptor_to_native(
        maplibre_core::render::OpenGLSurfaceDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            context: opengl_context_fields(
                context_platform,
                context_ownership,
                context_address_1,
                context_address_2,
                share_context_address,
                client_api,
                get_proc_address,
            )?,
            surface: surface_address as *mut c_void,
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_opengl_surface_attach_start(map, &descriptor, options, session, operation)
        },
    )
}

#[pyfunction]
#[allow(clippy::too_many_arguments)]
fn attach_opengl_owned_texture(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    context_platform: u32,
    context_ownership: u32,
    context_address_1: usize,
    context_address_2: usize,
    share_context_address: usize,
    client_api: u32,
    get_proc_address: usize,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::opengl_owned_texture_descriptor_to_native(
        maplibre_core::render::OpenGLOwnedTextureDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            context: opengl_context_fields(
                context_platform,
                context_ownership,
                context_address_1,
                context_address_2,
                share_context_address,
                client_api,
                get_proc_address,
            )?,
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_opengl_owned_texture_attach_start(
                map,
                &descriptor,
                options,
                session,
                operation,
            )
        },
    )
}

#[pyfunction]
#[allow(clippy::too_many_arguments)]
fn attach_opengl_borrowed_texture(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    physical_width: u32,
    physical_height: u32,
    context_platform: u32,
    context_ownership: u32,
    context_address_1: usize,
    context_address_2: usize,
    share_context_address: usize,
    client_api: u32,
    get_proc_address: usize,
    texture: u32,
    target: u32,
    driver: u32,
    texture_ring_depth: u32,
) -> PyResult<(RenderSessionHandle, u64)> {
    let descriptor = maplibre_core::render::opengl_borrowed_texture_descriptor_to_native(
        maplibre_core::render::OpenGLBorrowedTextureDescriptorFields {
            extent: maplibre_core::render::RenderTargetExtentFields {
                width,
                height,
                scale_factor,
            },
            physical_width,
            physical_height,
            context: opengl_context_fields(
                context_platform,
                context_ownership,
                context_address_1,
                context_address_2,
                share_context_address,
                client_api,
                get_proc_address,
            )?,
            texture,
            target,
        },
    );
    attach_render_session(
        map,
        driver,
        texture_ring_depth,
        |map, options, session, operation| unsafe {
            sys::mln_opengl_borrowed_texture_attach_start(
                map,
                &descriptor,
                options,
                session,
                operation,
            )
        },
    )
}

fn webgpu_context_fields(
    instance_address: usize,
    device_address: usize,
    queue_address: usize,
) -> maplibre_core::render::WebGpuContextDescriptorFields {
    maplibre_core::render::WebGpuContextDescriptorFields {
        instance: instance_address as *mut c_void,
        device: device_address as *mut c_void,
        queue: queue_address as *mut c_void,
    }
}

fn vulkan_context_fields(
    instance_address: usize,
    physical_device_address: usize,
    device_address: usize,
    graphics_queue_address: usize,
    graphics_queue_family_index: u32,
    get_instance_proc_addr: usize,
    get_device_proc_addr: usize,
) -> maplibre_core::render::VulkanContextDescriptorFields {
    maplibre_core::render::VulkanContextDescriptorFields {
        instance: instance_address as *mut c_void,
        physical_device: physical_device_address as *mut c_void,
        device: device_address as *mut c_void,
        graphics_queue: graphics_queue_address as *mut c_void,
        graphics_queue_family_index,
        get_instance_proc_addr: get_instance_proc_addr as *mut c_void,
        get_device_proc_addr: get_device_proc_addr as *mut c_void,
    }
}

fn opengl_context_fields(
    context_platform: u32,
    context_ownership: u32,
    context_address_1: usize,
    context_address_2: usize,
    share_context_address: usize,
    client_api: u32,
    get_proc_address: usize,
) -> PyResult<maplibre_core::render::OpenGLContextDescriptorFields> {
    match context_platform {
        sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL => {
            Ok(maplibre_core::render::OpenGLContextDescriptorFields::Wgl(
                maplibre_core::render::WglContextDescriptorFields {
                    device_context: context_address_1 as *mut c_void,
                    share_context: share_context_address as *mut c_void,
                    get_proc_address: get_proc_address as *mut c_void,
                    ownership: context_ownership,
                },
            ))
        }
        sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL => {
            Ok(maplibre_core::render::OpenGLContextDescriptorFields::Egl(
                maplibre_core::render::EglContextDescriptorFields {
                    display: context_address_1 as *mut c_void,
                    config: context_address_2 as *mut c_void,
                    share_context: share_context_address as *mut c_void,
                    client_api,
                    get_proc_address: get_proc_address as *mut c_void,
                    ownership: context_ownership,
                },
            ))
        }
        _ => Err(invalid_argument_error(format!(
            "unknown OpenGL context platform: {context_platform}"
        ))),
    }
}

/// Private PyO3 extension for the public maplibre_native_ffi package.
#[pymodule]
fn _native(module: &Bound<'_, PyModule>) -> PyResult<()> {
    module.add_class::<RuntimeHandle>()?;
    module.add_class::<MapHandle>()?;
    module.add_class::<MapProjectionHandle>()?;
    module.add_class::<GeoJsonSourceDataHandle>()?;
    module.add_class::<ResourceRequestHandle>()?;
    module.add_class::<LogReceiver>()?;
    module.add_class::<CustomGeometrySourceHandle>()?;
    module.add_class::<RenderSessionHandle>()?;
    module.add_class::<MetalOwnedTextureFrameHandle>()?;
    module.add_class::<VulkanOwnedTextureFrameHandle>()?;
    module.add_class::<OpenGLOwnedTextureFrameHandle>()?;
    module.add_class::<WebGPUOwnedTextureFrameHandle>()?;
    module.add_function(wrap_pyfunction!(expected_c_abi_version, module)?)?;
    module.add_function(wrap_pyfunction!(c_version, module)?)?;
    module.add_function(wrap_pyfunction!(supported_render_backends_raw, module)?)?;
    module.add_function(wrap_pyfunction!(
        supported_opengl_context_providers_raw,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        render_target_extent_physical_size,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(network_status_raw, module)?)?;
    module.add_function(wrap_pyfunction!(create_geojson_source_data, module)?)?;
    module.add_function(wrap_pyfunction!(projected_meters_for_lat_lng, module)?)?;
    module.add_function(wrap_pyfunction!(lat_lng_for_projected_meters, module)?)?;
    module.add_function(wrap_pyfunction!(set_network_status_raw, module)?)?;
    module.add_function(wrap_pyfunction!(set_log_callback, module)?)?;
    module.add_function(wrap_pyfunction!(clear_log_callback, module)?)?;
    module.add_function(wrap_pyfunction!(set_async_log_severity_mask, module)?)?;
    module.add_function(wrap_pyfunction!(
        set_network_status_raw_unchecked_for_test,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(map_size_by_id_for_test, module)?)?;
    module.add_function(wrap_pyfunction!(status_error_for_test, module)?)?;
    module.add_function(wrap_pyfunction!(
        status_error_after_support_call_for_test,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        create_runtime_with_abi_version_for_test,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(
        synthetic_runtime_event_batch_for_test,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(runtime_event_stride_for_test, module)?)?;
    module.add_function(wrap_pyfunction!(
        runtime_options_default_event_mask,
        module
    )?)?;
    module.add_function(wrap_pyfunction!(map_options_default_event_mask, module)?)?;
    module.add_function(wrap_pyfunction!(create_runtime, module)?)?;
    module.add_function(wrap_pyfunction!(create_map, module)?)?;
    module.add_function(wrap_pyfunction!(attach_metal_surface, module)?)?;
    module.add_function(wrap_pyfunction!(attach_vulkan_surface, module)?)?;
    module.add_function(wrap_pyfunction!(attach_metal_owned_texture, module)?)?;
    module.add_function(wrap_pyfunction!(attach_metal_borrowed_texture, module)?)?;
    module.add_function(wrap_pyfunction!(attach_vulkan_owned_texture, module)?)?;
    module.add_function(wrap_pyfunction!(attach_vulkan_borrowed_texture, module)?)?;
    module.add_function(wrap_pyfunction!(attach_opengl_surface, module)?)?;
    module.add_function(wrap_pyfunction!(attach_opengl_owned_texture, module)?)?;
    module.add_function(wrap_pyfunction!(attach_opengl_borrowed_texture, module)?)?;
    module.add_function(wrap_pyfunction!(attach_webgpu_surface, module)?)?;
    module.add_function(wrap_pyfunction!(attach_webgpu_owned_texture, module)?)?;
    module.add_function(wrap_pyfunction!(attach_webgpu_borrowed_texture, module)?)?;
    Ok(())
}
