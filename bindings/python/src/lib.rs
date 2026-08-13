#![deny(unsafe_op_in_unsafe_fn)]

use maplibre_native_ffi_core::{
    self as maplibre_core, Error, ErrorKind, LogEvent, LogSeverity, NetworkStatus, RenderMode,
    ResourceErrorReason, ResourceResponseStatus, RuntimeEventPayload, RuntimeEventType,
    TileOperation,
};
use maplibre_native_ffi_sys as sys;
use pyo3::buffer::PyBuffer;
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
    pyo3::import_exception!(maplibre_native_ffi.errors, InvalidArgumentError);
    pyo3::import_exception!(maplibre_native_ffi.errors, _OperationResultConsumedError);
    pyo3::import_exception!(maplibre_native_ffi.errors, InvalidStateError);
    pyo3::import_exception!(maplibre_native_ffi.errors, NativeError);
    pyo3::import_exception!(maplibre_native_ffi.errors, UnknownStatusError);
    pyo3::import_exception!(maplibre_native_ffi.errors, UnsupportedFeatureError);
    pyo3::import_exception!(maplibre_native_ffi.errors, WrongThreadError);
}

#[pyclass(name = "_RuntimeHandle")]
struct RuntimeHandle {
    state: Mutex<maplibre_core::handle::NativeHandleState<sys::mln_runtime>>,
    notification_source: Mutex<sys::mln_notification_source>,
    operation_gate: RuntimeOperationGate,
    resource_provider: Mutex<Option<Box<PyResourceProviderState>>>,
    resource_transform: Mutex<Option<Box<PyResourceTransformState>>>,
    http_header_transform: Mutex<Option<Box<PyHttpHeaderTransformState>>>,
}

#[derive(Debug)]
struct RuntimeOperationGate {
    state: Mutex<RuntimeOperationGateState>,
}

#[derive(Debug, Default)]
struct RuntimeOperationGateState {
    active_detached_operation: bool,
    closing: bool,
    closed: bool,
}

struct RuntimeDetachedOperationGuard<'a> {
    gate: &'a RuntimeOperationGate,
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

// The C API accepts signals and destruction from any thread, so this pyclass is
// `Send + Sync` without an unsafe assertion. The mutex makes close once-only
// against a concurrent signal.
#[pyclass(name = "_WakeSource")]
struct WakeSource {
    handle: Mutex<Option<sys::mln_wake_source>>,
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
    detached: bool,
    frame_acquired: bool,
}

#[pyclass(name = "_RenderSessionHandle")]
struct RenderSessionHandle {
    state: Arc<Mutex<RenderSessionState>>,
}

#[pyclass(name = "_DetachedRenderSessionHandle")]
struct DetachedRenderSessionHandle {
    state: Arc<Mutex<RenderSessionState>>,
}

#[derive(Debug, Clone, Copy)]
struct MetalOwnedTextureFrameRaw {
    generation: u64,
    width: u32,
    height: u32,
    scale_factor: f64,
    frame_id: u64,
    texture_address: usize,
    device_address: usize,
    pixel_format: u64,
}

#[pyclass(name = "_MetalOwnedTextureFrameHandle")]
struct MetalOwnedTextureFrameHandle {
    session: Arc<Mutex<RenderSessionState>>,
    raw: MetalOwnedTextureFrameRaw,
    closed: Mutex<bool>,
}

#[derive(Debug, Clone, Copy)]
struct VulkanOwnedTextureFrameRaw {
    generation: u64,
    width: u32,
    height: u32,
    scale_factor: f64,
    frame_id: u64,
    image_address: usize,
    image_view_address: usize,
    device_address: usize,
    format: u32,
    layout: u32,
}

#[pyclass(name = "_VulkanOwnedTextureFrameHandle")]
struct VulkanOwnedTextureFrameHandle {
    session: Arc<Mutex<RenderSessionState>>,
    raw: VulkanOwnedTextureFrameRaw,
    closed: Mutex<bool>,
}

#[derive(Debug, Clone, Copy)]
struct OpenGLOwnedTextureFrameRaw {
    generation: u64,
    width: u32,
    height: u32,
    scale_factor: f64,
    frame_id: u64,
    texture: u32,
    target: u32,
    internal_format: u32,
    format: u32,
    type_: u32,
}

enum OwnedTextureFrameRelease {
    Metal(MetalOwnedTextureFrameRaw),
    Vulkan(VulkanOwnedTextureFrameRaw),
    OpenGL(OpenGLOwnedTextureFrameRaw),
}

struct OwnedTextureFrameAcquisitionGuard {
    session: Arc<Mutex<RenderSessionState>>,
    frame: Option<OwnedTextureFrameRelease>,
}

#[pyclass(name = "_OpenGLOwnedTextureFrameHandle")]
struct OpenGLOwnedTextureFrameHandle {
    session: Arc<Mutex<RenderSessionState>>,
    raw: OpenGLOwnedTextureFrameRaw,
    closed: Mutex<bool>,
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

    fn begin_detached_operation(&self) -> PyResult<RuntimeDetachedOperationGuard<'_>> {
        let mut state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if state.closed {
            return Err(invalid_state_error("runtime handle is closed"));
        }
        if state.closing {
            return Err(invalid_state_error("runtime is closing"));
        }
        if state.active_detached_operation {
            return Err(invalid_state_error(
                "runtime has an active native operation",
            ));
        }
        state.active_detached_operation = true;
        Ok(RuntimeDetachedOperationGuard { gate: self })
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
        if state.active_detached_operation {
            return Err(invalid_state_error(
                "runtime has an active native operation",
            ));
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

impl Drop for RuntimeDetachedOperationGuard<'_> {
    fn drop(&mut self) {
        self.gate
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .active_detached_operation = false;
    }
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
            leak_optional_box(&self.resource_provider);
            leak_optional_box(&self.resource_transform);
            leak_optional_box(&self.http_header_transform);
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
        ) -> sys::mln_status,
    ) -> PyResult<()> {
        let state = self.state();
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
        // SAFETY: The C API validates the map pointer, string views, and options.
        maplibre_core::check(unsafe {
            add(state.handle(), source_id.raw(), url.raw(), options.as_ptr())
        })
        .map_err(map_error)
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
        ) -> sys::mln_status,
    ) -> PyResult<()> {
        let state = self.state();
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
        // SAFETY: The C API validates the map pointer, source ID, tile URL views, and options.
        maplibre_core::check(unsafe {
            add(
                state.handle(),
                source_id.raw(),
                tiles.as_ptr(),
                tiles.len(),
                options.as_ptr(),
            )
        })
        .map_err(map_error)
    }

    fn string_bool_call_with(
        &self,
        value: String,
        call: unsafe extern "C" fn(
            sys::mln_map,
            sys::mln_buffer_view,
            *mut bool,
        ) -> sys::mln_status,
    ) -> PyResult<bool> {
        let state = self.state();
        let value = maplibre_core::string::string_view(&value);
        let mut out = false;
        // SAFETY: The C API validates the map pointer, borrowed string view, and out pointer.
        maplibre_core::check(unsafe { call(state.handle(), value.raw(), &mut out) })
            .map_err(map_error)?;
        Ok(out)
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
/// The C API calls this once on the map owner thread, whether the source was
/// removed explicitly, dropped by a style load, or retired with the map, so the
/// binding never watches style loads to find its own detached sources.
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
        Ok(Self {
            handle,
            detached: false,
            frame_acquired: false,
        })
    }

    fn native(&self) -> sys::mln_render_session {
        self.handle.handle()
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn ensure_no_frame_acquired(&self) -> PyResult<()> {
        if self.frame_acquired {
            Err(invalid_state_error(
                "render session has an acquired texture frame",
            ))
        } else {
            Ok(())
        }
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
        let runtime_handle = {
            let state = self.state();
            let runtime_handle = state.live_handle();
            if runtime_handle.is_some() {
                state.mark_closed();
            }
            runtime_handle
        };
        if let Some(runtime_handle) = runtime_handle {
            // SAFETY: state owns an mln_runtime handle created by
            // mln_runtime_create and pairs it with the matching destroy
            // function. Destroy can wait for in-flight callbacks, so it runs
            // without the GIL or the state mutex.
            let status = py.detach(move || unsafe { sys::mln_runtime_destroy(runtime_handle) });
            if let Err(error) = maplibre_core::check(status) {
                self.state().restore_handle_for_retry(runtime_handle);
                self.operation_gate.finish_failed_close();
                return Err(map_error(error));
            }
        }
        self.resource_provider
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        self.resource_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        self.http_header_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        let source = std::mem::replace(
            &mut *self
                .notification_source
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner()),
            sys::mln_notification_source(0),
        );
        if source.0 != 0 {
            if let Err(error) =
                maplibre_core::check(unsafe { sys::mln_notification_source_close(source) })
            {
                *self
                    .notification_source
                    .lock()
                    .unwrap_or_else(|poisoned| poisoned.into_inner()) = source;
                self.operation_gate.finish_failed_close();
                return Err(map_error(error));
            }
        }
        self.operation_gate.finish_successful_close();
        Ok(())
    }

    fn pump(&self, py: Python<'_>, timeout_ms: i64) -> PyResult<()> {
        let _operation = self.operation_gate.begin_detached_operation()?;
        let runtime_handle = {
            let state = self.state_for_operation()?;
            let Some(runtime_handle) = state.live_handle() else {
                return Err(invalid_state_error("runtime handle is closed"));
            };
            runtime_handle
        };
        // SAFETY: runtime_handle passed the binding lifecycle gate, and the C
        // API validates that it is live and on the owner thread. The call parks,
        // so it runs without the GIL or the state mutex: another Python thread
        // signalling a wake source is what ends the park.
        let status = py.detach(|| unsafe { sys::mln_runtime_pump(runtime_handle, timeout_ms) });
        maplibre_core::check(status).map_err(map_error)
    }

    fn wake_source(&self) -> PyResult<WakeSource> {
        let state = self.state_for_operation()?;
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_wake_source>::new();
        // SAFETY: The C API validates that the pointer is a live runtime handle
        // and that the call occurs on the runtime owner thread, and out is a
        // null-initialized out-pointer owned by this call.
        maplibre_core::check(unsafe {
            sys::mln_runtime_wake_source_acquire(state.handle(), out.as_mut_ptr())
        })
        .map_err(map_error)?;
        let source = out.into_live("mln_wake_source").map_err(map_error)?;
        Ok(WakeSource {
            handle: Mutex::new(Some(source)),
        })
    }

    fn run_ambient_cache_operation_start(&self, ambient_operation: u32) -> PyResult<u64> {
        let state = self.state_for_operation()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: The C API validates the runtime handle, operation enum value,
        // owner-thread affinity, and writable operation pointer.
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
        // SAFETY: The C API validates the runtime handle, owner-thread
        // affinity, and writable operation pointer.
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

    fn operation_discard(&self, operation_id: u64) -> PyResult<()> {
        maplibre_core::check(unsafe {
            sys::mln_operation_discard_result(sys::mln_operation(operation_id))
        })
        .map_err(map_error)
    }

    fn set_resource_provider(
        &self,
        py: Python<'_>,
        callback: Py<PyAny>,
        max_pending_callbacks: usize,
    ) -> PyResult<()> {
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
        let _operation = self.operation_gate.begin_detached_operation()?;
        let runtime_handle = {
            let state = self.state_for_operation()?;
            let Some(runtime_handle) = state.live_handle() else {
                return Err(invalid_state_error("runtime handle is closed"));
            };
            runtime_handle
        };
        let callback = descriptor.callback;
        let user_data_address = descriptor.user_data as usize;
        let size = descriptor.size;
        // SAFETY: runtime_handle passed the binding lifecycle gate and the C API
        // validates that it is live. descriptor points to replacement state,
        // retained after a successful registration. Replacement can wait for
        // in-flight callbacks that need the GIL, so it runs without the GIL or
        // the state mutex.
        let status = py.detach(move || {
            let descriptor = sys::mln_resource_provider {
                size,
                callback,
                user_data: user_data_address as *mut c_void,
            };
            unsafe { sys::mln_runtime_set_resource_provider(runtime_handle, &descriptor) }
        });
        maplibre_core::check(status).map_err(map_error)?;
        // Native retired the previous provider before returning, so dropping
        // the state it replaces here can no longer be reached from native.
        self.resource_provider
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .replace(replacement);
        Ok(())
    }

    fn clear_resource_provider(&self, py: Python<'_>) -> PyResult<()> {
        let _operation = self.operation_gate.begin_detached_operation()?;
        let runtime_handle = {
            let state = self.state_for_operation()?;
            let Some(runtime_handle) = state.live_handle() else {
                return Err(invalid_state_error("runtime handle is closed"));
            };
            runtime_handle
        };
        // SAFETY: runtime_handle passed the binding lifecycle gate and the C API
        // validates that it is live. The call waits for in-flight callbacks that
        // need the GIL, so it runs without the GIL or the state mutex.
        let status =
            py.detach(move || unsafe { sys::mln_runtime_clear_resource_provider(runtime_handle) });
        maplibre_core::check(status).map_err(map_error)?;
        // Native can no longer reach the cleared provider state, so drop it.
        self.resource_provider
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        Ok(())
    }

    fn set_resource_transform(
        &self,
        py: Python<'_>,
        callback: Py<PyAny>,
        max_pending_callbacks: usize,
    ) -> PyResult<()> {
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
        let _operation = self.operation_gate.begin_detached_operation()?;
        let runtime_handle = {
            let state = self.state_for_operation()?;
            let Some(runtime_handle) = state.live_handle() else {
                return Err(invalid_state_error("runtime handle is closed"));
            };
            runtime_handle
        };
        let callback = descriptor.callback;
        let user_data_address = descriptor.user_data as usize;
        let size = descriptor.size;
        // SAFETY: runtime_handle passed the binding lifecycle gate and the C API
        // validates that it is live. descriptor points to replacement state,
        // retained after a successful registration. Replacement can wait for
        // in-flight callbacks, so it runs without the GIL or the state mutex.
        let status = py.detach(move || {
            let descriptor = sys::mln_resource_transform {
                size,
                callback,
                user_data: user_data_address as *mut c_void,
            };
            unsafe { sys::mln_runtime_set_resource_transform(runtime_handle, &descriptor) }
        });
        maplibre_core::check(status).map_err(map_error)?;
        self.resource_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .replace(replacement);
        Ok(())
    }

    fn clear_resource_transform(&self, py: Python<'_>) -> PyResult<()> {
        let _operation = self.operation_gate.begin_detached_operation()?;
        let runtime_handle = {
            let state = self.state_for_operation()?;
            let Some(runtime_handle) = state.live_handle() else {
                return Err(invalid_state_error("runtime handle is closed"));
            };
            runtime_handle
        };
        // SAFETY: runtime_handle passed the binding lifecycle gate and the C API
        // validates that it is live. The call waits for in-flight callbacks, so
        // it runs without the GIL or the state mutex.
        let status =
            py.detach(move || unsafe { sys::mln_runtime_clear_resource_transform(runtime_handle) });
        maplibre_core::check(status).map_err(map_error)?;
        self.resource_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        Ok(())
    }

    fn set_http_header_transform(
        &self,
        py: Python<'_>,
        callback: Py<PyAny>,
        max_pending_callbacks: usize,
    ) -> PyResult<()> {
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
        let _operation = self.operation_gate.begin_detached_operation()?;
        let runtime_handle = self
            .state_for_operation()?
            .live_handle()
            .ok_or_else(|| invalid_state_error("runtime handle is closed"))?;
        let callback = descriptor.callback;
        let user_data_address = descriptor.user_data as usize;
        let size = descriptor.size;
        let status = py.detach(move || {
            let descriptor = sys::mln_http_header_transform {
                size,
                callback,
                user_data: user_data_address as *mut c_void,
            };
            // SAFETY: runtime is live and replacement remains retained on success.
            unsafe { sys::mln_runtime_set_http_header_transform(runtime_handle, &descriptor) }
        });
        maplibre_core::check(status).map_err(map_error)?;
        self.http_header_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .replace(replacement);
        Ok(())
    }

    fn clear_http_header_transform(&self, py: Python<'_>) -> PyResult<()> {
        let _operation = self.operation_gate.begin_detached_operation()?;
        let runtime_handle = self
            .state_for_operation()?
            .live_handle()
            .ok_or_else(|| invalid_state_error("runtime handle is closed"))?;
        let status = py.detach(move || {
            // SAFETY: runtime is live; native waits for callback retirement.
            unsafe { sys::mln_runtime_clear_http_header_transform(runtime_handle) }
        });
        maplibre_core::check(status).map_err(map_error)?;
        self.http_header_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        Ok(())
    }

    fn drain_events(&self, py: Python<'_>, max_events: usize) -> PyResult<Py<PyAny>> {
        let state = self.state_for_operation()?;
        let mut batch = sys::mln_event_batch(0);
        maplibre_core::check(unsafe {
            sys::mln_runtime_drain_events(state.handle(), max_events, &mut batch)
        })
        .map_err(map_error)?;
        let result = (|| {
            let mut view: sys::mln_runtime_event_batch_view = unsafe { std::mem::zeroed() };
            view.size = std::mem::size_of::<sys::mln_runtime_event_batch_view>() as u32;
            maplibre_core::check(unsafe { sys::mln_event_batch_get(batch, &mut view) })
                .map_err(map_error)?;
            let mut copied = Vec::with_capacity(view.event_count);
            for event in unsafe { maplibre_core::events::drain_batch(&view) } {
                copied.push(event.map_err(map_error)?);
            }
            event_batch_to_py(py, copied, view.remaining_count)
        })();
        unsafe { sys::mln_event_batch_release(batch) };
        result
    }

    fn set_event_mask(&self, mask: u64) -> PyResult<()> {
        let state = self.state_for_operation()?;
        // SAFETY: The C API validates the runtime handle, owner-thread
        // affinity, and the mask bits.
        maplibre_core::check(unsafe { sys::mln_runtime_set_event_mask(state.handle(), mask) })
            .map_err(map_error)
    }

    fn get_event_mask(&self) -> PyResult<u64> {
        let state = self.state_for_operation()?;
        let mut mask = 0u64;
        // SAFETY: The C API validates the runtime handle and owner-thread
        // affinity, and mask points to writable storage for one u64.
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
        // SAFETY: state owns an mln_map handle created by mln_map_create and
        // pairs it with the matching status-returning destroy function.
        unsafe { state.close_status(sys::mln_map_destroy) }.map_err(map_error)?;
        Ok(())
    }

    /// This map's native handle, which names one map for the life of the
    /// process. It carries no ownership and cannot operate on the map.
    fn id(&self) -> PyResult<u64> {
        let state = self.state();
        Ok(state.handle().0)
    }

    fn request_repaint(&self) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates that the handle is live and
        // that the call occurs on the map owner thread.
        maplibre_core::check(unsafe { sys::mln_map_request_repaint(state.handle()) })
            .map_err(map_error)
    }

    fn request_still_image(&self) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates that the handle is live and
        // that the call occurs on the map owner thread.
        maplibre_core::check(unsafe { sys::mln_map_request_still_image(state.handle()) })
            .map_err(map_error)
    }

    fn dump_debug_logs(&self) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates that the handle is live and
        // that the call occurs on the map owner thread.
        maplibre_core::check(unsafe { sys::mln_map_dump_debug_logs(state.handle()) })
            .map_err(map_error)
    }

    fn set_event_mask(&self, mask: u64) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates the map pointer, owner-thread affinity,
        // and the mask bits.
        maplibre_core::check(unsafe { sys::mln_map_set_event_mask(state.handle(), mask) })
            .map_err(map_error)
    }

    fn get_event_mask(&self) -> PyResult<u64> {
        let state = self.state();
        let mut mask = 0u64;
        // SAFETY: The C API validates the map pointer and owner-thread affinity,
        // and mask points to writable storage for one u64.
        maplibre_core::check(unsafe { sys::mln_map_get_event_mask(state.handle(), &mut mask) })
            .map_err(map_error)?;
        Ok(mask)
    }

    fn set_debug_options(&self, options: u32) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates the map pointer, owner-thread affinity,
        // and debug option mask bits.
        maplibre_core::check(unsafe { sys::mln_map_set_debug_options(state.handle(), options) })
            .map_err(map_error)
    }

    fn get_debug_options(&self) -> PyResult<u32> {
        let state = self.state();
        let mut options = 0;
        // SAFETY: The C API validates the map pointer and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_debug_options(state.handle(), &mut options)
        })
        .map_err(map_error)?;
        Ok(options)
    }

    fn set_rendering_stats_view_enabled(&self, enabled: bool) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates the map pointer and owner-thread affinity.
        maplibre_core::check(unsafe {
            sys::mln_map_set_rendering_stats_view_enabled(state.handle(), enabled)
        })
        .map_err(map_error)
    }

    fn get_rendering_stats_view_enabled(&self) -> PyResult<bool> {
        let state = self.state();
        let mut enabled = false;
        // SAFETY: The C API validates the map pointer and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_rendering_stats_view_enabled(state.handle(), &mut enabled)
        })
        .map_err(map_error)?;
        Ok(enabled)
    }

    fn is_fully_loaded(&self) -> PyResult<bool> {
        let state = self.state();
        let mut loaded = false;
        // SAFETY: The C API validates the map pointer and out pointer.
        maplibre_core::check(unsafe { sys::mln_map_is_fully_loaded(state.handle(), &mut loaded) })
            .map_err(map_error)?;
        Ok(loaded)
    }

    fn get_size(&self) -> PyResult<(u32, u32, f64)> {
        let state = self.state();
        let mut width = 0u32;
        let mut height = 0u32;
        let mut scale_factor = 0f64;
        // SAFETY: The C API validates the map pointer and out pointers.
        maplibre_core::check(unsafe {
            sys::mln_map_get_size(state.handle(), &mut width, &mut height, &mut scale_factor)
        })
        .map_err(map_error)?;
        Ok((width, height, scale_factor))
    }

    fn get_viewport_options(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut options = unsafe { sys::mln_map_viewport_options_default() };
        // SAFETY: The C API validates the map pointer and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_viewport_options(state.handle(), &mut options)
        })
        .map_err(map_error)?;
        viewport_options_to_py(py, &options)
    }

    fn set_viewport_options(
        &self,
        north_orientation: Option<u32>,
        constrain_mode: Option<u32>,
        viewport_mode: Option<u32>,
        frustum_offset: Option<(f64, f64, f64, f64)>,
    ) -> PyResult<()> {
        let state = self.state();
        let options = viewport_options_from_parts(
            north_orientation,
            constrain_mode,
            viewport_mode,
            frustum_offset,
        );
        // SAFETY: The C API validates the map pointer and viewport options.
        maplibre_core::check(unsafe { sys::mln_map_set_viewport_options(state.handle(), &options) })
            .map_err(map_error)
    }

    fn get_tile_options(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut options = unsafe { sys::mln_map_tile_options_default() };
        // SAFETY: The C API validates the map pointer and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_tile_options(state.handle(), &mut options)
        })
        .map_err(map_error)?;
        tile_options_to_py(py, &options)
    }

    fn set_tile_options(
        &self,
        prefetch_zoom_delta: Option<u32>,
        lod_min_radius: Option<f64>,
        lod_scale: Option<f64>,
        lod_pitch_threshold: Option<f64>,
        lod_zoom_shift: Option<f64>,
        lod_mode: Option<u32>,
    ) -> PyResult<()> {
        let state = self.state();
        let options = tile_options_from_parts(
            prefetch_zoom_delta,
            lod_min_radius,
            lod_scale,
            lod_pitch_threshold,
            lod_zoom_shift,
            lod_mode,
        );
        // SAFETY: The C API validates the map pointer and tile options.
        maplibre_core::check(unsafe { sys::mln_map_set_tile_options(state.handle(), &options) })
            .map_err(map_error)
    }

    fn create_projection(&self) -> PyResult<MapProjectionHandle> {
        let state = self.state();
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_map_projection>::new();
        // SAFETY: The C API validates the map handle, owner-thread affinity, and
        // output pointer. out starts null and is consumed immediately on success.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_create(state.handle(), out.as_mut_ptr())
        })
        .map_err(map_error)?;
        let native = out.into_live("mln_map_projection").map_err(map_error)?;
        // SAFETY: ptr came from mln_map_projection_create and is paired with
        // mln_map_projection_destroy in close.
        let handle = unsafe {
            maplibre_core::handle::NativeHandleState::from_handle(native, "mln_map_projection")
        }
        .map_err(map_error)?;
        Ok(MapProjectionHandle {
            state: Mutex::new(handle),
        })
    }

    fn set_style_url(&self, url: String) -> PyResult<()> {
        let state = self.state();
        let url = maplibre_core::string::c_string(&url).map_err(map_error)?;
        // SAFETY: The C API validates that the pointer is a live map handle.
        // url is a null-terminated C string whose storage lives for this call.
        maplibre_core::check(unsafe { sys::mln_map_set_style_url(state.handle(), url.as_ptr()) })
            .map_err(map_error)?;
        Ok(())
    }

    fn set_style_json(&self, json: &Bound<'_, PyBytes>) -> PyResult<()> {
        let state = self.state();
        let json = maplibre_core::string::buffer_view(json.as_bytes());
        // SAFETY: The C API validates that the pointer is a live map handle.
        // json is a null-terminated C string whose storage lives for this call.
        maplibre_core::check(unsafe { sys::mln_map_set_style_json(state.handle(), json) })
            .map_err(map_error)?;
        Ok(())
    }

    fn copy_loaded_style_json(&self, py: Python<'_>) -> PyResult<Py<PyBytes>> {
        let state = self.state();
        // SAFETY: The C API validates the map pointer and each buffer and out
        // pointer it is given.
        unsafe {
            copy_bytes(py, |text, capacity, out_size| {
                sys::mln_map_copy_loaded_style_json(state.handle(), text, capacity, out_size)
            })
        }
    }

    fn copy_style_url(&self) -> PyResult<String> {
        let state = self.state();
        // SAFETY: The C API validates the map pointer and each buffer and out
        // pointer it is given.
        unsafe {
            copy_text(|text, capacity, out_size| {
                sys::mln_map_copy_style_url(state.handle(), text, capacity, out_size)
            })
        }
    }

    fn get_camera(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: The C API validates that the handle is live and
        // camera points to initialized writable storage.
        maplibre_core::check(unsafe { sys::mln_map_get_camera(state.handle(), &mut camera) })
            .map_err(map_error)?;
        camera_options_to_py(py, &camera)
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
    ) -> PyResult<()> {
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
        // SAFETY: The C API validates the map pointer and camera fields.
        maplibre_core::check(unsafe { sys::mln_map_jump_to(state.handle(), &camera) })
            .map_err(map_error)
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
    ) -> PyResult<()> {
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
        let animation = animation.map(animation_options_from_parts);
        // SAFETY: The C API validates the map pointer, camera fields, and
        // optional animation fields.
        maplibre_core::check(unsafe {
            sys::mln_map_ease_to(
                state.handle(),
                &camera,
                optional_ref_ptr(animation.as_ref()),
            )
        })
        .map_err(map_error)
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
    ) -> PyResult<()> {
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
        let animation = animation.map(animation_options_from_parts);
        // SAFETY: The C API validates the map pointer, camera fields, and
        // optional animation fields.
        maplibre_core::check(unsafe {
            sys::mln_map_fly_to(
                state.handle(),
                &camera,
                optional_ref_ptr(animation.as_ref()),
            )
        })
        .map_err(map_error)
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
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: The C API validates the map pointer, bounds, fit options, and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_lat_lng_bounds(state.handle(), bounds, &fit, &mut camera)
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
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: The C API validates the map pointer, coordinate slice, fit options, and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_lat_lngs(
                state.handle(),
                coordinates.as_ptr(),
                coordinates.len(),
                &fit,
                &mut camera,
            )
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
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: The C API validates the map pointer, geometry, fit options, and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_camera_for_geometry(state.handle(), geometry, &fit, &mut camera)
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
        let mut bounds = empty_lat_lng_bounds();
        let status = if unwrapped {
            // SAFETY: The C API validates the map pointer, camera options, and output pointer.
            unsafe {
                sys::mln_map_lat_lng_bounds_for_camera_unwrapped(
                    state.handle(),
                    &camera,
                    &mut bounds,
                )
            }
        } else {
            // SAFETY: The C API validates the map pointer, camera options, and output pointer.
            unsafe { sys::mln_map_lat_lng_bounds_for_camera(state.handle(), &camera, &mut bounds) }
        };
        maplibre_core::check(status).map_err(map_error)?;
        lat_lng_bounds_to_py(py, bounds)
    }

    fn get_bounds(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut bounds = unsafe { sys::mln_bound_options_default() };
        // SAFETY: The C API validates the map pointer and output pointer.
        maplibre_core::check(unsafe { sys::mln_map_get_bounds(state.handle(), &mut bounds) })
            .map_err(map_error)?;
        bound_options_to_py(py, &bounds)
    }

    fn set_bounds(
        &self,
        bounds: Option<((f64, f64), (f64, f64))>,
        unbounded: bool,
        min_zoom: Option<f64>,
        max_zoom: Option<f64>,
        min_pitch: Option<f64>,
        max_pitch: Option<f64>,
    ) -> PyResult<()> {
        let state = self.state();
        let bounds =
            bound_options_from_parts(bounds, unbounded, min_zoom, max_zoom, min_pitch, max_pitch);
        // SAFETY: The C API validates the map pointer and bounds fields.
        maplibre_core::check(unsafe { sys::mln_map_set_bounds(state.handle(), &bounds) })
            .map_err(map_error)
    }

    fn move_by(&self, delta_x: f64, delta_y: f64) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates the map pointer and delta values.
        maplibre_core::check(unsafe { sys::mln_map_move_by(state.handle(), delta_x, delta_y) })
            .map_err(map_error)
    }

    fn move_by_animated(
        &self,
        delta_x: f64,
        delta_y: f64,
        animation: Option<AnimationParts>,
    ) -> PyResult<()> {
        let state = self.state();
        let animation = animation.map(animation_options_from_parts);
        // SAFETY: The C API validates the map pointer, delta values, and
        // optional animation fields.
        maplibre_core::check(unsafe {
            sys::mln_map_move_by_animated(
                state.handle(),
                delta_x,
                delta_y,
                optional_ref_ptr(animation.as_ref()),
            )
        })
        .map_err(map_error)
    }

    fn scale_by(&self, scale: f64, anchor: Option<(f64, f64)>) -> PyResult<()> {
        let state = self.state();
        let anchor = anchor.map(screen_point_from_tuple);
        // SAFETY: The C API validates the map pointer, scale, and optional anchor.
        maplibre_core::check(unsafe {
            sys::mln_map_scale_by(state.handle(), scale, optional_ref_ptr(anchor.as_ref()))
        })
        .map_err(map_error)
    }

    fn scale_by_animated(
        &self,
        scale: f64,
        anchor: Option<(f64, f64)>,
        animation: Option<AnimationParts>,
    ) -> PyResult<()> {
        let state = self.state();
        let anchor = anchor.map(screen_point_from_tuple);
        let animation = animation.map(animation_options_from_parts);
        // SAFETY: The C API validates the map pointer, scale, optional anchor,
        // and optional animation fields.
        maplibre_core::check(unsafe {
            sys::mln_map_scale_by_animated(
                state.handle(),
                scale,
                optional_ref_ptr(anchor.as_ref()),
                optional_ref_ptr(animation.as_ref()),
            )
        })
        .map_err(map_error)
    }

    fn rotate_by(&self, first: (f64, f64), second: (f64, f64)) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates the map pointer and points.
        maplibre_core::check(unsafe {
            sys::mln_map_rotate_by(
                state.handle(),
                screen_point_from_tuple(first),
                screen_point_from_tuple(second),
            )
        })
        .map_err(map_error)
    }

    fn rotate_by_animated(
        &self,
        first: (f64, f64),
        second: (f64, f64),
        animation: Option<AnimationParts>,
    ) -> PyResult<()> {
        let state = self.state();
        let animation = animation.map(animation_options_from_parts);
        // SAFETY: The C API validates the map pointer, points, and optional
        // animation fields.
        maplibre_core::check(unsafe {
            sys::mln_map_rotate_by_animated(
                state.handle(),
                screen_point_from_tuple(first),
                screen_point_from_tuple(second),
                optional_ref_ptr(animation.as_ref()),
            )
        })
        .map_err(map_error)
    }

    fn pitch_by(&self, pitch: f64) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates the map pointer and pitch value.
        maplibre_core::check(unsafe { sys::mln_map_pitch_by(state.handle(), pitch) })
            .map_err(map_error)
    }

    fn pitch_by_animated(&self, pitch: f64, animation: Option<AnimationParts>) -> PyResult<()> {
        let state = self.state();
        let animation = animation.map(animation_options_from_parts);
        // SAFETY: The C API validates the map pointer, pitch value, and optional
        // animation fields.
        maplibre_core::check(unsafe {
            sys::mln_map_pitch_by_animated(
                state.handle(),
                pitch,
                optional_ref_ptr(animation.as_ref()),
            )
        })
        .map_err(map_error)
    }

    fn cancel_transitions(&self) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates the map pointer.
        maplibre_core::check(unsafe { sys::mln_map_cancel_transitions(state.handle()) })
            .map_err(map_error)
    }

    fn set_gesture_in_progress(&self, in_progress: bool) -> PyResult<()> {
        let state = self.state();
        // SAFETY: The C API validates the map pointer and owner-thread affinity.
        maplibre_core::check(unsafe {
            sys::mln_map_set_gesture_in_progress(state.handle(), in_progress)
        })
        .map_err(map_error)
    }

    fn is_gesture_in_progress(&self) -> PyResult<bool> {
        let state = self.state();
        let mut in_progress = false;
        // SAFETY: The C API validates the map pointer and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_is_gesture_in_progress(state.handle(), &mut in_progress)
        })
        .map_err(map_error)?;
        Ok(in_progress)
    }

    fn get_free_camera_options(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut options = unsafe { sys::mln_free_camera_options_default() };
        // SAFETY: The C API validates the map pointer and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_free_camera_options(state.handle(), &mut options)
        })
        .map_err(map_error)?;
        free_camera_options_to_py(py, &options)
    }

    fn set_free_camera_options(
        &self,
        position: Option<(f64, f64, f64)>,
        orientation: Option<(f64, f64, f64, f64)>,
    ) -> PyResult<()> {
        let state = self.state();
        let options = free_camera_options_from_parts(position, orientation);
        // SAFETY: The C API validates the map pointer and free camera fields.
        maplibre_core::check(unsafe {
            sys::mln_map_set_free_camera_options(state.handle(), &options)
        })
        .map_err(map_error)
    }

    fn get_projection_mode(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut mode = unsafe { sys::mln_projection_mode_default() };
        // SAFETY: The C API validates the map pointer and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_projection_mode(state.handle(), &mut mode)
        })
        .map_err(map_error)?;
        projection_mode_to_py(py, &mode)
    }

    fn set_projection_mode(
        &self,
        axonometric: Option<bool>,
        x_skew: Option<f64>,
        y_skew: Option<f64>,
    ) -> PyResult<()> {
        let state = self.state();
        let mode = projection_mode_from_parts(axonometric, x_skew, y_skew);
        // SAFETY: The C API validates the map pointer and projection mode fields.
        maplibre_core::check(unsafe { sys::mln_map_set_projection_mode(state.handle(), &mode) })
            .map_err(map_error)
    }

    fn pixel_for_lat_lng(
        &self,
        py: Python<'_>,
        latitude: f64,
        longitude: f64,
    ) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let mut point = sys::mln_screen_point { x: 0.0, y: 0.0 };
        // SAFETY: The C API validates the map pointer, coordinate, and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_pixel_for_lat_lng(
                state.handle(),
                sys::mln_lat_lng {
                    latitude,
                    longitude,
                },
                &mut point,
            )
        })
        .map_err(map_error)?;
        screen_point_to_py(py, point)
    }

    fn lat_lng_for_pixel(&self, py: Python<'_>, x: f64, y: f64) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let mut coordinate = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        // SAFETY: The C API validates the map pointer, point, and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lng_for_pixel(
                state.handle(),
                sys::mln_screen_point { x, y },
                &mut coordinate,
            )
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
        let mut points = vec![sys::mln_screen_point { x: 0.0, y: 0.0 }; coordinates.len()];
        // SAFETY: The C API validates the map pointer and coordinate/output slices.
        maplibre_core::check(unsafe {
            sys::mln_map_pixels_for_lat_lngs(
                state.handle(),
                coordinates.as_ptr(),
                coordinates.len(),
                points.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        screen_point_list_to_py(py, &points)
    }

    fn lat_lngs_for_pixels(&self, py: Python<'_>, points: Vec<(f64, f64)>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let points: Vec<_> = points.into_iter().map(screen_point_from_tuple).collect();
        let mut coordinates = vec![
            sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0
            };
            points.len()
        ];
        // SAFETY: The C API validates the map pointer and point/output slices.
        maplibre_core::check(unsafe {
            sys::mln_map_lat_lngs_for_pixels(
                state.handle(),
                points.as_ptr(),
                points.len(),
                coordinates.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        lat_lng_list_to_py(py, &coordinates)
    }

    #[allow(clippy::too_many_arguments)]
    fn add_style_source_json(
        &self,
        source_id: String,
        source_json: &Bound<'_, PyBytes>,
    ) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let source_json = maplibre_core::string::buffer_view(source_json.as_bytes());
        // SAFETY: The C API validates the map pointer, source ID, and JSON buffer view.
        maplibre_core::check(unsafe {
            sys::mln_map_add_style_source_json(state.handle(), source_id.raw(), source_json)
        })
        .map_err(map_error)
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
        synchronous_update: Option<bool>,
    ) -> PyResult<()> {
        let state = self.state();
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
            synchronous_update,
        )?;
        let options =
            maplibre_core::style::geojson_source_options_to_native(&options).map_err(map_error)?;
        // SAFETY: The C API validates the map pointer, borrowed string views, and options.
        maplibre_core::check(unsafe {
            sys::mln_map_add_geojson_source_url(
                state.handle(),
                source_id.raw(),
                url.raw(),
                options.as_ptr(),
            )
        })
        .map_err(map_error)
    }

    #[allow(clippy::too_many_arguments)]
    fn add_geojson_source_data(
        &self,
        source_id: String,
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
        synchronous_update: Option<bool>,
    ) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let data = maplibre_core::string::buffer_view(data.as_bytes());
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
            synchronous_update,
        )?;
        let options =
            maplibre_core::style::geojson_source_options_to_native(&options).map_err(map_error)?;
        // SAFETY: The C API validates the map pointer, source ID, GeoJSON buffer view, and options.
        maplibre_core::check(unsafe {
            sys::mln_map_add_geojson_source_data(
                state.handle(),
                source_id.raw(),
                data,
                options.as_ptr(),
            )
        })
        .map_err(map_error)
    }

    fn set_geojson_source_url(&self, source_id: String, url: String) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let url = maplibre_core::string::string_view(&url);
        // SAFETY: The C API validates the map pointer and borrowed string views.
        maplibre_core::check(unsafe {
            sys::mln_map_set_geojson_source_url(state.handle(), source_id.raw(), url.raw())
        })
        .map_err(map_error)
    }

    fn set_geojson_source_data(
        &self,
        source_id: String,
        data: &Bound<'_, PyBytes>,
    ) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let data = maplibre_core::string::buffer_view(data.as_bytes());
        // SAFETY: The C API validates the map pointer, source ID, and GeoJSON buffer view.
        maplibre_core::check(unsafe {
            sys::mln_map_set_geojson_source_data(state.handle(), source_id.raw(), data)
        })
        .map_err(map_error)
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
    ) -> PyResult<()> {
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
    ) -> PyResult<()> {
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
    ) -> PyResult<()> {
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
    ) -> PyResult<()> {
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
    ) -> PyResult<()> {
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
    ) -> PyResult<()> {
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

    fn remove_style_source(&self, source_id: String) -> PyResult<bool> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let mut removed = false;
        // SAFETY: The C API validates the map pointer, source ID view, and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_remove_style_source(state.handle(), source_id.raw(), &mut removed)
        })
        .map_err(map_error)?;
        Ok(removed)
    }

    fn style_source_exists(&self, source_id: String) -> PyResult<bool> {
        self.string_bool_call_with(source_id, sys::mln_map_style_source_exists)
    }

    fn get_style_source_type(&self, source_id: String) -> PyResult<Option<u32>> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let mut source_type = 0;
        let mut found = false;
        // SAFETY: The C API validates the map pointer, source ID view, and out pointers.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_source_type(
                state.handle(),
                source_id.raw(),
                &mut source_type,
                &mut found,
            )
        })
        .map_err(map_error)?;
        Ok(found.then_some(source_type))
    }

    fn get_style_source_info(
        &self,
        py: Python<'_>,
        source_id: String,
    ) -> PyResult<Option<Py<PyAny>>> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let mut info = maplibre_core::style::empty_style_source_info();
        let mut found = false;
        // SAFETY: The C API validates the map pointer, source ID view, info, and found pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_source_info(
                state.handle(),
                source_id.raw(),
                &mut info,
                &mut found,
            )
        })
        .map_err(map_error)?;
        if !found {
            return Ok(None);
        }
        let attribution = if info.has_attribution {
            let mut bytes = vec![0u8; info.attribution_size];
            let mut attribution_size = 0;
            let mut attribution_found = false;
            // SAFETY: bytes is writable for attribution_size bytes and retained
            // for this call. The C API validates all pointers.
            maplibre_core::check(unsafe {
                sys::mln_map_copy_style_source_attribution(
                    state.handle(),
                    source_id.raw(),
                    bytes.as_mut_ptr().cast::<c_char>(),
                    bytes.len(),
                    &mut attribution_size,
                    &mut attribution_found,
                )
            })
            .map_err(map_error)?;
            if attribution_found {
                bytes.truncate(attribution_size);
                Some(String::from_utf8(bytes).map_err(|error| {
                    invalid_argument_error(format!("native attribution is not UTF-8: {error}"))
                })?)
            } else {
                None
            }
        } else {
            None
        };
        let url = if info.fields & sys::MLN_STYLE_SOURCE_INFO_URL != 0 {
            let mut bytes = vec![0u8; info.url_size];
            let mut url_size = 0;
            let mut url_found = false;
            // SAFETY: bytes and both output values remain writable for this call.
            maplibre_core::check(unsafe {
                sys::mln_map_copy_style_source_url(
                    state.handle(),
                    source_id.raw(),
                    bytes.as_mut_ptr().cast::<c_char>(),
                    bytes.len(),
                    &mut url_size,
                    &mut url_found,
                )
            })
            .map_err(map_error)?;
            if !url_found {
                return Ok(None);
            }
            bytes.truncate(url_size);
            Some(String::from_utf8(bytes).map_err(|error| {
                invalid_argument_error(format!("native source URL is not UTF-8: {error}"))
            })?)
        } else {
            None
        };
        let tiles = if info.fields & sys::MLN_STYLE_SOURCE_INFO_TILEJSON != 0 {
            let mut out = maplibre_core::ptr::OutHandle::<sys::mln_style_string_list>::new();
            let mut tiles_found = false;
            // SAFETY: out starts null and both output values remain writable for this call.
            maplibre_core::check(unsafe {
                sys::mln_map_get_style_source_tile_urls(
                    state.handle(),
                    source_id.raw(),
                    out.as_mut_ptr(),
                    &mut tiles_found,
                )
            })
            .map_err(map_error)?;
            if !tiles_found {
                return Ok(None);
            }
            let native = out.into_live("mln_style_string_list").map_err(map_error)?;
            // SAFETY: native is an owned style string list returned by C.
            unsafe { maplibre_core::style::copy_style_string_list(native) }.map_err(map_error)?
        } else {
            Vec::new()
        };
        let copied =
            maplibre_core::style::style_source_info_from_native(&info, attribution, url, tiles);
        source_info_to_py(py, copied).map(Some)
    }

    fn list_style_source_ids(&self) -> PyResult<Vec<String>> {
        let state = self.state();
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_style_id_list>::new();
        // SAFETY: The C API validates the map pointer and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_list_style_source_ids(state.handle(), out.as_mut_ptr())
        })
        .map_err(map_error)?;
        let native = out.into_live("mln_style_id_list").map_err(map_error)?;
        // SAFETY: native is an owned style ID list returned by native.
        unsafe { maplibre_core::style::copy_style_id_list(native) }.map_err(map_error)
    }

    fn add_hillshade_layer(
        &self,
        layer_id: String,
        source_id: String,
        before_layer_id: Option<String>,
    ) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let source_id = maplibre_core::string::string_view(&source_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = maplibre_core::string::string_view(&before_layer_id);
        // SAFETY: The C API validates the map pointer and borrowed layer/source ID views.
        maplibre_core::check(unsafe {
            sys::mln_map_add_hillshade_layer(
                state.handle(),
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
            )
        })
        .map_err(map_error)
    }

    fn add_color_relief_layer(
        &self,
        layer_id: String,
        source_id: String,
        before_layer_id: Option<String>,
    ) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let source_id = maplibre_core::string::string_view(&source_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = maplibre_core::string::string_view(&before_layer_id);
        // SAFETY: The C API validates the map pointer and borrowed layer/source ID views.
        maplibre_core::check(unsafe {
            sys::mln_map_add_color_relief_layer(
                state.handle(),
                layer_id.raw(),
                source_id.raw(),
                before_layer_id.raw(),
            )
        })
        .map_err(map_error)
    }

    fn add_location_indicator_layer(
        &self,
        layer_id: String,
        before_layer_id: Option<String>,
    ) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = maplibre_core::string::string_view(&before_layer_id);
        // SAFETY: The C API validates the map pointer and borrowed layer ID views.
        maplibre_core::check(unsafe {
            sys::mln_map_add_location_indicator_layer(
                state.handle(),
                layer_id.raw(),
                before_layer_id.raw(),
            )
        })
        .map_err(map_error)
    }

    fn set_location_indicator_location(
        &self,
        layer_id: String,
        latitude: f64,
        longitude: f64,
        altitude: f64,
    ) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer, layer ID, coordinate, and altitude.
        maplibre_core::check(unsafe {
            sys::mln_map_set_location_indicator_location(
                state.handle(),
                layer_id.raw(),
                sys::mln_lat_lng {
                    latitude,
                    longitude,
                },
                altitude,
            )
        })
        .map_err(map_error)
    }

    fn set_location_indicator_bearing(&self, layer_id: String, bearing: f64) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer, layer ID, and bearing.
        maplibre_core::check(unsafe {
            sys::mln_map_set_location_indicator_bearing(state.handle(), layer_id.raw(), bearing)
        })
        .map_err(map_error)
    }

    fn set_location_indicator_accuracy_radius(
        &self,
        layer_id: String,
        radius: f64,
    ) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer, layer ID, and radius.
        maplibre_core::check(unsafe {
            sys::mln_map_set_location_indicator_accuracy_radius(
                state.handle(),
                layer_id.raw(),
                radius,
            )
        })
        .map_err(map_error)
    }

    fn set_location_indicator_image_name(
        &self,
        layer_id: String,
        image_kind: u32,
        image_id: String,
    ) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let image_id = maplibre_core::string::string_view(&image_id);
        // SAFETY: The C API validates the map pointer, layer ID, image kind, and image ID.
        maplibre_core::check(unsafe {
            sys::mln_map_set_location_indicator_image_name(
                state.handle(),
                layer_id.raw(),
                image_kind,
                image_id.raw(),
            )
        })
        .map_err(map_error)
    }

    fn add_style_layer_json(
        &self,
        layer_json: &Bound<'_, PyBytes>,
        before_layer_id: Option<String>,
    ) -> PyResult<()> {
        let state = self.state();
        let layer_json = maplibre_core::string::buffer_view(layer_json.as_bytes());
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = maplibre_core::string::string_view(&before_layer_id);
        // SAFETY: The C API validates the map pointer, JSON buffer view, and before-layer ID.
        maplibre_core::check(unsafe {
            sys::mln_map_add_style_layer_json(state.handle(), layer_json, before_layer_id.raw())
        })
        .map_err(map_error)
    }

    fn get_style_layer_json(
        &self,
        py: Python<'_>,
        layer_id: String,
    ) -> PyResult<Option<Py<PyBytes>>> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        let mut found = false;
        // SAFETY: The C API validates the map pointer, layer ID, out pointer, and found pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_layer_json(
                state.handle(),
                layer_id.raw(),
                out.as_mut_ptr(),
                &mut found,
            )
        })
        .map_err(map_error)?;
        if !found {
            return Ok(None);
        }
        owned_buffer_to_py(py, out.get()).map(Some)
    }

    fn set_style_light_json(&self, light_json: &Bound<'_, PyBytes>) -> PyResult<()> {
        let state = self.state();
        let light_json = maplibre_core::string::buffer_view(light_json.as_bytes());
        // SAFETY: The C API validates the map pointer and JSON buffer view.
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_light_json(state.handle(), light_json)
        })
        .map_err(map_error)
    }

    fn set_style_light_property(
        &self,
        property_name: String,
        value: &Bound<'_, PyBytes>,
    ) -> PyResult<()> {
        let state = self.state();
        let property_name = maplibre_core::string::string_view(&property_name);
        let value = maplibre_core::string::buffer_view(value.as_bytes());
        // SAFETY: The C API validates the map pointer, property name, and JSON buffer view.
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_light_property(state.handle(), property_name.raw(), value)
        })
        .map_err(map_error)
    }

    fn get_style_light_property(
        &self,
        py: Python<'_>,
        property_name: String,
    ) -> PyResult<Option<Py<PyBytes>>> {
        let state = self.state();
        let property_name = maplibre_core::string::string_view(&property_name);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: The C API validates the map pointer, property name, and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_light_property(
                state.handle(),
                property_name.raw(),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        out.into_option()
            .map(|buffer| owned_buffer_to_py(py, buffer))
            .transpose()
    }

    fn set_style_transition_options(
        &self,
        duration_ms: Option<f64>,
        delay_ms: Option<f64>,
        enable_placement_transitions: Option<bool>,
    ) -> PyResult<()> {
        let state = self.state();
        let mut options = maplibre_core::StyleTransitionOptions::default();
        options.duration_ms = duration_ms;
        options.delay_ms = delay_ms;
        options.enable_placement_transitions = enable_placement_transitions;
        let options = maplibre_core::style::style_transition_options_to_native(&options);
        // SAFETY: The C API validates the map pointer and options struct.
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_transition_options(state.handle(), &options)
        })
        .map_err(map_error)
    }

    fn get_style_transition_options(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let mut options = maplibre_core::style::empty_style_transition_options();
        // SAFETY: The C API validates the map pointer and out-options pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_transition_options(state.handle(), &mut options)
        })
        .map_err(map_error)?;
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
    ) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let property_name = maplibre_core::string::string_view(&property_name);
        let value = maplibre_core::string::buffer_view(value.as_bytes());
        // SAFETY: The C API validates the map pointer, layer/property names, and JSON buffer view.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_property(
                state.handle(),
                layer_id.raw(),
                property_name.raw(),
                value,
            )
        })
        .map_err(map_error)
    }

    fn get_layer_property(
        &self,
        py: Python<'_>,
        layer_id: String,
        property_name: String,
    ) -> PyResult<Option<Py<PyBytes>>> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let property_name = maplibre_core::string::string_view(&property_name);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: The C API validates the map pointer, layer/property names, and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_property(
                state.handle(),
                layer_id.raw(),
                property_name.raw(),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        out.into_option()
            .map(|buffer| owned_buffer_to_py(py, buffer))
            .transpose()
    }

    fn set_layer_source_layer(&self, layer_id: String, source_layer: String) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let source_layer = maplibre_core::string::string_view(&source_layer);
        // SAFETY: The C API validates the map pointer and both string views.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_source_layer(state.handle(), layer_id.raw(), source_layer.raw())
        })
        .map_err(map_error)
    }

    fn copy_layer_source_layer(&self, layer_id: String) -> PyResult<String> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer, layer ID, and each buffer
        // and out pointer it is given.
        unsafe {
            copy_text(|text, capacity, out_size| {
                sys::mln_map_copy_layer_source_layer(
                    state.handle(),
                    layer_id.raw(),
                    text,
                    capacity,
                    out_size,
                )
            })
        }
    }

    fn set_layer_source_id(&self, layer_id: String, source_id: String) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let source_id = maplibre_core::string::string_view(&source_id);
        // SAFETY: The C API validates the map pointer and both string views.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_source_id(state.handle(), layer_id.raw(), source_id.raw())
        })
        .map_err(map_error)
    }

    fn copy_layer_source_id(&self, layer_id: String) -> PyResult<String> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer, layer ID, and each buffer
        // and out pointer it is given.
        unsafe {
            copy_text(|text, capacity, out_size| {
                sys::mln_map_copy_layer_source_id(
                    state.handle(),
                    layer_id.raw(),
                    text,
                    capacity,
                    out_size,
                )
            })
        }
    }

    fn set_layer_min_zoom(&self, layer_id: String, min_zoom: f64) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer and layer ID.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_min_zoom(state.handle(), layer_id.raw(), min_zoom)
        })
        .map_err(map_error)
    }

    fn get_layer_min_zoom(&self, layer_id: String) -> PyResult<f64> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let mut min_zoom = 0.0;
        // SAFETY: The C API validates the map pointer, layer ID, and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_min_zoom(state.handle(), layer_id.raw(), &mut min_zoom)
        })
        .map_err(map_error)?;
        Ok(min_zoom)
    }

    fn set_layer_max_zoom(&self, layer_id: String, max_zoom: f64) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer and layer ID.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_max_zoom(state.handle(), layer_id.raw(), max_zoom)
        })
        .map_err(map_error)
    }

    fn get_layer_max_zoom(&self, layer_id: String) -> PyResult<f64> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let mut max_zoom = 0.0;
        // SAFETY: The C API validates the map pointer, layer ID, and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_max_zoom(state.handle(), layer_id.raw(), &mut max_zoom)
        })
        .map_err(map_error)?;
        Ok(max_zoom)
    }

    fn set_layer_visibility(&self, layer_id: String, visibility: u32) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        // SAFETY: The C API validates the map pointer, layer ID, and enum value.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_visibility(state.handle(), layer_id.raw(), visibility)
        })
        .map_err(map_error)
    }

    fn get_layer_visibility(&self, layer_id: String) -> PyResult<u32> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let mut visibility = 0;
        // SAFETY: The C API validates the map pointer, layer ID, and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_visibility(state.handle(), layer_id.raw(), &mut visibility)
        })
        .map_err(map_error)?;
        Ok(visibility)
    }

    fn set_layer_filter(
        &self,
        layer_id: String,
        filter: Option<&Bound<'_, PyBytes>>,
    ) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let filter = filter.map(|value| maplibre_core::string::buffer_view(value.as_bytes()));
        // SAFETY: The C API validates the map pointer, layer ID, and optional JSON buffer view.
        maplibre_core::check(unsafe {
            sys::mln_map_set_layer_filter(
                state.handle(),
                layer_id.raw(),
                optional_ref_ptr(filter.as_ref()),
            )
        })
        .map_err(map_error)
    }

    fn get_layer_filter(&self, py: Python<'_>, layer_id: String) -> PyResult<Option<Py<PyBytes>>> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: The C API validates the map pointer, layer ID, and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_layer_filter(state.handle(), layer_id.raw(), out.as_mut_ptr())
        })
        .map_err(map_error)?;
        out.into_option()
            .map(|buffer| owned_buffer_to_py(py, buffer))
            .transpose()
    }

    fn remove_style_layer(&self, layer_id: String) -> PyResult<bool> {
        self.string_bool_call_with(layer_id, sys::mln_map_remove_style_layer)
    }

    fn style_layer_exists(&self, layer_id: String) -> PyResult<bool> {
        self.string_bool_call_with(layer_id, sys::mln_map_style_layer_exists)
    }

    fn get_style_layer_type(&self, layer_id: String) -> PyResult<Option<String>> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let mut layer_type = sys::mln_buffer_view {
            data: ptr::null(),
            size: 0,
        };
        let mut found = false;
        // SAFETY: The C API validates the map pointer, layer ID view, and out pointers.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_layer_type(
                state.handle(),
                layer_id.raw(),
                &mut layer_type,
                &mut found,
            )
        })
        .map_err(map_error)?;
        if !found {
            return Ok(None);
        }
        // SAFETY: The returned layer type view is static/live for this call and copied immediately.
        unsafe { maplibre_core::string::copy_string_view(layer_type) }
            .map(Some)
            .map_err(map_error)
    }

    fn list_style_layer_ids(&self) -> PyResult<Vec<String>> {
        let state = self.state();
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_style_id_list>::new();
        // SAFETY: The C API validates the map pointer and out pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_list_style_layer_ids(state.handle(), out.as_mut_ptr())
        })
        .map_err(map_error)?;
        let native = out.into_live("mln_style_id_list").map_err(map_error)?;
        // SAFETY: native is an owned style ID list returned by native.
        unsafe { maplibre_core::style::copy_style_id_list(native) }.map_err(map_error)
    }

    fn move_style_layer(&self, layer_id: String, before_layer_id: Option<String>) -> PyResult<()> {
        let state = self.state();
        let layer_id = maplibre_core::string::string_view(&layer_id);
        let before_layer_id = before_layer_id.unwrap_or_default();
        let before_layer_id = maplibre_core::string::string_view(&before_layer_id);
        // SAFETY: The C API validates the map pointer and borrowed layer ID views.
        maplibre_core::check(unsafe {
            sys::mln_map_move_style_layer(state.handle(), layer_id.raw(), before_layer_id.raw())
        })
        .map_err(map_error)
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
    ) -> PyResult<()> {
        let state = self.state();
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
        maplibre_core::check(unsafe {
            sys::mln_map_set_style_image(state.handle(), image_id.raw(), &image, &options)
        })
        .map_err(map_error)
    }

    fn remove_style_image(&self, image_id: String) -> PyResult<bool> {
        self.string_bool_call_with(image_id, sys::mln_map_remove_style_image)
    }

    fn style_image_exists(&self, image_id: String) -> PyResult<bool> {
        self.string_bool_call_with(image_id, sys::mln_map_style_image_exists)
    }

    fn copy_style_image_stretches(
        &self,
        image_id: String,
    ) -> PyResult<Option<(Vec<(f32, f32)>, Vec<(f32, f32)>)>> {
        let state = self.state();
        let image_id = maplibre_core::string::string_view(&image_id);
        let mut x_count = 0;
        let mut y_count = 0;
        let mut found = false;
        // SAFETY: The C API validates the map pointer and image ID. Null arrays
        // with zero capacity make this a size probe.
        maplibre_core::check(unsafe {
            sys::mln_map_copy_style_image_stretches(
                state.handle(),
                image_id.raw(),
                std::ptr::null_mut(),
                0,
                &mut x_count,
                std::ptr::null_mut(),
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
        // SAFETY: Each buffer is writable for its reported count.
        maplibre_core::check(unsafe {
            sys::mln_map_copy_style_image_stretches(
                state.handle(),
                image_id.raw(),
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
        let to_public = |raw: &[sys::mln_image_stretch]| -> Vec<(f32, f32)> {
            raw.iter()
                .map(|stretch| (stretch.from, stretch.to))
                .collect()
        };
        Ok(Some((to_public(&raw_x), to_public(&raw_y))))
    }

    fn get_style_image_info(
        &self,
        py: Python<'_>,
        image_id: String,
    ) -> PyResult<Option<Py<PyAny>>> {
        let state = self.state();
        let image_id = maplibre_core::string::string_view(&image_id);
        let mut info = maplibre_core::style::empty_style_image_info();
        let mut found = false;
        // SAFETY: The C API validates the map pointer, image ID view, info, and found pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_image_info(state.handle(), image_id.raw(), &mut info, &mut found)
        })
        .map_err(map_error)?;
        if found {
            style_image_info_to_py(py, &info).map(Some)
        } else {
            Ok(None)
        }
    }

    fn copy_style_image_premultiplied_rgba8(
        &self,
        py: Python<'_>,
        image_id: String,
    ) -> PyResult<Option<Py<PyAny>>> {
        let state = self.state();
        let image_id = maplibre_core::string::string_view(&image_id);
        let mut info = maplibre_core::style::empty_style_image_info();
        let mut found = false;
        // SAFETY: The C API validates the map pointer, image ID view, info, and found pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_get_style_image_info(state.handle(), image_id.raw(), &mut info, &mut found)
        })
        .map_err(map_error)?;
        if !found {
            return Ok(None);
        }
        let mut pixels = vec![0u8; info.byte_length];
        let mut byte_length = 0;
        let mut copy_found = false;
        // SAFETY: pixels is writable for its length and retained for this call.
        // The C API validates all pointers and capacity.
        maplibre_core::check(unsafe {
            sys::mln_map_copy_style_image_premultiplied_rgba8(
                state.handle(),
                image_id.raw(),
                pixels.as_mut_ptr(),
                pixels.len(),
                &mut byte_length,
                &mut copy_found,
            )
        })
        .map_err(map_error)?;
        if !copy_found {
            return Ok(None);
        }
        pixels.truncate(byte_length);
        style_image_to_py(py, &info, &pixels).map(Some)
    }

    fn add_image_source_url(
        &self,
        source_id: String,
        coordinates: Vec<(f64, f64)>,
        url: String,
    ) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let url = maplibre_core::string::string_view(&url);
        let coordinates = lat_lngs_from_tuples(coordinates);
        // SAFETY: The C API validates the map pointer, source ID, coordinate slice, and URL.
        maplibre_core::check(unsafe {
            sys::mln_map_add_image_source_url(
                state.handle(),
                source_id.raw(),
                coordinates.as_ptr(),
                coordinates.len(),
                url.raw(),
            )
        })
        .map_err(map_error)
    }

    fn add_image_source_image(
        &self,
        source_id: String,
        coordinates: Vec<(f64, f64)>,
        width: u32,
        height: u32,
        stride: u32,
        pixels: Vec<u8>,
    ) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let coordinates = lat_lngs_from_tuples(coordinates);
        let image = premultiplied_rgba8_image_from_parts(width, height, stride, &pixels);
        // SAFETY: The C API validates the map pointer, source ID, coordinates,
        // image descriptor, and pixel storage. pixels is retained for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_add_image_source_image(
                state.handle(),
                source_id.raw(),
                coordinates.as_ptr(),
                coordinates.len(),
                &image,
            )
        })
        .map_err(map_error)
    }

    fn set_image_source_url(&self, source_id: String, url: String) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let url = maplibre_core::string::string_view(&url);
        // SAFETY: The C API validates the map pointer and borrowed string views.
        maplibre_core::check(unsafe {
            sys::mln_map_set_image_source_url(state.handle(), source_id.raw(), url.raw())
        })
        .map_err(map_error)
    }

    fn set_image_source_image(
        &self,
        source_id: String,
        width: u32,
        height: u32,
        stride: u32,
        pixels: Vec<u8>,
    ) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let image = premultiplied_rgba8_image_from_parts(width, height, stride, &pixels);
        // SAFETY: The C API validates the map pointer, source ID, image descriptor,
        // and pixel storage. pixels is retained for this call.
        maplibre_core::check(unsafe {
            sys::mln_map_set_image_source_image(state.handle(), source_id.raw(), &image)
        })
        .map_err(map_error)
    }

    fn set_image_source_coordinates(
        &self,
        source_id: String,
        coordinates: Vec<(f64, f64)>,
    ) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let coordinates = lat_lngs_from_tuples(coordinates);
        // SAFETY: The C API validates the map pointer, source ID, and coordinate slice.
        maplibre_core::check(unsafe {
            sys::mln_map_set_image_source_coordinates(
                state.handle(),
                source_id.raw(),
                coordinates.as_ptr(),
                coordinates.len(),
            )
        })
        .map_err(map_error)
    }

    fn get_image_source_coordinates(
        &self,
        py: Python<'_>,
        source_id: String,
    ) -> PyResult<Option<Py<PyAny>>> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let mut coordinates = vec![
            sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0
            };
            4
        ];
        let mut coordinate_count = 0;
        let mut found = false;
        // SAFETY: coordinates is writable for four entries. The C API validates
        // all pointers and returns a copied coordinate count.
        maplibre_core::check(unsafe {
            sys::mln_map_get_image_source_coordinates(
                state.handle(),
                source_id.raw(),
                coordinates.as_mut_ptr(),
                coordinates.len(),
                &mut coordinate_count,
                &mut found,
            )
        })
        .map_err(map_error)?;
        if !found {
            return Ok(None);
        }
        coordinates.truncate(coordinate_count);
        lat_lng_list_to_py(py, &coordinates).map(Some)
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
    ) -> PyResult<CustomGeometrySourceHandle> {
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
        // SAFETY: map_state owns or has released the map pointer. The C API
        // validates that it is live. source_id_view and descriptor are valid for
        // this call, and descriptor's release callback takes the leaked state
        // back once the C API stops referencing it.
        maplibre_core::check(unsafe {
            sys::mln_map_add_custom_geometry_source(
                map_state.handle(),
                source_id_view.raw(),
                &descriptor,
            )
        })
        .map_err(map_error)?;
        // A rejected add owes no release, so the state only leaves this box
        // after the C API accepted it.
        Box::leak(state);
        Ok(handle)
    }

    fn set_custom_geometry_source_tile_data(
        &self,
        source_id: String,
        z: u32,
        x: u32,
        y: u32,
        data: &Bound<'_, PyBytes>,
    ) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let data = maplibre_core::string::buffer_view(data.as_bytes());
        // SAFETY: The C API validates the map pointer, source ID, tile ID, and GeoJSON buffer view.
        maplibre_core::check(unsafe {
            sys::mln_map_set_custom_geometry_source_tile_data(
                state.handle(),
                source_id.raw(),
                sys::mln_canonical_tile_id { z, x, y },
                data,
            )
        })
        .map_err(map_error)
    }

    fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: String,
        z: u32,
        x: u32,
        y: u32,
    ) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        // SAFETY: The C API validates the map pointer, source ID, and tile ID.
        maplibre_core::check(unsafe {
            sys::mln_map_invalidate_custom_geometry_source_tile(
                state.handle(),
                source_id.raw(),
                sys::mln_canonical_tile_id { z, x, y },
            )
        })
        .map_err(map_error)
    }

    fn invalidate_custom_geometry_source_region(
        &self,
        source_id: String,
        southwest: (f64, f64),
        northeast: (f64, f64),
    ) -> PyResult<()> {
        let state = self.state();
        let source_id = maplibre_core::string::string_view(&source_id);
        let bounds = lat_lng_bounds_from_tuple((southwest, northeast));
        // SAFETY: The C API validates the map pointer, source ID, and bounds.
        maplibre_core::check(unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(
                state.handle(),
                source_id.raw(),
                bounds,
            )
        })
        .map_err(map_error)
    }

    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
}

#[pymethods]
impl MapProjectionHandle {
    fn close(&self) -> PyResult<()> {
        let state = self.state();
        // SAFETY: state owns an mln_map_projection handle created by
        // mln_map_projection_create and pairs it with the matching destroy.
        unsafe { state.close_status(sys::mln_map_projection_destroy) }.map_err(map_error)
    }

    fn get_camera(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self.state();
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut camera = unsafe { sys::mln_camera_options_default() };
        // SAFETY: The C API validates that the projection is live and camera
        // points to initialized writable storage.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_get_camera(state.handle(), &mut camera)
        })
        .map_err(map_error)?;
        camera_options_to_py(py, &camera)
    }

    #[allow(clippy::too_many_arguments)]
    fn set_camera(
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
    ) -> PyResult<()> {
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
        // SAFETY: The C API validates the projection pointer and camera fields.
        maplibre_core::check(unsafe { sys::mln_map_projection_set_camera(state.handle(), &camera) })
            .map_err(map_error)
    }

    fn set_visible_coordinates(
        &self,
        coordinates: Vec<(f64, f64)>,
        padding: (f64, f64, f64, f64),
    ) -> PyResult<()> {
        let state = self.state();
        let coordinates: Vec<sys::mln_lat_lng> = coordinates
            .into_iter()
            .map(|(latitude, longitude)| sys::mln_lat_lng {
                latitude,
                longitude,
            })
            .collect();
        let padding = edge_insets_from_tuple(padding);
        // SAFETY: The C API validates the projection pointer, coordinates, and
        // padding. coordinates is retained for the duration of this call.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_visible_coordinates(
                state.handle(),
                coordinates.as_ptr(),
                coordinates.len(),
                padding,
            )
        })
        .map_err(map_error)
    }

    fn set_visible_geometry(
        &self,
        geometry: &Bound<'_, PyBytes>,
        padding: (f64, f64, f64, f64),
    ) -> PyResult<()> {
        let state = self.state();
        let geometry = maplibre_core::string::buffer_view(geometry.as_bytes());
        let padding = edge_insets_from_tuple(padding);
        // SAFETY: The C API validates the projection pointer, geometry buffer view, and padding.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_visible_geometry(state.handle(), geometry, padding)
        })
        .map_err(map_error)
    }

    fn pixel_for_lat_lng(
        &self,
        py: Python<'_>,
        latitude: f64,
        longitude: f64,
    ) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let mut point = sys::mln_screen_point { x: 0.0, y: 0.0 };
        // SAFETY: The C API validates the projection pointer, coordinate, and
        // output pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_pixel_for_lat_lng(
                state.handle(),
                sys::mln_lat_lng {
                    latitude,
                    longitude,
                },
                &mut point,
            )
        })
        .map_err(map_error)?;
        screen_point_to_py(py, point)
    }

    fn lat_lng_for_pixel(&self, py: Python<'_>, x: f64, y: f64) -> PyResult<Py<PyAny>> {
        let state = self.state();
        let mut coordinate = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        // SAFETY: The C API validates the projection pointer, point, and output
        // pointer.
        maplibre_core::check(unsafe {
            sys::mln_map_projection_lat_lng_for_pixel(
                state.handle(),
                sys::mln_screen_point { x, y },
                &mut coordinate,
            )
        })
        .map_err(map_error)?;
        lat_lng_to_py(py, coordinate)
    }

    #[getter]
    fn closed(&self) -> bool {
        self.state().is_closed()
    }
}

impl RenderSessionHandle {
    /// Shared body for the `set_*_target` methods. The caller materializes the
    /// native descriptor, which the C API borrows only for this call.
    fn set_target<D>(
        &self,
        raw: D,
        set_target: impl FnOnce(sys::mln_render_session, &D) -> sys::mln_status,
    ) -> PyResult<()> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        maplibre_core::check(set_target(state.native(), &raw)).map_err(map_error)
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
        state.ensure_no_frame_acquired()?;
        // SAFETY: state owns an mln_render_session handle created by an attach
        // function and pairs it with the matching status-returning destroy.
        unsafe { state.handle.close_status(sys::mln_render_session_destroy) }.map_err(map_error)
    }

    fn resize(&self, width: u32, height: u32, scale_factor: f64) -> PyResult<()> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        // SAFETY: The C API validates that the pointer is live, attached, and
        // called on the owner thread.
        maplibre_core::check(unsafe {
            sys::mln_render_session_resize(state.native(), width, height, scale_factor)
        })
        .map_err(map_error)
    }

    fn set_metal_surface_target(
        &self,
        width: u32,
        height: u32,
        scale_factor: f64,
        device_address: usize,
        layer_address: usize,
    ) -> PyResult<()> {
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
        self.set_target(descriptor, |session, raw| {
            // SAFETY: raw is fully initialized and lives for this call. The C
            // API validates the session pointer, state, and descriptor fields.
            unsafe { sys::mln_metal_surface_set_target(session, raw) }
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
    ) -> PyResult<()> {
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
        self.set_target(descriptor, |session, raw| {
            // SAFETY: raw is fully initialized and lives for this call. The C
            // API validates the session pointer, state, and descriptor fields.
            unsafe { sys::mln_vulkan_surface_set_target(session, raw) }
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
    ) -> PyResult<()> {
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
        self.set_target(descriptor, |session, raw| {
            // SAFETY: raw is fully initialized and lives for this call. The C
            // API validates the session pointer, state, and descriptor fields.
            unsafe { sys::mln_opengl_surface_set_target(session, raw) }
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
    ) -> PyResult<()> {
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
        self.set_target(descriptor, |session, raw| {
            // SAFETY: raw is fully initialized and lives for this call. The C
            // API validates the session pointer, state, and descriptor fields.
            unsafe { sys::mln_metal_borrowed_texture_set_target(session, raw) }
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
    ) -> PyResult<()> {
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
        self.set_target(descriptor, |session, raw| {
            // SAFETY: raw is fully initialized and lives for this call. The C
            // API validates the session pointer, state, and descriptor fields.
            unsafe { sys::mln_vulkan_borrowed_texture_set_target(session, raw) }
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
    ) -> PyResult<()> {
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
        self.set_target(descriptor, |session, raw| {
            // SAFETY: raw is fully initialized and lives for this call. The C
            // API validates the session pointer, state, and descriptor fields.
            unsafe { sys::mln_opengl_borrowed_texture_set_target(session, raw) }
        })
    }

    fn render_update(&self) -> PyResult<u32> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let mut result = sys::MLN_RENDER_RESULT_NO_UPDATE;
        // SAFETY: The C API validates the render-session pointer and state, and
        // result points to caller-owned output storage.
        maplibre_core::check(unsafe {
            sys::mln_render_session_render_update(state.native(), &raw mut result)
        })
        .map_err(map_error)?;
        Ok(result)
    }

    fn detach(&self) -> PyResult<DetachedRenderSessionHandle> {
        let mut state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        // SAFETY: The C API validates the render-session pointer and state.
        maplibre_core::check(unsafe { sys::mln_render_session_detach(state.native()) })
            .map_err(map_error)?;
        state.detached = true;
        drop(state);
        Ok(DetachedRenderSessionHandle {
            state: Arc::clone(&self.state),
        })
    }

    fn reduce_memory_use(&self) -> PyResult<()> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        // SAFETY: The C API validates the render-session pointer and state.
        maplibre_core::check(unsafe { sys::mln_render_session_reduce_memory_use(state.native()) })
            .map_err(map_error)
    }

    fn clear_data(&self) -> PyResult<()> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        // SAFETY: The C API validates the render-session pointer and state.
        maplibre_core::check(unsafe { sys::mln_render_session_clear_data(state.native()) })
            .map_err(map_error)
    }

    fn dump_debug_logs(&self) -> PyResult<()> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        // SAFETY: The C API validates the render-session pointer and state.
        maplibre_core::check(unsafe { sys::mln_render_session_dump_debug_logs(state.native()) })
            .map_err(map_error)
    }

    fn query_rendered_features(
        &self,
        py: Python<'_>,
        geometry: &Bound<'_, PyAny>,
        layer_ids: Option<Vec<String>>,
        filter: Option<&Bound<'_, PyBytes>>,
    ) -> PyResult<Py<PyBytes>> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let geometry = rendered_query_geometry_from_wire(geometry)?;
        let geometry = maplibre_core::query::rendered_query_geometry_to_native(&geometry);
        let mut options = maplibre_core::RenderedFeatureQueryOptions::default();
        if let Some(layer_ids) = layer_ids {
            options.layer_ids = Some(layer_ids);
        }
        if let Some(filter) = filter {
            options.filter = Some(filter.as_bytes().to_vec());
        }
        let options = maplibre_core::query::rendered_feature_query_options_to_native(&options)
            .map_err(map_error)?;
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: The C API validates the render-session pointer, query geometry/options, and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_rendered_features(
                state.native(),
                geometry.as_ptr(),
                options.as_ptr(),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        owned_buffer_to_py(py, out.get())
    }

    fn query_source_features(
        &self,
        py: Python<'_>,
        source_id: String,
        source_layer_ids: Option<Vec<String>>,
        filter: Option<&Bound<'_, PyBytes>>,
    ) -> PyResult<Py<PyBytes>> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let source_id = maplibre_core::string::string_view(&source_id);
        let mut options = maplibre_core::SourceFeatureQueryOptions::default();
        if let Some(source_layer_ids) = source_layer_ids {
            options.source_layer_ids = Some(source_layer_ids);
        }
        if let Some(filter) = filter {
            options.filter = Some(filter.as_bytes().to_vec());
        }
        let options = maplibre_core::query::source_feature_query_options_to_native(&options)
            .map_err(map_error)?;
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: The C API validates the render-session pointer, source ID, query options, and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_source_features(
                state.native(),
                source_id.raw(),
                options.as_ptr(),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        owned_buffer_to_py(py, out.get())
    }

    fn query_feature_extensions(
        &self,
        py: Python<'_>,
        source_id: String,
        feature: &Bound<'_, PyBytes>,
        extension: String,
        extension_field: String,
        arguments: Option<&Bound<'_, PyBytes>>,
    ) -> PyResult<Py<PyBytes>> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let source_id = maplibre_core::string::string_view(&source_id);
        let feature = maplibre_core::string::buffer_view(feature.as_bytes());
        let extension = maplibre_core::string::string_view(&extension);
        let extension_field = maplibre_core::string::string_view(&extension_field);
        let arguments = arguments.map(|value| maplibre_core::string::buffer_view(value.as_bytes()));
        let arguments_ptr = optional_ref_ptr(arguments.as_ref());
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: The C API validates the render-session pointer, feature, strings, arguments, and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_feature_extensions(
                state.native(),
                source_id.raw(),
                feature,
                extension.raw(),
                extension_field.raw(),
                arguments_ptr,
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        owned_buffer_to_py(py, out.get())
    }

    fn set_feature_state(
        &self,
        source_id: String,
        source_layer_id: Option<String>,
        feature_id: Option<String>,
        state_key: Option<String>,
        state_value: &Bound<'_, PyBytes>,
    ) -> PyResult<()> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let selector =
            feature_state_selector_from_parts(source_id, source_layer_id, feature_id, state_key)?;
        let selector = maplibre_core::query::feature_state_selector_to_native(&selector);
        let state_value = maplibre_core::string::buffer_view(state_value.as_bytes());
        // SAFETY: The C API validates the render-session pointer, selector, and JSON state.
        maplibre_core::check(unsafe {
            sys::mln_render_session_set_feature_state(
                state.native(),
                selector.as_ptr(),
                state_value,
            )
        })
        .map_err(map_error)
    }

    fn get_feature_state(
        &self,
        py: Python<'_>,
        source_id: String,
        source_layer_id: Option<String>,
        feature_id: Option<String>,
        state_key: Option<String>,
    ) -> PyResult<Py<PyBytes>> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let selector =
            feature_state_selector_from_parts(source_id, source_layer_id, feature_id, state_key)?;
        let selector = maplibre_core::query::feature_state_selector_to_native(&selector);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: The C API validates the render-session pointer, selector, and output pointer.
        maplibre_core::check(unsafe {
            sys::mln_render_session_get_feature_state(
                state.native(),
                selector.as_ptr(),
                out.as_mut_ptr(),
            )
        })
        .map_err(map_error)?;
        owned_buffer_to_py(py, out.get())
    }

    fn remove_feature_state(
        &self,
        source_id: String,
        source_layer_id: Option<String>,
        feature_id: Option<String>,
        state_key: Option<String>,
    ) -> PyResult<()> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let selector =
            feature_state_selector_from_parts(source_id, source_layer_id, feature_id, state_key)?;
        let selector = maplibre_core::query::feature_state_selector_to_native(&selector);
        // SAFETY: The C API validates the render-session pointer and selector.
        maplibre_core::check(unsafe {
            sys::mln_render_session_remove_feature_state(state.native(), selector.as_ptr())
        })
        .map_err(map_error)
    }

    fn texture_image_info(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let info = probe_texture_image_info(state.native())?;
        texture_image_info_to_py(py, info)
    }

    fn read_premultiplied_rgba8(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let info = probe_texture_image_info(state.native())?;
        let mut data = vec![0; info.byte_length];
        let info = read_texture_image_into(state.native(), &mut data)?;
        image_to_py(py, info, &data)
    }

    fn read_premultiplied_rgba8_into(
        &self,
        py: Python<'_>,
        buffer: &Bound<'_, PyAny>,
    ) -> PyResult<Py<PyAny>> {
        let py_buffer = PyBuffer::<u8>::get(buffer).map_err(|error| {
            invalid_argument_error(format!("expected writable contiguous u8 buffer: {error}"))
        })?;
        let Some(cells) = py_buffer.as_mut_slice(py) else {
            return Err(invalid_argument_error(
                "expected writable contiguous u8 buffer",
            ));
        };
        let data = if cells.is_empty() {
            std::ptr::null_mut()
        } else {
            cells.as_ptr().cast::<u8>().cast_mut()
        };
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        // SAFETY: data points to the writable contiguous Python buffer borrowed
        // above for cells.len() u8 elements, or is null when the buffer is empty.
        let info = read_texture_image_raw(state.native(), data, cells.len())?;
        // An empty destination reaches native as the null pointer and zero
        // capacity of a size probe, which succeeds without copying, so report
        // the buffer as too small unless the frame carries no bytes.
        if cells.is_empty() && info.byte_length > 0 {
            return Err(invalid_argument_error(format!(
                "buffer length 0 is smaller than the required {} bytes",
                info.byte_length
            )));
        }
        texture_image_info_to_py(py, info)
    }

    fn acquire_metal_owned_texture_frame(
        &self,
        py: Python<'_>,
    ) -> PyResult<Py<MetalOwnedTextureFrameHandle>> {
        let session = Arc::clone(&self.state);
        let guard_session = Arc::clone(&session);
        let mut state = session
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let mut raw = empty_metal_owned_texture_frame();
        // SAFETY: raw points to initialized writable frame storage, and the C
        // API validates the session pointer and texture-session state.
        maplibre_core::check(unsafe {
            sys::mln_metal_owned_texture_acquire_frame(state.native(), &mut raw)
        })
        .map_err(map_error)?;
        state.frame_acquired = true;
        let raw = MetalOwnedTextureFrameRaw::from_native(&raw);
        let mut guard = OwnedTextureFrameAcquisitionGuard::metal(guard_session, raw);
        drop(state);
        let frame = Py::new(
            py,
            MetalOwnedTextureFrameHandle {
                session,
                raw,
                closed: Mutex::new(false),
            },
        )?;
        guard.disarm();
        Ok(frame)
    }

    fn acquire_vulkan_owned_texture_frame(
        &self,
        py: Python<'_>,
    ) -> PyResult<Py<VulkanOwnedTextureFrameHandle>> {
        let session = Arc::clone(&self.state);
        let guard_session = Arc::clone(&session);
        let mut state = session
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let mut raw = empty_vulkan_owned_texture_frame();
        // SAFETY: raw points to initialized writable frame storage, and the C
        // API validates the session pointer and texture-session state.
        maplibre_core::check(unsafe {
            sys::mln_vulkan_owned_texture_acquire_frame(state.native(), &mut raw)
        })
        .map_err(map_error)?;
        state.frame_acquired = true;
        let raw = VulkanOwnedTextureFrameRaw::from_native(&raw);
        let mut guard = OwnedTextureFrameAcquisitionGuard::vulkan(guard_session, raw);
        drop(state);
        let frame = Py::new(
            py,
            VulkanOwnedTextureFrameHandle {
                session,
                raw,
                closed: Mutex::new(false),
            },
        )?;
        guard.disarm();
        Ok(frame)
    }

    fn acquire_opengl_owned_texture_frame(
        &self,
        py: Python<'_>,
    ) -> PyResult<Py<OpenGLOwnedTextureFrameHandle>> {
        let session = Arc::clone(&self.state);
        let guard_session = Arc::clone(&session);
        let mut state = session
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.ensure_no_frame_acquired()?;
        let mut raw = empty_opengl_owned_texture_frame();
        // SAFETY: raw points to initialized writable frame storage, and the C
        // API validates the session pointer and texture-session state.
        maplibre_core::check(unsafe {
            sys::mln_opengl_owned_texture_acquire_frame(state.native(), &mut raw)
        })
        .map_err(map_error)?;
        state.frame_acquired = true;
        let raw = OpenGLOwnedTextureFrameRaw::from_native(&raw);
        let mut guard = OwnedTextureFrameAcquisitionGuard::opengl(guard_session, raw);
        drop(state);
        let frame = Py::new(
            py,
            OpenGLOwnedTextureFrameHandle {
                session,
                raw,
                closed: Mutex::new(false),
            },
        )?;
        guard.disarm();
        Ok(frame)
    }

    #[getter]
    fn closed(&self) -> bool {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .is_closed()
    }

    #[getter]
    fn detached(&self) -> bool {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .detached
    }
}

#[pymethods]
impl DetachedRenderSessionHandle {
    fn close(&self) -> PyResult<()> {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if state.is_closed() {
            return Ok(());
        }
        state.ensure_no_frame_acquired()?;
        // SAFETY: state owns an mln_render_session handle created by an attach
        // function and pairs it with the matching status-returning destroy.
        unsafe { state.handle.close_status(sys::mln_render_session_destroy) }.map_err(map_error)
    }

    #[getter]
    fn closed(&self) -> bool {
        self.state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .is_closed()
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
impl WakeSource {
    fn signal(&self) -> PyResult<()> {
        let handle = self
            .handle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let Some(handle) = *handle else {
            return Err(invalid_state_error("wake source is closed"));
        };
        // SAFETY: handle came from a successful acquire and is still owned by
        // this wrapper, and the C API accepts signals from any thread.
        maplibre_core::check(unsafe { sys::mln_wake_source_signal(handle) }).map_err(map_error)
    }

    fn close(&self) {
        let mut handle = self
            .handle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let Some(handle) = handle.take() else {
            return;
        };
        // SAFETY: handle came from a successful acquire, the mutex makes this
        // the only close, and the C API accepts destruction from any thread.
        unsafe { sys::mln_wake_source_destroy(handle) };
    }

    #[getter]
    fn closed(&self) -> bool {
        self.handle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .is_none()
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
        maplibre_core::resource::resource_provider_descriptor(
            Some(resource_provider_trampoline),
            ptr::from_ref(self).cast_mut().cast::<c_void>(),
        )
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
        maplibre_core::resource::resource_transform_descriptor(
            Some(resource_transform_trampoline),
            ptr::from_ref(self).cast_mut().cast::<c_void>(),
        )
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
        maplibre_core::resource::http_header_transform_descriptor(
            Some(http_header_transform_trampoline),
            ptr::from_ref(self).cast_mut().cast::<c_void>(),
        )
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
    remaining_count: usize,
) -> PyResult<Py<PyAny>> {
    let list = PyList::empty(py);
    for event in events {
        list.append(event_to_py(py, event)?)?;
    }
    let dict = PyDict::new(py);
    dict.set_item("events", list)?;
    dict.set_item("remaining_count", remaining_count)?;
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
    synchronous_update: Option<bool>,
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
    if let Some(synchronous_update) = synchronous_update {
        options.synchronous_update = Some(synchronous_update);
    }
    Ok(options)
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

fn feature_state_selector_from_parts(
    source_id: String,
    source_layer_id: Option<String>,
    feature_id: Option<String>,
    state_key: Option<String>,
) -> PyResult<maplibre_core::FeatureStateSelector> {
    let mut selector = maplibre_core::FeatureStateSelector::new(source_id);
    if let Some(source_layer_id) = source_layer_id {
        selector = selector.with_source_layer_id(source_layer_id);
    }
    if let Some(feature_id) = feature_id {
        selector = selector.with_feature_id(feature_id);
    }
    if let Some(state_key) = state_key {
        selector = selector.with_state_key(state_key).map_err(map_error)?;
    }
    Ok(selector)
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

fn style_image_to_py(
    py: Python<'_>,
    info: &sys::mln_style_image_info,
    pixels: &[u8],
) -> PyResult<Py<PyAny>> {
    let dict = PyDict::new(py);
    dict.set_item("info", style_image_info_to_py(py, info)?)?;
    dict.set_item("data", PyBytes::new(py, pixels))?;
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

fn probe_texture_image_info(
    session: sys::mln_render_session,
) -> PyResult<maplibre_core::TextureImageInfo> {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut info = unsafe { sys::mln_texture_image_info_default() };
    // SAFETY: The C API validates session. Null data and zero capacity are the
    // documented metadata probe path, with info pointing to initialized storage.
    let status = unsafe {
        sys::mln_texture_read_premultiplied_rgba8(session, std::ptr::null_mut(), 0, &mut info)
    };
    if status == sys::MLN_STATUS_OK
        || (status == sys::MLN_STATUS_INVALID_ARGUMENT && info.byte_length > 0)
    {
        Ok(maplibre_core::values::texture_image_info_from_native(&info))
    } else {
        Err(map_error(Error::from_status(status)))
    }
}

fn read_texture_image_raw(
    session: sys::mln_render_session,
    data: *mut u8,
    capacity: usize,
) -> PyResult<maplibre_core::TextureImageInfo> {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut info = unsafe { sys::mln_texture_image_info_default() };
    // SAFETY: The caller guarantees data points to capacity writable bytes or
    // is null for an empty buffer. The C API validates session and capacity.
    maplibre_core::check(unsafe {
        sys::mln_texture_read_premultiplied_rgba8(session, data, capacity, &mut info)
    })
    .map_err(map_error)?;
    Ok(maplibre_core::values::texture_image_info_from_native(&info))
}

fn read_texture_image_into(
    session: sys::mln_render_session,
    data: &mut [u8],
) -> PyResult<maplibre_core::TextureImageInfo> {
    let data_ptr = if data.is_empty() {
        std::ptr::null_mut()
    } else {
        data.as_mut_ptr()
    };
    read_texture_image_raw(session, data_ptr, data.len())
}

impl MetalOwnedTextureFrameRaw {
    fn from_native(raw: &sys::mln_metal_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            texture_address: raw.texture as usize,
            device_address: raw.device as usize,
            pixel_format: raw.pixel_format,
        }
    }

    fn to_native(self) -> sys::mln_metal_owned_texture_frame {
        sys::mln_metal_owned_texture_frame {
            size: std::mem::size_of::<sys::mln_metal_owned_texture_frame>() as u32,
            generation: self.generation,
            width: self.width,
            height: self.height,
            scale_factor: self.scale_factor,
            frame_id: self.frame_id,
            texture: self.texture_address as *mut c_void,
            device: self.device_address as *mut c_void,
            pixel_format: self.pixel_format,
        }
    }
}

impl VulkanOwnedTextureFrameRaw {
    fn from_native(raw: &sys::mln_vulkan_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            image_address: raw.image as usize,
            image_view_address: raw.image_view as usize,
            device_address: raw.device as usize,
            format: raw.format,
            layout: raw.layout,
        }
    }

    fn to_native(self) -> sys::mln_vulkan_owned_texture_frame {
        sys::mln_vulkan_owned_texture_frame {
            size: std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32,
            generation: self.generation,
            width: self.width,
            height: self.height,
            scale_factor: self.scale_factor,
            frame_id: self.frame_id,
            image: self.image_address as *mut c_void,
            image_view: self.image_view_address as *mut c_void,
            device: self.device_address as *mut c_void,
            format: self.format,
            layout: self.layout,
        }
    }
}

impl OpenGLOwnedTextureFrameRaw {
    fn from_native(raw: &sys::mln_opengl_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            texture: raw.texture,
            target: raw.target,
            internal_format: raw.internal_format,
            format: raw.format,
            type_: raw.type_,
        }
    }

    fn to_native(self) -> sys::mln_opengl_owned_texture_frame {
        sys::mln_opengl_owned_texture_frame {
            size: std::mem::size_of::<sys::mln_opengl_owned_texture_frame>() as u32,
            generation: self.generation,
            width: self.width,
            height: self.height,
            scale_factor: self.scale_factor,
            frame_id: self.frame_id,
            texture: self.texture,
            target: self.target,
            internal_format: self.internal_format,
            format: self.format,
            type_: self.type_,
        }
    }
}

fn empty_metal_owned_texture_frame() -> sys::mln_metal_owned_texture_frame {
    sys::mln_metal_owned_texture_frame {
        size: std::mem::size_of::<sys::mln_metal_owned_texture_frame>() as u32,
        generation: 0,
        width: 0,
        height: 0,
        scale_factor: 0.0,
        frame_id: 0,
        texture: std::ptr::null_mut(),
        device: std::ptr::null_mut(),
        pixel_format: 0,
    }
}

fn empty_vulkan_owned_texture_frame() -> sys::mln_vulkan_owned_texture_frame {
    sys::mln_vulkan_owned_texture_frame {
        size: std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32,
        generation: 0,
        width: 0,
        height: 0,
        scale_factor: 0.0,
        frame_id: 0,
        image: std::ptr::null_mut(),
        image_view: std::ptr::null_mut(),
        device: std::ptr::null_mut(),
        format: 0,
        layout: 0,
    }
}

fn empty_opengl_owned_texture_frame() -> sys::mln_opengl_owned_texture_frame {
    sys::mln_opengl_owned_texture_frame {
        size: std::mem::size_of::<sys::mln_opengl_owned_texture_frame>() as u32,
        generation: 0,
        width: 0,
        height: 0,
        scale_factor: 0.0,
        frame_id: 0,
        texture: 0,
        target: 0,
        internal_format: 0,
        format: 0,
        type_: 0,
    }
}

impl OwnedTextureFrameAcquisitionGuard {
    fn metal(session: Arc<Mutex<RenderSessionState>>, raw: MetalOwnedTextureFrameRaw) -> Self {
        Self {
            session,
            frame: Some(OwnedTextureFrameRelease::Metal(raw)),
        }
    }

    fn vulkan(session: Arc<Mutex<RenderSessionState>>, raw: VulkanOwnedTextureFrameRaw) -> Self {
        Self {
            session,
            frame: Some(OwnedTextureFrameRelease::Vulkan(raw)),
        }
    }

    fn opengl(session: Arc<Mutex<RenderSessionState>>, raw: OpenGLOwnedTextureFrameRaw) -> Self {
        Self {
            session,
            frame: Some(OwnedTextureFrameRelease::OpenGL(raw)),
        }
    }

    fn disarm(&mut self) {
        self.frame = None;
    }
}

impl Drop for OwnedTextureFrameAcquisitionGuard {
    fn drop(&mut self) {
        let Some(frame) = self.frame.take() else {
            return;
        };
        let mut session = self
            .session
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        match frame {
            OwnedTextureFrameRelease::Metal(raw) => {
                let raw = raw.to_native();
                // SAFETY: raw reconstructs the frame returned by a successful
                // native acquire call whose Python object construction failed.
                let _ = maplibre_core::check(unsafe {
                    sys::mln_metal_owned_texture_release_frame(session.native(), &raw)
                });
            }
            OwnedTextureFrameRelease::Vulkan(raw) => {
                let raw = raw.to_native();
                // SAFETY: raw reconstructs the frame returned by a successful
                // native acquire call whose Python object construction failed.
                let _ = maplibre_core::check(unsafe {
                    sys::mln_vulkan_owned_texture_release_frame(session.native(), &raw)
                });
            }
            OwnedTextureFrameRelease::OpenGL(raw) => {
                let raw = raw.to_native();
                // SAFETY: raw reconstructs the frame returned by a successful
                // native acquire call whose Python object construction failed.
                let _ = maplibre_core::check(unsafe {
                    sys::mln_opengl_owned_texture_release_frame(session.native(), &raw)
                });
            }
        }
        session.frame_acquired = false;
    }
}

#[pymethods]
impl MetalOwnedTextureFrameHandle {
    fn close(&self) -> PyResult<()> {
        let mut closed = self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if *closed {
            return Ok(());
        }
        let mut session = self
            .session
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let raw = self.raw.to_native();
        // SAFETY: raw reconstructs the frame returned by the successful native
        // acquire call for this session and has not been released yet.
        maplibre_core::check(unsafe {
            sys::mln_metal_owned_texture_release_frame(session.native(), &raw)
        })
        .map_err(map_error)?;
        session.frame_acquired = false;
        *closed = true;
        Ok(())
    }

    fn frame(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        if *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            return Err(invalid_state_error(
                "MetalOwnedTextureFrameHandle is closed",
            ));
        }
        let dict = PyDict::new(py);
        dict.set_item("generation", self.raw.generation)?;
        dict.set_item("width", self.raw.width)?;
        dict.set_item("height", self.raw.height)?;
        dict.set_item("scale_factor", self.raw.scale_factor)?;
        dict.set_item("frame_id", self.raw.frame_id)?;
        dict.set_item("pixel_format", self.raw.pixel_format)?;
        Ok(dict.into_any().unbind())
    }

    fn texture_address(&self) -> PyResult<usize> {
        if *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            Err(invalid_state_error(
                "MetalOwnedTextureFrameHandle is closed",
            ))
        } else {
            Ok(self.raw.texture_address)
        }
    }

    fn device_address(&self) -> PyResult<usize> {
        if *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            Err(invalid_state_error(
                "MetalOwnedTextureFrameHandle is closed",
            ))
        } else {
            Ok(self.raw.device_address)
        }
    }

    #[getter]
    fn closed(&self) -> bool {
        *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
    }
}

impl Drop for MetalOwnedTextureFrameHandle {
    fn drop(&mut self) {
        // Python finalization may run off the owner thread, so frame release is
        // explicit through close().
    }
}

#[pymethods]
impl VulkanOwnedTextureFrameHandle {
    fn close(&self) -> PyResult<()> {
        let mut closed = self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if *closed {
            return Ok(());
        }
        let mut session = self
            .session
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let raw = self.raw.to_native();
        // SAFETY: raw reconstructs the frame returned by the successful native
        // acquire call for this session and has not been released yet.
        maplibre_core::check(unsafe {
            sys::mln_vulkan_owned_texture_release_frame(session.native(), &raw)
        })
        .map_err(map_error)?;
        session.frame_acquired = false;
        *closed = true;
        Ok(())
    }

    fn frame(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        if *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            return Err(invalid_state_error(
                "VulkanOwnedTextureFrameHandle is closed",
            ));
        }
        let dict = PyDict::new(py);
        dict.set_item("generation", self.raw.generation)?;
        dict.set_item("width", self.raw.width)?;
        dict.set_item("height", self.raw.height)?;
        dict.set_item("scale_factor", self.raw.scale_factor)?;
        dict.set_item("frame_id", self.raw.frame_id)?;
        dict.set_item("format", self.raw.format)?;
        dict.set_item("layout", self.raw.layout)?;
        Ok(dict.into_any().unbind())
    }

    fn image_address(&self) -> PyResult<usize> {
        if *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            Err(invalid_state_error(
                "VulkanOwnedTextureFrameHandle is closed",
            ))
        } else {
            Ok(self.raw.image_address)
        }
    }

    fn image_view_address(&self) -> PyResult<usize> {
        if *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            Err(invalid_state_error(
                "VulkanOwnedTextureFrameHandle is closed",
            ))
        } else {
            Ok(self.raw.image_view_address)
        }
    }

    fn device_address(&self) -> PyResult<usize> {
        if *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            Err(invalid_state_error(
                "VulkanOwnedTextureFrameHandle is closed",
            ))
        } else {
            Ok(self.raw.device_address)
        }
    }

    #[getter]
    fn closed(&self) -> bool {
        *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
    }
}

impl Drop for VulkanOwnedTextureFrameHandle {
    fn drop(&mut self) {
        // Python finalization may run off the owner thread, so frame release is
        // explicit through close().
    }
}

#[pymethods]
impl OpenGLOwnedTextureFrameHandle {
    fn close(&self) -> PyResult<()> {
        let mut closed = self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if *closed {
            return Ok(());
        }
        let mut session = self
            .session
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let raw = self.raw.to_native();
        // SAFETY: raw reconstructs the frame returned by the successful native
        // acquire call for this session and has not been released yet.
        maplibre_core::check(unsafe {
            sys::mln_opengl_owned_texture_release_frame(session.native(), &raw)
        })
        .map_err(map_error)?;
        session.frame_acquired = false;
        *closed = true;
        Ok(())
    }

    fn frame(&self, py: Python<'_>) -> PyResult<Py<PyAny>> {
        if *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            return Err(invalid_state_error(
                "OpenGLOwnedTextureFrameHandle is closed",
            ));
        }
        let dict = PyDict::new(py);
        dict.set_item("generation", self.raw.generation)?;
        dict.set_item("width", self.raw.width)?;
        dict.set_item("height", self.raw.height)?;
        dict.set_item("scale_factor", self.raw.scale_factor)?;
        dict.set_item("frame_id", self.raw.frame_id)?;
        dict.set_item("target", self.raw.target)?;
        dict.set_item("internal_format", self.raw.internal_format)?;
        dict.set_item("format", self.raw.format)?;
        dict.set_item("type", self.raw.type_)?;
        Ok(dict.into_any().unbind())
    }

    fn texture(&self) -> PyResult<u32> {
        if *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
        {
            Err(invalid_state_error(
                "OpenGLOwnedTextureFrameHandle is closed",
            ))
        } else {
            Ok(self.raw.texture)
        }
    }

    #[getter]
    fn closed(&self) -> bool {
        *self
            .closed
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
    }
}

impl Drop for OpenGLOwnedTextureFrameHandle {
    fn drop(&mut self) {
        // Python finalization may run off the owner thread, so frame release is
        // explicit through close().
    }
}

fn map_error(error: Error) -> PyErr {
    let raw_status = error.raw_status();
    let diagnostic = error.diagnostic().to_owned();
    match error.kind() {
        ErrorKind::InvalidArgument => {
            py_errors::InvalidArgumentError::new_err((raw_status, diagnostic))
        }
        ErrorKind::InvalidState => py_errors::InvalidStateError::new_err((raw_status, diagnostic)),
        ErrorKind::WrongThread => py_errors::WrongThreadError::new_err((raw_status, diagnostic)),
        ErrorKind::Unsupported => {
            py_errors::UnsupportedFeatureError::new_err((raw_status, diagnostic))
        }
        ErrorKind::NativeError => py_errors::NativeError::new_err((raw_status, diagnostic)),
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

/// Test helper that calls the C size accessor with a raw map id, which the safe
/// API cannot express: a test can replay a released id or one from another
/// thread.
#[pyfunction]
fn map_size_by_id_for_test(py: Python<'_>, id: u64) -> PyResult<(u32, u32, f64)> {
    py.detach(|| {
        let mut width = 0u32;
        let mut height = 0u32;
        let mut scale_factor = 0.0f64;
        // SAFETY: the id is well-formed and the out-pointers are live locals;
        // the C API resolves the id rather than dereferencing it.
        let status = unsafe {
            sys::mln_map_get_size(sys::mln_map(id), &mut width, &mut height, &mut scale_factor)
        };
        if status == sys::MLN_STATUS_OK {
            Ok((width, height, scale_factor))
        } else {
            Err(map_error(Error::from_status_and_diagnostic(
                status,
                maplibre_native_ffi_core::error::capture_thread_diagnostic(),
            )))
        }
    })
}

/// Test helper that passes a map id where a runtime id belongs. The two are
/// distinct newtypes, so this call has no expression in the safe API.
#[pyfunction]
fn pump_runtime_with_map_id_for_test(py: Python<'_>, id: u64) -> PyResult<()> {
    py.detach(|| {
        // SAFETY: the value is well-formed; the C API rejects it on its kind tag.
        let status = unsafe { sys::mln_runtime_pump(sys::mln_runtime(id), 0) };
        if status == sys::MLN_STATUS_OK {
            Ok(())
        } else {
            Err(map_error(Error::from_status_and_diagnostic(
                status,
                maplibre_native_ffi_core::error::capture_thread_diagnostic(),
            )))
        }
    })
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
    actual_abi_version: u32,
    asset_path: Option<String>,
    cache_path: Option<String>,
) -> PyResult<RuntimeHandle> {
    maplibre_core::validate_abi_version_value(actual_abi_version).map_err(map_error)?;
    create_runtime(
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
            let offset = messages.len() as u32;
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
        remaining_count: 3,
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
    event_batch_to_py(py, copied, batch.remaining_count)
}

/// Reports the event stride a drain of `runtime` names and the stride this
/// extension compiled against, so the Python suite can assert the decode steps
/// by the batch's own value.
#[pyfunction]
fn runtime_event_stride_for_test(runtime: &RuntimeHandle) -> PyResult<(u32, u32)> {
    let state = runtime.state_for_operation()?;
    let mut batch = sys::mln_event_batch(0);
    maplibre_core::check(unsafe { sys::mln_runtime_drain_events(state.handle(), 0, &mut batch) })
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
/// Probes the required byte length, then copies the layer text into a `String`.
///
/// # Safety
///
/// `copy` must forward its arguments to a C entry point that writes at most
/// `capacity` bytes through the text pointer and the required length through the
/// size pointer.
unsafe fn copy_text(
    copy: impl Fn(*mut c_char, usize, *mut usize) -> sys::mln_status,
) -> PyResult<String> {
    let mut required = 0;
    maplibre_core::check(copy(std::ptr::null_mut(), 0, &mut required)).map_err(map_error)?;
    if required == 0 {
        return Ok(String::new());
    }

    let mut bytes = vec![0u8; required];
    let mut copied = 0;
    maplibre_core::check(copy(
        bytes.as_mut_ptr().cast::<c_char>(),
        bytes.len(),
        &mut copied,
    ))
    .map_err(map_error)?;
    bytes.truncate(copied.min(bytes.len()));
    String::from_utf8(bytes)
        .map_err(|error| invalid_argument_error(format!("native text is not UTF-8: {error}")))
}

unsafe fn copy_bytes(
    py: Python<'_>,
    copy: impl Fn(*mut u8, usize, *mut usize) -> sys::mln_status,
) -> PyResult<Py<PyBytes>> {
    let mut required = 0;
    maplibre_core::check(copy(ptr::null_mut(), 0, &mut required)).map_err(map_error)?;
    let mut bytes = vec![0u8; required];
    let mut copied = 0;
    maplibre_core::check(copy(
        if bytes.is_empty() {
            ptr::null_mut()
        } else {
            bytes.as_mut_ptr()
        },
        bytes.len(),
        &mut copied,
    ))
    .map_err(map_error)?;
    bytes.truncate(copied.min(bytes.len()));
    Ok(PyBytes::new(py, &bytes).unbind())
}

fn owned_buffer_to_py(py: Python<'_>, buffer: sys::mln_buffer) -> PyResult<Py<PyBytes>> {
    // SAFETY: The buffer is an owned handle returned by the C API.
    let bytes = unsafe { maplibre_core::string::copy_owned_buffer(buffer) }.map_err(map_error)?;
    Ok(PyBytes::new(py, &bytes).unbind())
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

/// Creates a runtime handle on the current thread.
#[pyfunction]
fn create_runtime(
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
    if let Err(error) =
        maplibre_core::check(unsafe { sys::mln_runtime_create(&raw_options, out.as_mut_ptr()) })
    {
        let _ = unsafe { sys::mln_notification_source_close(source) };
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
        resource_provider: Mutex::new(None),
        resource_transform: Mutex::new(None),
        http_header_transform: Mutex::new(None),
    })
}

/// Creates a map handle owned by a runtime.
#[pyfunction]
fn create_map(
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
    let mut out = maplibre_core::ptr::OutHandle::<sys::mln_map>::new();
    maplibre_core::check(unsafe {
        sys::mln_map_create(runtime_state.handle(), &raw_options, out.as_mut_ptr())
    })
    .map_err(map_error)?;
    let native = out.into_live("mln_map").map_err(map_error)?;
    let state = unsafe { maplibre_core::handle::NativeHandleState::from_handle(native, "mln_map") }
        .map_err(map_error)?;
    Ok(MapHandle {
        state: Mutex::new(state),
    })
}

fn attach_render_session<F>(map: &MapHandle, attach: F) -> PyResult<RenderSessionHandle>
where
    F: FnOnce(sys::mln_map, *mut sys::mln_render_session) -> sys::mln_status,
{
    let map_state = map.state();
    let mut out = maplibre_core::ptr::OutHandle::<sys::mln_render_session>::new();
    maplibre_core::check(attach(map_state.handle(), out.as_mut_ptr())).map_err(map_error)?;
    let native = out.into_live("mln_render_session").map_err(map_error)?;
    Ok(RenderSessionHandle {
        state: Arc::new(Mutex::new(RenderSessionState::new(native)?)),
    })
}

#[pyfunction]
fn attach_metal_surface(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    device_address: usize,
    layer_address: usize,
) -> PyResult<RenderSessionHandle> {
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
    attach_render_session(map, |map_ptr, out| {
        // SAFETY: descriptor is fully initialized and lives for this call. The C
        // API validates the map pointer, descriptor fields, and out pointer.
        unsafe { sys::mln_metal_surface_attach(map_ptr, &descriptor, out) }
    })
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
) -> PyResult<RenderSessionHandle> {
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
    attach_render_session(map, |map_ptr, out| {
        // SAFETY: descriptor is fully initialized and lives for this call. The C
        // API validates the map pointer, descriptor fields, and out pointer.
        unsafe { sys::mln_vulkan_surface_attach(map_ptr, &descriptor, out) }
    })
}

#[pyfunction]
fn attach_metal_owned_texture(
    map: &MapHandle,
    width: u32,
    height: u32,
    scale_factor: f64,
    device_address: usize,
) -> PyResult<RenderSessionHandle> {
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
    attach_render_session(map, |map_ptr, out| {
        // SAFETY: descriptor is fully initialized and lives for this call. The C
        // API validates the map pointer, descriptor fields, and out pointer.
        unsafe { sys::mln_metal_owned_texture_attach(map_ptr, &descriptor, out) }
    })
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
) -> PyResult<RenderSessionHandle> {
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
    attach_render_session(map, |map_ptr, out| {
        // SAFETY: descriptor is fully initialized and lives for this call. The C
        // API validates the map pointer, descriptor fields, and out pointer.
        unsafe { sys::mln_metal_borrowed_texture_attach(map_ptr, &descriptor, out) }
    })
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
) -> PyResult<RenderSessionHandle> {
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
    attach_render_session(map, |map_ptr, out| {
        // SAFETY: descriptor is fully initialized and lives for this call. The C
        // API validates the map pointer, descriptor fields, and out pointer.
        unsafe { sys::mln_vulkan_owned_texture_attach(map_ptr, &descriptor, out) }
    })
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
) -> PyResult<RenderSessionHandle> {
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
    attach_render_session(map, |map_ptr, out| {
        // SAFETY: descriptor is fully initialized and lives for this call. The C
        // API validates the map pointer, descriptor fields, and out pointer.
        unsafe { sys::mln_vulkan_borrowed_texture_attach(map_ptr, &descriptor, out) }
    })
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
) -> PyResult<RenderSessionHandle> {
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
    attach_render_session(map, |map_ptr, out| {
        // SAFETY: descriptor is fully initialized and lives for this call. The C
        // API validates the map pointer, descriptor fields, and out pointer.
        unsafe { sys::mln_opengl_surface_attach(map_ptr, &descriptor, out) }
    })
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
) -> PyResult<RenderSessionHandle> {
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
    attach_render_session(map, |map_ptr, out| {
        // SAFETY: descriptor is fully initialized and lives for this call. The C
        // API validates the map pointer, descriptor fields, and out pointer.
        unsafe { sys::mln_opengl_owned_texture_attach(map_ptr, &descriptor, out) }
    })
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
) -> PyResult<RenderSessionHandle> {
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
    attach_render_session(map, |map_ptr, out| {
        // SAFETY: descriptor is fully initialized and lives for this call. The C
        // API validates the map pointer, descriptor fields, and out pointer.
        unsafe { sys::mln_opengl_borrowed_texture_attach(map_ptr, &descriptor, out) }
    })
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
    module.add_class::<ResourceRequestHandle>()?;
    module.add_class::<WakeSource>()?;
    module.add_class::<LogReceiver>()?;
    module.add_class::<CustomGeometrySourceHandle>()?;
    module.add_class::<RenderSessionHandle>()?;
    module.add_class::<DetachedRenderSessionHandle>()?;
    module.add_class::<MetalOwnedTextureFrameHandle>()?;
    module.add_class::<VulkanOwnedTextureFrameHandle>()?;
    module.add_class::<OpenGLOwnedTextureFrameHandle>()?;
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
    module.add_function(wrap_pyfunction!(pump_runtime_with_map_id_for_test, module)?)?;
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
    Ok(())
}
