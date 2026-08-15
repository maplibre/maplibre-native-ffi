use std::fmt;
use std::marker::PhantomData;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Condvar, Mutex};
use std::time::Duration;

use maplibre_core::AmbientCacheOperation;
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::events::{OfflineRegionDownloadState, OfflineRegionStatus, RuntimeEventBatch};
use crate::handle::{ConcurrentNativeHandle, closed_handle_error, out_handle};
use crate::resource::{HttpHeaderTransformState, ResourceProviderState, ResourceTransformState};
use crate::{
    Error, ErrorKind, HandleOperationError, ResourceProviderDecision, Result, RuntimeEventMask,
};
#[cfg(test)]
use crate::{LatLngBounds, MapHandle, MapOptions};

pub use maplibre_core::runtime::{OfflineRegionDefinition, OfflineRegionInfo, RuntimeOptions};
pub(crate) use maplibre_core::runtime::{
    OfflineRegionDefinitionNativeExt, RuntimeOptionsNativeExt,
};

pub(crate) fn wait_raw_operation_completed(operation: sys::mln_operation) -> Result<()> {
    (|| {
        let mut completed = false;
        // SAFETY: operation is an owned live observer and completed is writable.
        maplibre_core::check(unsafe { sys::mln_operation_wait(operation, -1, &mut completed) })?;
        if !completed {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "an unbounded operation wait returned before completion",
            ));
        }
        let mut terminal_status = sys::MLN_STATUS_OK;
        // SAFETY: operation completed and terminal_status is writable.
        maplibre_core::check(unsafe {
            sys::mln_operation_get_status(operation, &mut terminal_status)
        })?;
        if terminal_status == sys::MLN_STATUS_OK {
            Ok(())
        } else {
            let mut size = 0;
            // SAFETY: null/zero is the documented diagnostic size probe.
            maplibre_core::check(unsafe {
                sys::mln_operation_copy_diagnostic(operation, std::ptr::null_mut(), 0, &mut size)
            })?;
            let mut bytes = vec![0_u8; size];
            // SAFETY: bytes is writable for its full capacity.
            maplibre_core::check(unsafe {
                sys::mln_operation_copy_diagnostic(
                    operation,
                    bytes.as_mut_ptr().cast(),
                    bytes.len(),
                    &mut size,
                )
            })?;
            Err(Error::from_status_and_diagnostic(
                terminal_status,
                String::from_utf8_lossy(&bytes),
            ))
        }
    })()
}

struct NotificationCallbackState {
    callback: Box<dyn Fn() + Send + Sync + 'static>,
}

impl fmt::Debug for NotificationCallbackState {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("NotificationCallbackState").finish()
    }
}

unsafe extern "C" fn notification_callback(user_data: *mut std::ffi::c_void) {
    if user_data.is_null() {
        return;
    }
    // SAFETY: user_data points to the boxed state retained until native clears
    // the callback and waits for in-flight invocations.
    let state = unsafe { &*user_data.cast::<NotificationCallbackState>() };
    let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| (state.callback)()));
}

/// Kind of receiver endpoint reported ready by the runtime's notification source.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ReadyEndpointKind {
    RuntimeEvents,
    Operation,
    ResourceRequests,
    LogRecords,
    RenderFrames,
    DriverWork,
    Unknown(u32),
}

/// Copied notification endpoint ready for receiver-side service.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ReadyEndpoint {
    pub kind: ReadyEndpointKind,
    pub id: u64,
}

fn ready_endpoint_kind(raw: u32) -> ReadyEndpointKind {
    match raw {
        sys::MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS => ReadyEndpointKind::RuntimeEvents,
        sys::MLN_NOTIFICATION_ENDPOINT_OPERATION => ReadyEndpointKind::Operation,
        sys::MLN_NOTIFICATION_ENDPOINT_ADAPTER_RESOURCE_REQUESTS => {
            ReadyEndpointKind::ResourceRequests
        }
        sys::MLN_NOTIFICATION_ENDPOINT_ADAPTER_LOG_RECORDS => ReadyEndpointKind::LogRecords,
        sys::MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES => ReadyEndpointKind::RenderFrames,
        sys::MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK => ReadyEndpointKind::DriverWork,
        value => ReadyEndpointKind::Unknown(value),
    }
}

#[expect(
    clippy::vec_box,
    reason = "retired callback boxes must keep stable addresses until native releases them"
)]
#[derive(Debug)]
pub(crate) struct RuntimeState {
    handle: ConcurrentNativeHandle<sys::mln_runtime>,
    notification_source: Mutex<sys::mln_notification_source>,
    resource_transform: Mutex<Option<Box<ResourceTransformState>>>,
    retired_resource_transforms: Mutex<Vec<Box<ResourceTransformState>>>,
    http_header_transform: Mutex<Option<Box<HttpHeaderTransformState>>>,
    retired_http_header_transforms: Mutex<Vec<Box<HttpHeaderTransformState>>>,
    resource_provider: Mutex<Option<Box<ResourceProviderState>>>,
    retired_resource_providers: Mutex<Vec<Box<ResourceProviderState>>>,
    notification_callback: Mutex<Option<Box<NotificationCallbackState>>>,
    pub(crate) operations: Arc<OperationRegistry>,
}
impl RuntimeState {
    fn new(
        native: sys::mln_runtime,
        notification_source: sys::mln_notification_source,
    ) -> Result<Self> {
        // SAFETY: native came from a successful typed creation take and its
        // registry/control state supports calls from any thread.
        let handle = unsafe { ConcurrentNativeHandle::from_handle(native, "mln_runtime") }?;
        Ok(Self {
            handle,
            notification_source: Mutex::new(notification_source),
            resource_transform: Mutex::new(None),
            retired_resource_transforms: Mutex::new(Vec::new()),
            http_header_transform: Mutex::new(None),
            retired_http_header_transforms: Mutex::new(Vec::new()),
            resource_provider: Mutex::new(None),
            retired_resource_providers: Mutex::new(Vec::new()),
            notification_callback: Mutex::new(None),
            operations: Arc::new(OperationRegistry::default()),
        })
    }

    pub(crate) fn native(&self) -> Result<sys::mln_runtime> {
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("RuntimeHandle"))
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
            && self
                .notification_source
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner())
                .0
                == 0
    }

    fn close(&self) -> Result<()> {
        if self.operations.live.load(Ordering::Acquire) != 0 {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "RuntimeHandle cannot close while operation handles are live",
            ));
        }
        let runtime = self.native()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: runtime is live and operation is a null writable handle.
        maplibre_core::check(unsafe { sys::mln_runtime_close_start(runtime, &mut operation) })?;
        let result = wait_raw_operation_completed(operation);
        // SAFETY: the close observer is owned by this call.
        unsafe { sys::mln_operation_release(operation) };
        result?;
        self.handle.mark_closed();
        self.resource_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        self.http_header_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        self.resource_provider
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        self.retired_resource_transforms
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .clear();
        self.retired_http_header_transforms
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .clear();
        self.retired_resource_providers
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .clear();
        let mut source = self
            .notification_source
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if source.0 != 0 {
            // SAFETY: Runtime close completed and detached its endpoint.
            maplibre_core::check(unsafe { sys::mln_notification_source_close(*source) })?;
            *source = sys::mln_notification_source(0);
        }
        self.notification_callback
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take();
        Ok(())
    }

    fn set_resource_provider<F>(&self, callback: F) -> Result<u64>
    where
        F: Fn(crate::ResourceRequest, crate::ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        let replacement = ResourceProviderState::new(callback);
        let descriptor = replacement.descriptor();
        self.install_resource_provider(replacement, descriptor)
    }

    /// Installs a provider descriptor whose callback is null, exercising the
    /// native install-failure rollback path that public callers cannot reach.
    #[cfg(test)]
    fn set_resource_provider_with_rejected_descriptor_for_testing<F>(
        &self,
        callback: F,
    ) -> Result<u64>
    where
        F: Fn(crate::ResourceRequest, crate::ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        let replacement = ResourceProviderState::new(callback);
        let mut descriptor = replacement.descriptor();
        descriptor.callback = None;
        self.install_resource_provider(replacement, descriptor)
    }

    fn install_resource_provider(
        &self,
        replacement: Box<ResourceProviderState>,
        descriptor: sys::mln_resource_provider,
    ) -> Result<u64> {
        let runtime = self.native()?;
        let mut command_id = 0;
        // SAFETY: descriptor points to replacement, retained after acceptance.
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_resource_provider(runtime, &descriptor, &mut command_id)
        })?;
        if let Some(previous) = self
            .resource_provider
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .replace(replacement)
        {
            self.retired_resource_providers
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner())
                .push(previous);
        }
        Ok(command_id)
    }

    fn clear_resource_provider(&self) -> Result<u64> {
        let runtime = self.native()?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_clear_resource_provider(runtime, &mut command_id)
        })?;
        if let Some(previous) = self
            .resource_provider
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take()
        {
            self.retired_resource_providers
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner())
                .push(previous);
        }
        Ok(command_id)
    }

    fn set_resource_transform<F>(&self, callback: F) -> Result<u64>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        let runtime = self.native()?;
        let replacement = ResourceTransformState::new(callback);
        let descriptor = replacement.descriptor();
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_resource_transform(runtime, &descriptor, &mut command_id)
        })?;
        if let Some(previous) = self
            .resource_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .replace(replacement)
        {
            self.retired_resource_transforms
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner())
                .push(previous);
        }
        Ok(command_id)
    }

    fn clear_resource_transform(&self) -> Result<u64> {
        let runtime = self.native()?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_clear_resource_transform(runtime, &mut command_id)
        })?;
        if let Some(previous) = self
            .resource_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take()
        {
            self.retired_resource_transforms
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner())
                .push(previous);
        }
        Ok(command_id)
    }

    fn set_http_header_transform<F>(&self, callback: F) -> Result<u64>
    where
        F: Fn(crate::HttpHeaderTransformRequest) -> Vec<crate::HttpHeader> + Send + Sync + 'static,
    {
        let runtime = self.native()?;
        let replacement = HttpHeaderTransformState::new(callback);
        let descriptor = replacement.descriptor();
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_http_header_transform(runtime, &descriptor, &mut command_id)
        })?;
        if let Some(previous) = self
            .http_header_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .replace(replacement)
        {
            self.retired_http_header_transforms
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner())
                .push(previous);
        }
        Ok(command_id)
    }

    fn clear_http_header_transform(&self) -> Result<u64> {
        let runtime = self.native()?;
        let mut command_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_clear_http_header_transform(runtime, &mut command_id)
        })?;
        if let Some(previous) = self
            .http_header_transform
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take()
        {
            self.retired_http_header_transforms
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner())
                .push(previous);
        }
        Ok(command_id)
    }
}

impl Drop for RuntimeState {
    fn drop(&mut self) {
        if self.operations.live.load(Ordering::Acquire) != 0 {
            self.handle.leak_for_report();
            let source = *self
                .notification_source
                .get_mut()
                .unwrap_or_else(|poisoned| poisoned.into_inner());
            if source.0 != 0 {
                maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
                    type_name: "mln_notification_source",
                    id: source.0,
                });
            }
            return;
        }
        if self.close().is_err() {
            self.handle.leak_for_report();
            let source = *self
                .notification_source
                .get_mut()
                .unwrap_or_else(|poisoned| poisoned.into_inner());
            if source.0 != 0 {
                maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
                    type_name: "mln_notification_source",
                    id: source.0,
                });
            }
        }
    }
}

/// Any-thread runtime handle backed by a core-owned worker.
pub struct RuntimeHandle {
    pub(crate) inner: Arc<RuntimeState>,
}

impl fmt::Debug for RuntimeHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RuntimeHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

#[derive(Debug, Default)]
pub(crate) struct OperationRegistry {
    live: AtomicUsize,
}

#[derive(Debug, Default)]
struct OperationState {
    live: bool,
    closing: bool,
    result_consumed: bool,
    result_in_use: bool,
    active_uses: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum OperationKind {
    AmbientCache,
    Barrier,
    MapStillImage,
    CameraQuery,
    CameraFitBounds,
    CameraFitCoordinates,
    CameraFitGeometry,
    BoundsForCamera,
    BoundsForCameraUnwrapped,
    PixelForLatLng,
    LatLngForPixel,
    PixelsForLatLngs,
    LatLngsForPixels,
    RegionCreate,
    RegionGet,
    RegionsList,
    RegionsMergeDatabase,
    RegionUpdateMetadata,
    RegionGetStatus,
    RegionSetObserved,
    RegionSetDownloadState,
    RegionInvalidate,
    LoadedStyleJson,
    StyleLayerInfo,
    StyleUrl,
    StyleSourceInfo,
    StyleSourceAttribution,
    StyleSourceUrl,
    StyleSourceTileUrls,
    StyleSourceIds,
    ImageSourceCoordinates,
    StyleImageInfo,
    StyleImagePixels,
    StyleImageStretches,
    StyleLayerIds,
    StyleLayerJson,
    StyleLightProperty,
    StyleTransitionOptions,
    LayerProperty,
    LayerFilter,
    LayerSourceLayer,
    LayerSourceId,
    RegionDelete,
    SetMaximumAmbientCacheSize,
    RenderAttach,
    RenderResize,
    RenderBarrier,
    RenderMaintenance,
    RenderReadback,
    RenderDetach,
    RenderSetTarget,
    RenderFrameRelease,
    RenderQuery,
    RenderFeatureState,
}

/// Common asynchronous operation handle with a typed result.
pub struct OperationHandle<T> {
    operation: sys::mln_operation,
    pub(crate) operation_kind: OperationKind,
    registry: Arc<OperationRegistry>,
    state: Mutex<OperationState>,
    idle: Condvar,
    _result: PhantomData<fn() -> T>,
}

impl<T> fmt::Debug for OperationHandle<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        f.debug_struct("OperationHandle")
            .field("operation", &self.operation)
            .field("operation_kind", &self.operation_kind)
            .field("live", &state.live)
            .field("result_consumed", &state.result_consumed)
            .finish()
    }
}
impl<T> OperationHandle<T> {
    pub(crate) fn new(
        operation: sys::mln_operation,
        operation_kind: OperationKind,
        registry: Arc<OperationRegistry>,
    ) -> Result<Self> {
        if operation.0 == 0 {
            return Err(Error::invalid_argument("operation handle must not be zero"));
        }
        registry.live.fetch_add(1, Ordering::Release);
        Ok(Self {
            operation,
            operation_kind,
            registry,
            state: Mutex::new(OperationState {
                live: true,
                closing: false,
                result_consumed: false,
                active_uses: 0,
                result_in_use: false,
            }),
            idle: Condvar::new(),
            _result: PhantomData,
        })
    }

    pub(crate) fn with_operation<R>(
        &self,
        call: impl FnOnce(sys::mln_operation) -> Result<R>,
    ) -> Result<R> {
        {
            let mut state = self
                .state
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner());
            if !state.live || state.closing {
                return Err(closed_handle_error("OperationHandle"));
            }
            state.active_uses += 1;
        }
        let result = call(self.operation);
        let mut state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.active_uses -= 1;
        if state.active_uses == 0 {
            self.idle.notify_all();
        }
        result
    }

    pub(crate) fn with_result_operation<R>(
        &self,
        call: impl FnOnce(sys::mln_operation) -> Result<R>,
    ) -> Result<R> {
        {
            let mut state = self
                .state
                .lock()
                .unwrap_or_else(|poisoned| poisoned.into_inner());
            if !state.live || state.closing {
                return Err(closed_handle_error("OperationHandle"));
            }
            if state.result_consumed || state.result_in_use {
                return Err(Error::new(
                    ErrorKind::InvalidState,
                    None,
                    "operation result was already taken, discarded, or is being consumed",
                ));
            }
            state.result_in_use = true;
            state.active_uses += 1;
        }
        let result = call(self.operation);
        let mut state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        state.result_in_use = false;
        state.active_uses -= 1;
        if result.is_ok() {
            state.result_consumed = true;
        }
        if state.active_uses == 0 {
            self.idle.notify_all();
        }
        result
    }
    fn release_native(&self) {
        let mut state = self
            .state
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if !state.live {
            return;
        }
        state.closing = true;
        while state.active_uses != 0 {
            state = self
                .idle
                .wait(state)
                .unwrap_or_else(|poisoned| poisoned.into_inner());
        }
        state.live = false;
        drop(state);
        // SAFETY: This wrapper owns one live public observer and no native call
        // is still using it.
        unsafe { sys::mln_operation_release(self.operation) };
        self.registry.live.fetch_sub(1, Ordering::Release);
    }

    /// Reports whether the operation reached a terminal disposition.
    pub fn is_completed(&self) -> Result<bool> {
        self.with_operation(|operation| {
            let mut completed = false;
            // SAFETY: The leased operation is live and completed is writable.
            maplibre_core::check(unsafe { sys::mln_operation_poll(operation, &mut completed) })?;
            Ok(completed)
        })
    }

    /// Waits up to `timeout` for a terminal disposition.
    pub fn wait(&self, timeout: Duration) -> Result<bool> {
        let timeout_ms = i64::try_from(timeout.as_millis()).unwrap_or(i64::MAX);
        self.with_operation(|operation| {
            let mut completed = false;
            // SAFETY: The leased operation is live and completed is writable.
            maplibre_core::check(unsafe {
                sys::mln_operation_wait(operation, timeout_ms, &mut completed)
            })?;
            Ok(completed)
        })
    }

    /// Requests cancellation of this operation.
    pub fn cancel(&self) -> Result<()> {
        self.with_operation(|operation| {
            // SAFETY: The leased operation is live.
            maplibre_core::check(unsafe { sys::mln_operation_cancel(operation) })
        })
    }

    /// Copies the terminal native status.
    pub fn terminal_status(&self) -> Result<i32> {
        self.with_operation(|operation| {
            let mut status = sys::MLN_STATUS_OK;
            // SAFETY: The leased operation is live and status is writable.
            maplibre_core::check(unsafe { sys::mln_operation_get_status(operation, &mut status) })?;
            Ok(status)
        })
    }

    /// Copies the terminal diagnostic text.
    pub fn diagnostic(&self) -> Result<String> {
        self.with_operation(|operation| {
            let mut size = 0;
            // SAFETY: A null output with zero capacity is the documented size probe.
            maplibre_core::check(unsafe {
                sys::mln_operation_copy_diagnostic(operation, std::ptr::null_mut(), 0, &mut size)
            })?;
            let mut bytes = vec![0_u8; size];
            // SAFETY: bytes has `size` writable bytes and the operation is leased.
            maplibre_core::check(unsafe {
                sys::mln_operation_copy_diagnostic(
                    operation,
                    bytes.as_mut_ptr().cast(),
                    bytes.len(),
                    &mut size,
                )
            })?;
            String::from_utf8(bytes).map_err(|_| {
                Error::new(
                    ErrorKind::NativeError,
                    None,
                    "operation diagnostic is not UTF-8",
                )
            })
        })
    }

    /// Discards an untaken terminal result while retaining the observer.
    pub fn discard(&self) -> Result<()> {
        self.with_result_operation(|operation| {
            // SAFETY: The leased operation is live.
            maplibre_core::check(unsafe { sys::mln_operation_discard_result(operation) })
        })
    }

    /// Releases this operation observer.
    pub fn release(self) {
        self.release_native();
    }
}

impl<T> Drop for OperationHandle<T> {
    fn drop(&mut self) {
        self.release_native();
    }
}

impl OperationHandle<OfflineRegionInfo> {
    /// Takes a completed create/update result while retaining the observer.
    pub fn take(&self) -> Result<OfflineRegionInfo> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_snapshot>::new();
        self.with_result_operation(|operation| {
            let status = match self.operation_kind {
                OperationKind::RegionCreate => unsafe {
                    sys::mln_runtime_offline_region_create_take_result(operation, out.as_mut_ptr())
                },
                OperationKind::RegionUpdateMetadata => unsafe {
                    sys::mln_runtime_offline_region_update_metadata_take_result(
                        operation,
                        out.as_mut_ptr(),
                    )
                },
                _ => sys::MLN_STATUS_INVALID_STATE,
            };
            maplibre_core::check(status)
        })?;
        let snapshot = out.into_live("mln_offline_region_snapshot")?;
        // SAFETY: On success, the C API returns an owned snapshot handle;
        // core copies and releases it.
        unsafe { maplibre_core::runtime::copy_offline_region_snapshot(snapshot) }
    }
}

impl OperationHandle<Option<OfflineRegionInfo>> {
    /// Takes a completed get result while retaining the observer.
    pub fn take(&self) -> Result<Option<OfflineRegionInfo>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_snapshot>::new();
        let mut found = false;
        self.with_result_operation(|operation| {
            // SAFETY: The leased operation and writable outputs remain live.
            maplibre_core::check(unsafe {
                sys::mln_runtime_offline_region_get_take_result(
                    operation,
                    out.as_mut_ptr(),
                    &mut found,
                )
            })
        })?;
        if !found {
            return Ok(None);
        }
        let snapshot = out.into_live("mln_offline_region_snapshot")?;
        // SAFETY: When found is true, the C API returns an owned snapshot
        // handle; core copies and releases it.
        Ok(Some(unsafe {
            maplibre_core::runtime::copy_offline_region_snapshot(snapshot)?
        }))
    }
}

impl OperationHandle<Vec<OfflineRegionInfo>> {
    /// Takes a completed list/merge result while retaining the observer.
    pub fn take(&self) -> Result<Vec<OfflineRegionInfo>> {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_list>::new();
        self.with_result_operation(|operation| {
            let status = match self.operation_kind {
                OperationKind::RegionsList => unsafe {
                    sys::mln_runtime_offline_regions_list_take_result(operation, out.as_mut_ptr())
                },
                OperationKind::RegionsMergeDatabase => unsafe {
                    sys::mln_runtime_offline_regions_merge_database_take_result(
                        operation,
                        out.as_mut_ptr(),
                    )
                },
                _ => sys::MLN_STATUS_INVALID_STATE,
            };
            maplibre_core::check(status)
        })?;
        let list = out.into_live("mln_offline_region_list")?;
        // SAFETY: On success, the C API returns an owned list handle; core
        // copies and releases it.
        unsafe { maplibre_core::runtime::copy_offline_region_list(list) }
    }
}

impl OperationHandle<OfflineRegionStatus> {
    /// Takes a completed status result while retaining the observer.
    pub fn take(&self) -> Result<OfflineRegionStatus> {
        let mut raw = maplibre_core::events::empty_offline_region_status_native();
        self.with_result_operation(|operation| {
            // SAFETY: The leased operation is live and raw is writable.
            maplibre_core::check(unsafe {
                sys::mln_runtime_offline_region_get_status_take_result(operation, &mut raw)
            })
        })?;
        Ok(maplibre_core::events::offline_region_status_from_native(
            raw,
        ))
    }
}

impl OperationHandle<crate::CameraSnapshot> {
    /// Takes the completed ordered camera result exactly once.
    pub fn take(&self) -> Result<crate::CameraSnapshot> {
        if self.operation_kind != OperationKind::CameraQuery {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "operation does not contain a camera query result",
            ));
        }
        let mut raw = sys::mln_camera_query_result {
            size: std::mem::size_of::<sys::mln_camera_query_result>() as u32,
            reserved: 0,
            generation: 0,
            // SAFETY: the constructor initializes this ABI version's descriptor.
            camera: unsafe { sys::mln_camera_options_default() },
        };
        self.with_result_operation(|operation| {
            // SAFETY: operation is leased and raw is size-tagged writable storage.
            maplibre_core::check(unsafe {
                sys::mln_map_camera_query_take_result(operation, &mut raw)
            })
        })?;
        Ok(crate::CameraSnapshot {
            generation: raw.generation,
            camera: maplibre_core::camera::camera_options_from_native(raw.camera),
        })
    }
}

impl RuntimeHandle {
    /// Creates a runtime on the current thread using explicit options.
    pub fn with_options(options: &RuntimeOptions) -> Result<Self> {
        maplibre_core::validate_abi_version()?;
        let native_options = options.to_native()?;
        let raw_options = native_options.to_raw();
        Self::create_with_native_options_after_abi_validation(&raw_options)
    }

    #[cfg(test)]
    fn create_with_native_options_after_abi_version_check_for_testing(
        options: *const sys::mln_runtime_options,
        actual_abi_version: u32,
    ) -> Result<Self> {
        maplibre_core::validate_abi_version_value(actual_abi_version)?;
        Self::create_with_native_options_after_abi_validation(options)
    }

    fn create_with_native_options_after_abi_validation(
        options: *const sys::mln_runtime_options,
    ) -> Result<Self> {
        let mut source = sys::mln_notification_source(0);
        // SAFETY: source points to a null writable handle.
        maplibre_core::check(unsafe { sys::mln_notification_source_create(&mut source) })?;
        // SAFETY: A nonnull options pointer comes from the binding's
        // materialized native options and remains readable for this call.
        let mut raw_options = if options.is_null() {
            unsafe { sys::mln_runtime_options_default() }
        } else {
            unsafe { *options }
        };
        raw_options.notification_source = source;
        let mut operation = sys::mln_operation(0);
        // SAFETY: raw_options remains readable and operation is null writable storage.
        if let Err(error) = maplibre_core::check(unsafe {
            sys::mln_runtime_create_start(&raw_options, &mut operation)
        }) {
            // SAFETY: failed creation did not retain the source.
            unsafe { sys::mln_notification_source_close(source) };
            return Err(error);
        }
        let result = (|| {
            wait_raw_operation_completed(operation)?;
            let mut out = maplibre_core::ptr::OutHandle::<sys::mln_runtime>::new();
            // SAFETY: operation completed successfully and out is null writable storage.
            maplibre_core::check(unsafe {
                sys::mln_runtime_create_take_result(operation, out.as_mut_ptr())
            })?;
            let ptr = out_handle(out, "mln_runtime")?;
            Ok(Self {
                inner: Arc::new(RuntimeState::new(ptr, source)?),
            })
        })();
        // SAFETY: the creation observer is owned by this call.
        unsafe { sys::mln_operation_release(operation) };
        if result.is_err() {
            // SAFETY: no runtime wrapper retained source on failure.
            unsafe { sys::mln_notification_source_close(source) };
        }
        result
    }

    /// Installs or replaces the runtime-scoped network resource provider.
    ///
    /// Native may invoke the closure from worker or network threads, so keep it
    /// quick and call no map or runtime APIs from it. Return `PassThrough` to
    /// let native networking handle the request, or `Handle` to complete or
    /// release the provided `ResourceRequestHandle` inline or later.
    ///
    /// A successful replacement retires the previous provider before returning
    /// and then releases its Rust state. Handles the previous provider already
    /// took stay valid; complete and release each one as usual.
    pub fn set_resource_provider<F>(&self, callback: F) -> Result<u64>
    where
        F: Fn(crate::ResourceRequest, crate::ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        self.inner.set_resource_provider(callback)
    }

    /// Clears the runtime-scoped network resource provider, sending later
    /// requests to MapLibre's online file source. The clear waits for in-flight
    /// provider callbacks before returning and then releases the Rust callback
    /// state. Handles the provider already took stay valid.
    pub fn clear_resource_provider(&self) -> Result<u64> {
        self.inner.clear_resource_provider()
    }

    /// Installs or replaces the runtime-scoped network URL transform.
    ///
    /// Native may invoke the closure from worker or network threads, so keep it
    /// quick and call no MapLibre Native APIs from it. `Some(url)` replaces the
    /// request URL; `None`, an empty string, or a panic keeps the original.
    pub fn set_resource_transform<F>(&self, callback: F) -> Result<u64>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        self.inner.set_resource_transform(callback)
    }

    /// Clears the runtime-scoped network URL transform. The clear waits for
    /// in-flight transform callbacks before returning and then releases the
    /// Rust callback state.
    pub fn clear_resource_transform(&self) -> Result<u64> {
        self.inner.clear_resource_transform()
    }

    /// Installs or replaces the runtime-scoped outgoing HTTP header transform.
    ///
    /// Native invokes the closure synchronously on worker or network threads
    /// after URL transformation. Returned headers are copied before the closure
    /// returns. Panics, duplicate names, and invalid headers leave the request
    /// unchanged.
    pub fn set_http_header_transform<F>(&self, callback: F) -> Result<u64>
    where
        F: Fn(crate::HttpHeaderTransformRequest) -> Vec<crate::HttpHeader> + Send + Sync + 'static,
    {
        self.inner.set_http_header_transform(callback)
    }

    /// Clears the runtime-scoped outgoing HTTP header transform.
    pub fn clear_http_header_transform(&self) -> Result<u64> {
        self.inner.clear_http_header_transform()
    }

    /// Installs the any-thread scheduling callback for this runtime's receiver.
    ///
    /// The callback must only schedule receiver work; call [`Self::drain_ready`]
    /// later on that receiver to identify ready endpoints.
    pub fn set_notification_callback<F>(&self, callback: F) -> Result<()>
    where
        F: Fn() + Send + Sync + 'static,
    {
        let replacement = Box::new(NotificationCallbackState {
            callback: Box::new(callback),
        });
        let user_data = (&*replacement as *const NotificationCallbackState)
            .cast_mut()
            .cast();
        let mut callback_slot = self
            .inner
            .notification_callback
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let source = *self
            .inner
            .notification_source
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        // SAFETY: replacement remains boxed at a stable address after success.
        maplibre_core::check(unsafe {
            sys::mln_notification_source_set_callback(
                source,
                Some(notification_callback),
                user_data,
            )
        })?;
        callback_slot.replace(replacement);
        Ok(())
    }

    /// Clears the receiver scheduling callback after native exits all entries.
    pub fn clear_notification_callback(&self) -> Result<()> {
        let mut callback_slot = self
            .inner
            .notification_callback
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let source = *self
            .inner
            .notification_source
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        // SAFETY: source is owned by this live runtime.
        maplibre_core::check(unsafe { sys::mln_notification_source_clear_callback(source) })?;
        callback_slot.take();
        Ok(())
    }

    /// Drains and copies the receiver endpoints currently ready for service.
    pub fn drain_ready(&self) -> Result<Vec<ReadyEndpoint>> {
        let source = *self
            .inner
            .notification_source
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let mut batch = sys::mln_ready_batch(0);
        // SAFETY: source is live and batch is null writable storage.
        maplibre_core::check(unsafe {
            sys::mln_notification_source_drain_ready(source, &mut batch)
        })?;
        let mut view = sys::mln_ready_batch_view {
            size: std::mem::size_of::<sys::mln_ready_batch_view>() as u32,
            endpoint_size: 0,
            endpoints: std::ptr::null(),
            endpoint_count: 0,
        };
        // SAFETY: batch is owned and view is writable.
        if let Err(error) =
            maplibre_core::check(unsafe { sys::mln_ready_batch_get(batch, &mut view) })
        {
            // SAFETY: this call owns batch.
            unsafe { sys::mln_ready_batch_release(batch) };
            return Err(error);
        }
        if view.endpoint_size < std::mem::size_of::<sys::mln_ready_endpoint>() as u32
            || (view.endpoint_count != 0 && view.endpoints.is_null())
        {
            // SAFETY: this call owns batch.
            unsafe { sys::mln_ready_batch_release(batch) };
            return Err(Error::new(
                ErrorKind::NativeError,
                None,
                "native returned an invalid ready-batch view",
            ));
        }
        let mut endpoints = Vec::with_capacity(view.endpoint_count);
        for index in 0..view.endpoint_count {
            // SAFETY: the validated view exposes endpoint_count records at
            // endpoint_size byte strides until batch is released below.
            let raw = unsafe {
                std::ptr::read_unaligned(
                    view.endpoints
                        .cast::<u8>()
                        .add(index * view.endpoint_size as usize)
                        .cast::<sys::mln_ready_endpoint>(),
                )
            };
            endpoints.push(ReadyEndpoint {
                kind: ready_endpoint_kind(raw.kind),
                id: raw.id,
            });
        }
        // SAFETY: copied endpoints no longer borrow batch.
        unsafe { sys::mln_ready_batch_release(batch) };
        Ok(endpoints)
    }

    pub(crate) fn start_operation<T>(
        &self,
        operation: sys::mln_operation,
        operation_kind: OperationKind,
    ) -> Result<OperationHandle<T>> {
        OperationHandle::new(
            operation,
            operation_kind,
            Arc::clone(&self.inner.operations),
        )
    }

    /// Starts an ambient cache maintenance operation for this runtime.
    pub fn start_ambient_cache_operation(
        &self,
        ambient_operation: AmbientCacheOperation,
    ) -> Result<OperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_runtime_run_ambient_cache_operation_start(
                runtime,
                ambient_operation.to_native(),
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::AmbientCache)
    }

    /// Starts a change to this runtime's maximum ambient cache size.
    ///
    /// MapLibre evicts ambient resources to fit the new budget, so lowering it
    /// discards cached resources. Offline regions are unaffected.
    pub fn start_set_maximum_ambient_cache_size(&self, size: u64) -> Result<OperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_maximum_ambient_cache_size_start(runtime, size, &mut operation)
        })?;
        self.start_operation(operation, OperationKind::SetMaximumAmbientCacheSize)
    }

    /// Starts creating an offline region.
    pub fn start_create_offline_region(
        &self,
        definition: &OfflineRegionDefinition,
        metadata: &[u8],
    ) -> Result<OperationHandle<OfflineRegionInfo>> {
        let runtime = self.inner.native()?;
        let definition = definition.to_native()?;
        let raw_definition = definition.to_raw();
        let mut operation = sys::mln_operation(0);
        // SAFETY: runtime is live. raw_definition points into definition-owned
        // string and geometry storage, metadata storage is valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_create_start(
                runtime,
                &raw_definition,
                maplibre_core::runtime::metadata_ptr(metadata),
                metadata.len(),
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::RegionCreate)
    }

    /// Starts getting an offline region snapshot by ID.
    pub fn start_offline_region(
        &self,
        region_id: i64,
    ) -> Result<OperationHandle<Option<OfflineRegionInfo>>> {
        let runtime = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: runtime is live and operation points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_get_start(runtime, region_id, &mut operation)
        })?;
        self.start_operation(operation, OperationKind::RegionGet)
    }

    /// Starts listing offline regions in this runtime's database.
    pub fn start_offline_regions(&self) -> Result<OperationHandle<Vec<OfflineRegionInfo>>> {
        let runtime = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: runtime is live and operation points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_regions_list_start(runtime, &mut operation)
        })?;
        self.start_operation(operation, OperationKind::RegionsList)
    }

    /// Starts merging offline regions from another database path.
    pub fn start_merge_offline_regions_database(
        &self,
        path: &str,
    ) -> Result<OperationHandle<Vec<OfflineRegionInfo>>> {
        let runtime = self.inner.native()?;
        let path = maplibre_core::string::c_string(path)?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: runtime is live, path is NUL-terminated and valid for this
        // call, and operation points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_regions_merge_database_start(
                runtime,
                path.as_ptr(),
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::RegionsMergeDatabase)
    }

    /// Starts updating opaque metadata for an offline region.
    pub fn start_update_offline_region_metadata(
        &self,
        region_id: i64,
        metadata: &[u8],
    ) -> Result<OperationHandle<OfflineRegionInfo>> {
        let runtime = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: runtime is live, metadata storage is valid for this call, and
        // operation points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_update_metadata_start(
                runtime,
                region_id,
                maplibre_core::runtime::metadata_ptr(metadata),
                metadata.len(),
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::RegionUpdateMetadata)
    }

    /// Starts getting the current completed/download status for an offline region.
    pub fn start_offline_region_status(
        &self,
        region_id: i64,
    ) -> Result<OperationHandle<OfflineRegionStatus>> {
        let runtime = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: runtime is live and operation points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_get_status_start(runtime, region_id, &mut operation)
        })?;
        self.start_operation(operation, OperationKind::RegionGetStatus)
    }

    /// Starts enabling or disabling runtime events for an offline region.
    pub fn start_set_offline_region_observed(
        &self,
        region_id: i64,
        observed: bool,
    ) -> Result<OperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_set_observed_start(
                runtime,
                region_id,
                observed,
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::RegionSetObserved)
    }

    /// Starts setting an offline region's native download state.
    pub fn start_set_offline_region_download_state(
        &self,
        region_id: i64,
        state: OfflineRegionDownloadState,
    ) -> Result<OperationHandle<()>> {
        let runtime = self.inner.native()?;
        let state = state.raw_for_set()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_set_download_state_start(
                runtime,
                region_id,
                state,
                &mut operation,
            )
        })?;
        self.start_operation(operation, OperationKind::RegionSetDownloadState)
    }

    /// Starts invalidating cached resources for an offline region.
    pub fn start_invalidate_offline_region(&self, region_id: i64) -> Result<OperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_invalidate_start(runtime, region_id, &mut operation)
        })?;
        self.start_operation(operation, OperationKind::RegionInvalidate)
    }

    /// Starts deleting an offline region.
    pub fn start_delete_offline_region(&self, region_id: i64) -> Result<OperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_delete_start(runtime, region_id, &mut operation)
        })?;
        self.start_operation(operation, OperationKind::RegionDelete)
    }

    /// Starts an ordered barrier that completes after all previously accepted
    /// runtime work reaches a terminal disposition.
    pub fn start_barrier(&self) -> Result<OperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation = sys::mln_operation(0);
        // SAFETY: runtime is live and operation is null writable storage.
        maplibre_core::check(unsafe { sys::mln_runtime_barrier_start(runtime, &mut operation) })?;
        self.start_operation(operation, OperationKind::Barrier)
    }

    /// Drains an owned batch of queued runtime events.
    ///
    /// `max_events` bounds the drain. Zero drains every queued event, and a
    /// positive value drains at most that many and reports the rest through
    /// [`RuntimeEventBatch::remaining`]. The returned batch remains stable
    /// across later drains and releases its native storage when dropped.
    pub fn drain_events(&self, max_events: usize) -> Result<RuntimeEventBatch> {
        let runtime = self.inner.native()?;
        let mut batch = sys::mln_event_batch(0);
        // SAFETY: runtime is live and batch points to a null writable handle.
        maplibre_core::check(unsafe {
            sys::mln_runtime_drain_events(runtime, max_events, &mut batch)
        })?;
        // SAFETY: A successful drain returns one owned native batch.
        unsafe { RuntimeEventBatch::new(batch) }
    }

    /// Selects which runtime-originated event types this runtime queues.
    ///
    /// A runtime reads the bits in
    /// [`RuntimeEventMask::ALL_RUNTIME_EVENTS`](crate::RuntimeEventMask::ALL_RUNTIME_EVENTS),
    /// so [`RuntimeEventMask::ALL`](crate::RuntimeEventMask::ALL) selects every
    /// runtime-originated type. Narrowing gates later events and keeps queued
    /// ones. A bit outside `ALL` is an invalid-argument error.
    pub fn set_event_mask(&self, mask: RuntimeEventMask) -> Result<()> {
        let runtime = self.inner.native()?;
        // SAFETY: runtime is live. The C API validates unknown mask bits.
        maplibre_core::check(unsafe { sys::mln_runtime_set_event_mask(runtime, mask.bits()) })
    }

    /// Reports which runtime-originated event types this runtime queues,
    /// starting from the mask its creation options selected.
    pub fn event_mask(&self) -> Result<RuntimeEventMask> {
        let runtime = self.inner.native()?;
        let mut raw = 0;
        // SAFETY: runtime is live and out_mask points to writable u64 storage.
        maplibre_core::check(unsafe { sys::mln_runtime_get_event_mask(runtime, &mut raw) })?;
        Ok(RuntimeEventMask::from_bits_retain(raw))
    }

    /// Explicitly closes the runtime and waits for its worker to stop before
    /// releasing the shared notification source.
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if self.inner.is_closed() {
            return Ok(());
        }
        if Arc::strong_count(&self.inner) > 1 {
            return Err(HandleOperationError::new(
                Error::new(
                    ErrorKind::InvalidState,
                    None,
                    "RuntimeHandle cannot close while child handles are live",
                ),
                self,
            ));
        }
        self.inner
            .close()
            .map_err(|error| HandleOperationError::new(error, self))
    }
}

#[cfg(test)]
mod tests {
    // The fixture HTTP servers below are native-only; a browser build fetches
    // from the servers the test runner hosts instead.
    #[cfg(not(target_os = "emscripten"))]
    use std::io::{Read, Write};
    #[cfg(not(target_os = "emscripten"))]
    use std::net::TcpListener;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::{Arc, Mutex};
    use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

    use super::*;
    use crate::{
        CommandDisposition, ErrorKind, ResourceErrorReason, ResourceKind, ResourceProviderDecision,
        ResourceResponse, RuntimeEvent, RuntimeEventPayload, RuntimeEventSource, RuntimeEventType,
    };

    const PROVIDER_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;

    #[cfg(not(target_os = "emscripten"))]
    fn spawn_style_server(
        request_count: usize,
    ) -> (
        String,
        std::sync::mpsc::Receiver<String>,
        std::thread::JoinHandle<()>,
    ) {
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let base_url = format!("http://{}", listener.local_addr().unwrap());
        let (sender, receiver) = std::sync::mpsc::channel();
        let handle = std::thread::spawn(move || {
            for _ in 0..request_count {
                let (mut stream, _) = listener.accept().unwrap();
                stream
                    .set_read_timeout(Some(Duration::from_secs(5)))
                    .unwrap();
                let mut request = [0; 4096];
                let bytes = stream.read(&mut request).unwrap();
                let request = String::from_utf8_lossy(&request[..bytes]);
                let path = request
                    .lines()
                    .next()
                    .and_then(|line| line.split_whitespace().nth(1))
                    .unwrap_or("")
                    .to_owned();
                sender.send(path).unwrap();

                let body = PROVIDER_STYLE_JSON.as_bytes();
                write!(
                    stream,
                    "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
                    body.len()
                )
                .unwrap();
                stream.write_all(body).unwrap();
            }
        });
        (base_url, receiver, handle)
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    fn spawn_recording_style_server(
        request_count: usize,
    ) -> (
        String,
        std::sync::mpsc::Receiver<String>,
        std::thread::JoinHandle<()>,
    ) {
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let base_url = format!("http://{}", listener.local_addr().unwrap());
        let (sender, receiver) = std::sync::mpsc::channel();
        let handle = std::thread::spawn(move || {
            for _ in 0..request_count {
                let (mut stream, _) = listener.accept().unwrap();
                stream
                    .set_read_timeout(Some(Duration::from_secs(5)))
                    .unwrap();
                let mut bytes = [0; 4096];
                let count = stream.read(&mut bytes).unwrap();
                sender
                    .send(String::from_utf8_lossy(&bytes[..count]).into_owned())
                    .unwrap();
                let body = PROVIDER_STYLE_JSON.as_bytes();
                write!(
                    stream,
                    "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
                    body.len()
                )
                .unwrap();
                stream.write_all(body).unwrap();
            }
        });
        (base_url, receiver, handle)
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    fn spawn_redirect_style_servers() -> (
        String,
        std::sync::mpsc::Receiver<(String, bool)>,
        Vec<std::thread::JoinHandle<()>>,
    ) {
        let origin = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let destination = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let origin_url = format!("http://{}", origin.local_addr().unwrap());
        let destination_url = format!("http://{}", destination.local_addr().unwrap());
        let (sender, receiver) = std::sync::mpsc::channel();

        let origin_sender = sender.clone();
        let origin_destination_url = destination_url.clone();
        let origin_server = std::thread::spawn(move || {
            for _ in 0..3 {
                let (mut stream, _) = origin.accept().unwrap();
                stream
                    .set_read_timeout(Some(Duration::from_secs(5)))
                    .unwrap();
                let mut bytes = [0; 4096];
                let count = stream.read(&mut bytes).unwrap();
                let request = String::from_utf8_lossy(&bytes[..count]);
                let path = request
                    .lines()
                    .next()
                    .and_then(|line| line.split_whitespace().nth(1))
                    .unwrap_or("")
                    .to_owned();
                let has_header = request
                    .lines()
                    .any(|line| line.eq_ignore_ascii_case("X-Map-Token: secret"));
                origin_sender.send((path.clone(), has_header)).unwrap();

                if path == "/same-start.json" {
                    write!(
                        stream,
                        "HTTP/1.1 302 Found\r\nLocation: /same-final.json\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
                    )
                    .unwrap();
                } else if path == "/cross-start.json" {
                    write!(
                        stream,
                        "HTTP/1.1 302 Found\r\nLocation: {origin_destination_url}/cross-final.json\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
                    )
                    .unwrap();
                } else {
                    let body = PROVIDER_STYLE_JSON.as_bytes();
                    write!(
                        stream,
                        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
                        body.len()
                    )
                    .unwrap();
                    stream.write_all(body).unwrap();
                }
            }
        });

        let destination_server = std::thread::spawn(move || {
            let (mut stream, _) = destination.accept().unwrap();
            stream
                .set_read_timeout(Some(Duration::from_secs(5)))
                .unwrap();
            let mut bytes = [0; 4096];
            let count = stream.read(&mut bytes).unwrap();
            let request = String::from_utf8_lossy(&bytes[..count]);
            let path = request
                .lines()
                .next()
                .and_then(|line| line.split_whitespace().nth(1))
                .unwrap_or("")
                .to_owned();
            let has_header = request
                .lines()
                .any(|line| line.eq_ignore_ascii_case("X-Map-Token: secret"));
            sender.send((path, has_header)).unwrap();
            let body = PROVIDER_STYLE_JSON.as_bytes();
            write!(
                stream,
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
                body.len()
            )
            .unwrap();
            stream.write_all(body).unwrap();
        });

        (
            origin_url,
            receiver,
            vec![origin_server, destination_server],
        )
    }

    fn wait_for_operation<T>(
        _runtime: &mut RuntimeHandle,
        operation: &OperationHandle<T>,
    ) -> Result<()> {
        let deadline = Instant::now() + Duration::from_secs(30);
        loop {
            if Instant::now() >= deadline {
                return Err(Error::new(
                    ErrorKind::InvalidState,
                    None,
                    format!(
                        "timed out waiting for operation handle {}",
                        operation.operation.0
                    ),
                ));
            }
            std::thread::sleep(std::time::Duration::from_millis(1));
            if operation.is_completed()? {
                let status = operation.terminal_status()?;
                if status != sys::MLN_STATUS_OK {
                    return Err(Error::from_status_and_diagnostic(
                        status,
                        operation.diagnostic()?,
                    ));
                }
                return Ok(());
            }
            std::thread::sleep(Duration::from_millis(1));
        }
    }

    #[test]
    // Spec coverage: BND-084.
    fn runtime_ambient_cache_operations_use_real_c_abi() {
        let base = TempDir::new("maplibre-rust-ambient-cache");
        let cache = base.path().join("ambient.db");

        let mut options = RuntimeOptions::default();
        options.cache_path = Some(cache.to_string_lossy().into_owned());
        let mut runtime = RuntimeHandle::with_options(&options).unwrap();

        for operation in [
            AmbientCacheOperation::PackDatabase,
            AmbientCacheOperation::Invalidate,
            AmbientCacheOperation::Clear,
            AmbientCacheOperation::ResetDatabase,
        ] {
            let operation = runtime.start_ambient_cache_operation(operation).unwrap();
            wait_for_operation(&mut runtime, &operation).unwrap();
            operation.discard().unwrap();
            operation.release();
        }

        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-084.
    fn runtime_set_maximum_ambient_cache_size_reports_completion() {
        let base = TempDir::new("maplibre-rust-cache-size");
        let cache = base.path().join("ambient-size.db");

        let mut options = RuntimeOptions::default();
        options.cache_path = Some(cache.to_string_lossy().into_owned());
        let mut runtime = RuntimeHandle::with_options(&options).unwrap();

        // Raising then lowering the budget exercises the same operation API.
        for size in [8 * 1024 * 1024, 0] {
            let operation = runtime.start_set_maximum_ambient_cache_size(size).unwrap();
            wait_for_operation(&mut runtime, &operation).unwrap();
            operation.discard().unwrap();
            operation.release();
        }

        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-084.
    fn runtime_close_rejects_a_live_operation_before_destroying_native() {
        let mut options = RuntimeOptions::default();
        options.cache_path = Some(":memory:".into());
        let mut runtime = RuntimeHandle::with_options(&options).unwrap();
        let operation = runtime
            .start_ambient_cache_operation(AmbientCacheOperation::Clear)
            .unwrap();

        let error = runtime.close().unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        runtime = error.into_handle();
        wait_for_operation(&mut runtime, &operation).unwrap();
        operation.discard().unwrap();
        assert_eq!(operation.terminal_status().unwrap(), sys::MLN_STATUS_OK);
        operation.release();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-084 and BND-085.
    fn offline_region_apis_use_real_c_abi() {
        let mut options = RuntimeOptions::default();
        options.cache_path = Some(":memory:".into());
        let mut runtime = RuntimeHandle::with_options(&options).unwrap();
        let definition = test_offline_region_definition("custom://offline-style.json");

        let create = runtime
            .start_create_offline_region(&definition, b"abc")
            .unwrap();
        wait_for_operation(&mut runtime, &create).unwrap();
        let created = create.take().unwrap();
        create.release();
        assert_eq!(created.definition, definition);
        assert_eq!(created.metadata, b"abc");

        let geometry_definition = OfflineRegionDefinition::GeometryRegion {
            style_url: "custom://offline-geometry-style.json".into(),
            geometry: br#"{"type":"Point","coordinates":[-122.5,37.5]}"#.to_vec(),
            min_zoom: 0.0,
            max_zoom: 1.0,
            pixel_ratio: 1.0,
            include_ideographs: false,
        };
        let create_geometry = runtime
            .start_create_offline_region(&geometry_definition, b"geo")
            .unwrap();
        wait_for_operation(&mut runtime, &create_geometry).unwrap();
        let geometry_region = create_geometry.take().unwrap();
        create_geometry.release();
        assert_eq!(geometry_region.definition, geometry_definition);
        assert_eq!(geometry_region.metadata, b"geo");

        let get = runtime.start_offline_region(created.id).unwrap();
        wait_for_operation(&mut runtime, &get).unwrap();
        let fetched = get.take().unwrap().unwrap();
        get.release();
        assert_eq!(fetched, created);

        let list = runtime.start_offline_regions().unwrap();
        wait_for_operation(&mut runtime, &list).unwrap();
        let listed = list.take().unwrap();
        list.release();
        assert!(listed.iter().any(|region| region.id == created.id));

        let update = runtime
            .start_update_offline_region_metadata(created.id, b"")
            .unwrap();
        wait_for_operation(&mut runtime, &update).unwrap();
        let updated = update.take().unwrap();
        update.release();
        assert_eq!(updated.id, created.id);
        assert!(updated.metadata.is_empty());

        let status_operation = runtime.start_offline_region_status(created.id).unwrap();
        wait_for_operation(&mut runtime, &status_operation).unwrap();
        let status = status_operation.take().unwrap();
        status_operation.release();
        assert!(matches!(
            status.download_state,
            OfflineRegionDownloadState::Inactive | OfflineRegionDownloadState::Active
        ));

        let set_inactive = runtime
            .start_set_offline_region_download_state(
                created.id,
                OfflineRegionDownloadState::Inactive,
            )
            .unwrap();
        wait_for_operation(&mut runtime, &set_inactive).unwrap();
        set_inactive.discard().unwrap();
        set_inactive.release();
        let error = runtime
            .start_set_offline_region_download_state(
                created.id,
                OfflineRegionDownloadState::Unknown(99),
            )
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);

        let observe = runtime
            .start_set_offline_region_observed(created.id, true)
            .unwrap();
        wait_for_operation(&mut runtime, &observe).unwrap();
        observe.discard().unwrap();
        observe.release();
        let unobserve = runtime
            .start_set_offline_region_observed(created.id, false)
            .unwrap();
        wait_for_operation(&mut runtime, &unobserve).unwrap();
        unobserve.discard().unwrap();
        unobserve.release();
        let invalidate = runtime.start_invalidate_offline_region(created.id).unwrap();
        wait_for_operation(&mut runtime, &invalidate).unwrap();
        invalidate.discard().unwrap();
        invalidate.release();
        let delete = runtime.start_delete_offline_region(created.id).unwrap();
        wait_for_operation(&mut runtime, &delete).unwrap();
        delete.discard().unwrap();
        delete.release();
        let delete_geometry = runtime
            .start_delete_offline_region(geometry_region.id)
            .unwrap();
        wait_for_operation(&mut runtime, &delete_geometry).unwrap();
        delete_geometry.discard().unwrap();
        delete_geometry.release();

        let missing_created = runtime.start_offline_region(created.id).unwrap();
        wait_for_operation(&mut runtime, &missing_created).unwrap();
        assert!(missing_created.take().unwrap().is_none());
        missing_created.release();
        let missing_geometry = runtime.start_offline_region(geometry_region.id).unwrap();
        wait_for_operation(&mut runtime, &missing_geometry).unwrap();
        assert!(missing_geometry.take().unwrap().is_none());
        missing_geometry.release();

        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-084.
    fn offline_region_merge_database_uses_real_c_abi() {
        let base = TempDir::new("maplibre-rust-offline-merge");
        let main_cache = base.path().join("main.db");
        let side_cache = base.path().join("side.db");

        let definition = test_offline_region_definition("custom://merge-style.json");
        {
            let mut side_options = RuntimeOptions::default();
            side_options.cache_path = Some(side_cache.to_string_lossy().into_owned());
            let mut side_runtime = RuntimeHandle::with_options(&side_options).unwrap();
            let create = side_runtime
                .start_create_offline_region(&definition, b"merge")
                .unwrap();
            wait_for_operation(&mut side_runtime, &create).unwrap();
            create.take().unwrap();
            create.release();
            side_runtime.close().unwrap();
        }

        let mut main_options = RuntimeOptions::default();
        main_options.cache_path = Some(main_cache.to_string_lossy().into_owned());
        let mut main_runtime = RuntimeHandle::with_options(&main_options).unwrap();
        let merge = main_runtime
            .start_merge_offline_regions_database(&side_cache.to_string_lossy())
            .unwrap();
        wait_for_operation(&mut main_runtime, &merge).unwrap();
        let merged = merge.take().unwrap();
        merge.release();
        assert_eq!(merged.len(), 1);
        assert_eq!(merged[0].definition, definition);
        assert_eq!(merged[0].metadata, b"merge");
        main_runtime.close().unwrap();
    }

    fn test_offline_region_definition(style_url: &str) -> OfflineRegionDefinition {
        OfflineRegionDefinition::TilePyramid {
            style_url: style_url.into(),
            bounds: LatLngBounds::new(
                crate::LatLng::new(37.0, -123.0),
                crate::LatLng::new(38.0, -122.0),
            ),
            min_zoom: 0.0,
            max_zoom: 1.0,
            pixel_ratio: 1.0,
            include_ideographs: false,
        }
    }

    struct TempDir {
        path: std::path::PathBuf,
    }

    impl TempDir {
        fn new(prefix: &str) -> Self {
            let nanos = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_nanos();
            let path =
                std::env::temp_dir().join(format!("{prefix}-{}-{nanos}", std::process::id()));
            std::fs::create_dir_all(&path).unwrap();
            Self { path }
        }

        fn path(&self) -> &std::path::Path {
            &self.path
        }
    }

    impl Drop for TempDir {
        fn drop(&mut self) {
            let _ = std::fs::remove_dir_all(&self.path);
        }
    }

    #[test]
    // Spec coverage: BND-040.
    fn runtime_create_with_explicit_options_uses_real_c_abi() {
        let mut options = RuntimeOptions::default();
        options.asset_path = Some(String::new());
        options.cache_path = Some(String::new());
        let runtime = RuntimeHandle::with_options(&options).unwrap();

        std::thread::sleep(std::time::Duration::from_millis(1));
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-001.
    fn runtime_creation_rejects_abi_mismatch_before_storing_handle() {
        let error = RuntimeHandle::create_with_native_options_after_abi_version_check_for_testing(
            std::ptr::null(),
            maplibre_core::EXPECTED_C_ABI_VERSION + 1,
        )
        .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::AbiVersionMismatch);
        assert_eq!(error.raw_status(), None);
        assert!(
            error
                .diagnostic()
                .contains("unsupported MapLibre Native C ABI version")
        );
    }

    fn drain_holds_event_type(runtime: &RuntimeHandle, event_type: RuntimeEventType) -> bool {
        runtime
            .drain_events(0)
            .unwrap()
            .iter()
            .any(|event| event.event_type() == event_type)
    }

    fn wait_for_runtime_event(runtime: &mut RuntimeHandle, event_type: RuntimeEventType) -> bool {
        for _ in 0..100 {
            std::thread::sleep(std::time::Duration::from_millis(1));
            if drain_holds_event_type(runtime, event_type) {
                return true;
            }
            std::thread::sleep(Duration::from_millis(10));
        }
        false
    }

    fn wait_for_map_loading_failure(runtime: &mut RuntimeHandle) -> RuntimeEvent {
        for _ in 0..100 {
            std::thread::sleep(std::time::Duration::from_millis(1));
            let mut failure = None;
            for event in runtime.drain_events(0).unwrap().iter() {
                if event.event_type() == RuntimeEventType::MapLoadingFailed {
                    failure = Some(event.to_owned().unwrap());
                    break;
                }
            }
            if let Some(failure) = failure {
                return failure;
            }
            std::thread::sleep(Duration::from_millis(10));
        }
        panic!("expected a map loading-failure event");
    }

    #[test]
    // Spec coverage: BND-080.
    fn runtime_create_run_drain_and_close() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();

        std::thread::sleep(std::time::Duration::from_millis(1));
        // A fresh runtime with no map queues nothing.
        let batch = runtime.drain_events(0).unwrap();
        assert!(batch.is_empty());
        assert_eq!(batch.len(), 0);
        assert_eq!(batch.remaining(), 0);
        assert_eq!(batch.iter().count(), 0);
        // A second drain of an empty queue reports the same empty batch.
        assert!(runtime.drain_events(0).unwrap().is_empty());
        runtime.close().unwrap();
    }

    fn wait_for_event(runtime: &RuntimeHandle, event_type: RuntimeEventType) -> bool {
        for _ in 0..500 {
            if drain_holds_event_type(runtime, event_type) {
                return true;
            }
            std::thread::sleep(Duration::from_millis(1));
        }
        false
    }

    #[test]
    fn native_worker_makes_progress_without_host_driving() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        map.set_style_json(PROVIDER_STYLE_JSON.as_bytes()).unwrap();

        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn runtime_accepts_concurrent_barrier_submissions() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        std::thread::scope(|scope| {
            let first = scope.spawn(|| runtime.start_barrier().unwrap());
            let second = scope.spawn(|| runtime.start_barrier().unwrap());
            for operation in [first.join().unwrap(), second.join().unwrap()] {
                assert!(operation.wait(Duration::from_secs(5)).unwrap());
                assert_eq!(operation.terminal_status().unwrap(), sys::MLN_STATUS_OK);
                operation.discard().unwrap();
            }
        });
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-090.
    fn a_bounded_drain_reports_the_events_it_left_queued() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        map.set_style_json(PROVIDER_STYLE_JSON.as_bytes()).unwrap();
        std::thread::sleep(std::time::Duration::from_millis(1));

        let bounded = runtime.drain_events(1).unwrap();
        assert_eq!(bounded.len(), 1);
        assert!(
            bounded.remaining() > 0,
            "a style load should queue more than one event"
        );
        let remaining = bounded.remaining();
        let rest = runtime.drain_events(0).unwrap();
        assert!(
            rest.len() >= remaining,
            "the unbounded drain should include every event previously reported queued"
        );
        assert_eq!(rest.remaining(), 0);
        let first = rest.iter().next().unwrap();
        assert_eq!(first.source(), RuntimeEventSource::Map(map.id()));

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-060 and BND-091.
    fn a_creation_mask_is_applied_when_the_runtime_is_created() {
        let mut options = crate::RuntimeOptions::default();
        options.event_mask = RuntimeEventMask::MAP_STYLE_LOADED;
        let runtime = RuntimeHandle::with_options(&options).unwrap();

        assert_eq!(
            runtime.event_mask().unwrap(),
            RuntimeEventMask::MAP_STYLE_LOADED
        );
        runtime.close().unwrap();

        // A creation mask carrying an undefined bit fails before the runtime
        // exists.
        options.event_mask = RuntimeEventMask::from_bits_retain(1 << 63);
        let error = RuntimeHandle::with_options(&options).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    }

    #[test]
    // Spec coverage: BND-091.
    fn a_runtime_mask_round_trips_and_rejects_undefined_bits() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();

        // The default options mask selects every type.
        assert_eq!(runtime.event_mask().unwrap(), RuntimeEventMask::ALL);

        runtime.set_event_mask(RuntimeEventMask::ALL).unwrap();
        assert_eq!(runtime.event_mask().unwrap(), RuntimeEventMask::ALL);

        // Read, clear one bit, write back: every other bit survives.
        let mut mask = runtime.event_mask().unwrap();
        mask.remove(RuntimeEventMask::OFFLINE_REGION_STATUS_CHANGED);
        runtime.set_event_mask(mask).unwrap();
        let read_back = runtime.event_mask().unwrap();
        assert!(!read_back.contains(RuntimeEventMask::OFFLINE_REGION_STATUS_CHANGED));
        assert!(read_back.contains(RuntimeEventMask::MAP_IDLE));
        assert!(read_back.contains(RuntimeEventMask::MAP_STYLE_LOADED));

        let undefined = RuntimeEventMask::from_bits_retain(1 << 63);
        let error = runtime.set_event_mask(undefined).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert_eq!(runtime.event_mask().unwrap(), read_back);

        runtime.close().unwrap();
    }

    #[test]
    fn runtime_state_is_readable_from_another_thread() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        std::thread::scope(|scope| {
            let mask = scope
                .spawn(|| runtime.event_mask().unwrap())
                .join()
                .unwrap();
            assert_eq!(mask, RuntimeEventMask::ALL);
        });
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-123.
    fn resource_provider_installs_replaces_clears_and_releases_state() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);

        runtime
            .set_resource_provider(move |_, _| {
                let _ = &first_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&first), 2);

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        runtime
            .set_resource_provider(move |_, _| {
                let _ = &second_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 2);

        runtime.clear_resource_provider().unwrap();
        assert_eq!(Arc::strong_count(&second), 2);

        let third = Arc::new(());
        let third_callback = Arc::clone(&third);
        runtime
            .set_resource_provider(move |_, _| {
                let _ = &third_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&third), 2);

        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&third), 1);
        assert_eq!(Arc::strong_count(&first), 1);
        assert_eq!(Arc::strong_count(&second), 1);
    }

    #[test]
    // Spec coverage: BND-122.
    fn resource_provider_replacement_rolls_back_when_native_install_fails() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);
        runtime
            .set_resource_provider(move |_, _| {
                let _ = &first_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap();

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        let error = runtime
            .inner
            .set_resource_provider_with_rejected_descriptor_for_testing(move |_, _| {
                let _ = &second_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 1);

        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&first), 1);
    }

    // Requests a style no file source serves, so the loading-failure event that
    // follows proves the request reached the network file source where the
    // runtime-scoped provider sits.
    fn load_probe_style(runtime: &mut RuntimeHandle, map: &MapHandle, style_url: &str) {
        map.set_style_url(style_url).unwrap();
        let event = wait_for_map_loading_failure(runtime);
        assert!(
            event
                .message
                .as_deref()
                .is_some_and(|message| message.contains("\"jar\""))
        );
    }

    #[test]
    // Spec coverage: BND-142 and BND-154.
    fn resource_provider_is_consulted_until_replaced_and_cleared_while_a_map_is_live() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

        let first_calls = Arc::new(AtomicUsize::new(0));
        let first_callback_calls = Arc::clone(&first_calls);
        runtime
            .set_resource_provider(move |_, _| {
                first_callback_calls.fetch_add(1, Ordering::SeqCst);
                ResourceProviderDecision::PassThrough
            })
            .unwrap();
        load_probe_style(&mut runtime, &map, "jar:file:/packaged/first.json");
        assert!(first_calls.load(Ordering::SeqCst) > 0);

        let second_calls = Arc::new(AtomicUsize::new(0));
        let second_callback_calls = Arc::clone(&second_calls);
        runtime
            .set_resource_provider(move |_, _| {
                second_callback_calls.fetch_add(1, Ordering::SeqCst);
                ResourceProviderDecision::PassThrough
            })
            .unwrap();
        let first_calls_after_replace = first_calls.load(Ordering::SeqCst);
        load_probe_style(&mut runtime, &map, "jar:file:/packaged/second.json");
        assert!(second_calls.load(Ordering::SeqCst) > 0);
        assert_eq!(
            first_calls.load(Ordering::SeqCst),
            first_calls_after_replace
        );

        runtime.clear_resource_provider().unwrap();
        let second_calls_after_clear = second_calls.load(Ordering::SeqCst);
        load_probe_style(&mut runtime, &map, "jar:file:/packaged/third.json");
        assert_eq!(
            first_calls.load(Ordering::SeqCst),
            first_calls_after_replace
        );
        assert_eq!(
            second_calls.load(Ordering::SeqCst),
            second_calls_after_clear
        );

        // Clearing an already cleared provider stays a successful no-op.
        runtime.clear_resource_provider().unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-143 and BND-150.
    fn resource_provider_completes_style_request_inline_through_c_abi() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        runtime
            .set_resource_provider(move |request, handle| {
                if request.requested_url != "custom://style.json" {
                    return ResourceProviderDecision::PassThrough;
                }
                callback_calls.fetch_add(1, Ordering::SeqCst);
                assert_eq!(request.kind, ResourceKind::Style);
                handle
                    .complete(ResourceResponse::ok(
                        PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                    ))
                    .unwrap();
                ResourceProviderDecision::PassThrough
            })
            .unwrap();

        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        map.set_style_url("custom://style.json").unwrap();

        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(calls.load(Ordering::SeqCst), 1);
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-155.
    fn resource_provider_sees_scheme_alias_and_its_resolved_url() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let resolved = Arc::new(Mutex::new(None));
        let callback_resolved = Arc::clone(&resolved);
        runtime
            .set_resource_provider(move |request, handle| {
                if request.requested_url != "maplibre://maps/style" {
                    return ResourceProviderDecision::PassThrough;
                }
                *callback_resolved.lock().unwrap() = Some(request.resolved_url.clone());
                handle
                    .complete(ResourceResponse::ok(
                        PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                    ))
                    .unwrap();
                ResourceProviderDecision::Handle
            })
            .unwrap();

        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        map.set_style_url("maplibre://maps/style").unwrap();

        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(
            resolved.lock().unwrap().as_deref(),
            Some("https://demotiles.maplibre.org/style.json")
        );
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-144 and BND-145.
    fn resource_provider_completes_style_request_from_another_thread() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let (sender, receiver) = std::sync::mpsc::channel();
        runtime
            .set_resource_provider(move |request, handle| {
                if request.requested_url == "custom://async-style.json" {
                    sender.send(handle).unwrap();
                    ResourceProviderDecision::Handle
                } else {
                    ResourceProviderDecision::PassThrough
                }
            })
            .unwrap();

        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        map.set_style_url("custom://async-style.json").unwrap();
        let handle = receiver
            .recv_timeout(Duration::from_secs(5))
            .expect("provider should send handled request");
        assert!(!handle.is_cancelled().unwrap());
        std::thread::spawn(move || {
            handle
                .complete(ResourceResponse::ok(
                    PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                ))
                .unwrap();
        })
        .join()
        .unwrap();

        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-149.
    fn resource_provider_error_response_becomes_copied_loading_failure_event() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        runtime
            .set_resource_provider(move |request, handle| {
                if request.requested_url == "custom://broken-style.json" {
                    handle
                        .complete(ResourceResponse::error(
                            ResourceErrorReason::Other,
                            "provider failed",
                        ))
                        .unwrap();
                    ResourceProviderDecision::Handle
                } else {
                    ResourceProviderDecision::PassThrough
                }
            })
            .unwrap();

        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        let map_id = map.id();
        map.set_style_url("custom://broken-style.json").unwrap();

        let event = wait_for_map_loading_failure(&mut runtime);
        let copied_message = event.message.clone();
        // The copy stays intact after the drain that ends the batch's window.
        let _ = runtime.drain_events(0).unwrap();

        assert_eq!(event.source, RuntimeEventSource::Map(map_id));
        assert_eq!(event.event_type, RuntimeEventType::MapLoadingFailed);
        assert_eq!(event.message, copied_message);
        assert!(
            event
                .message
                .as_deref()
                .is_some_and(|message| message.contains("provider failed"))
        );

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-140 and BND-123.
    fn resource_transform_installs_replaces_clears_and_releases_state() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);

        runtime
            .set_resource_transform(move |request| {
                let _ = &first_callback;
                assert!(matches!(
                    request.kind,
                    ResourceKind::Style | ResourceKind::UnknownRaw(_)
                ));
                None
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&first), 2);

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        runtime
            .set_resource_transform(move |_| {
                let _ = &second_callback;
                Some("https://example.test/replacement".to_owned())
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 2);

        runtime.clear_resource_transform().unwrap();
        assert_eq!(Arc::strong_count(&second), 2);
        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&first), 1);
        assert_eq!(Arc::strong_count(&second), 1);
    }

    /// The browser has no in-process TCP server, so the runner serves the two
    /// style documents and answers 404 for everything else. That makes the
    /// server the oracle: a rewrite that did not happen fails the style load
    /// rather than fetching an identical document, and the layer each document
    /// carries names which one was used. See scripts/run-browser-test.mjs.
    #[cfg(target_os = "emscripten")]
    #[test]
    // Spec coverage: BND-140.
    fn resource_transform_rewrites_style_url_and_clear_restores_original_url() {
        let origin = std::env::var("MLN_FFI_TEST_FIXTURE_ORIGIN").expect(
            "MLN_FFI_TEST_FIXTURE_ORIGIN is unset; run the suite through \
             `mise run //bindings/rust:test emscripten-wasm32-webgl`",
        );
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let transform_url = format!("{origin}/__fixture/rewritten-style.json");
        // Matches the URL loaded after the clear as well, so a transform that
        // outlived the clear rewrites that request too and the layer id says so.
        runtime
            .set_resource_transform(move |request| {
                (request.url.ends_with("/original-style.json")
                    || request.url.ends_with("/original-after-clear.json"))
                .then(|| transform_url.clone())
            })
            .unwrap();

        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        map.set_style_url(&format!("{origin}/__fixture/original-style.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        // Contains rather than equals: MapLibre adds its own annotation layer to
        // every style it loads.
        let layer_ids = map.style_layer_ids().unwrap();
        assert!(layer_ids.wait(Duration::from_secs(5)).unwrap());
        assert!(layer_ids.take().unwrap().iter().any(|id| id == "rewritten"));
        layer_ids.release();

        runtime.clear_resource_transform().unwrap();
        map.set_style_url(&format!("{origin}/__fixture/original-after-clear.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        let layer_ids = map.style_layer_ids().unwrap();
        assert!(layer_ids.wait(Duration::from_secs(5)).unwrap());
        assert!(
            layer_ids
                .take()
                .unwrap()
                .iter()
                .any(|id| id == "original-after-clear")
        );
        layer_ids.release();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[cfg(not(target_os = "emscripten"))]
    #[test]
    // Spec coverage: BND-140.
    fn resource_transform_rewrites_style_url_and_clear_restores_original_url() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let (base_url, requests, server) = spawn_style_server(2);
        let transform_base_url = base_url.clone();

        // Matches the URL loaded after the clear as well, so a transform that
        // outlived the clear rewrites that request too and the recorded path
        // says so.
        runtime
            .set_resource_transform(move |request| {
                if request.url.ends_with("/original-style.json")
                    || request.url.ends_with("/original-after-clear.json")
                {
                    Some(format!("{transform_base_url}/rewritten-style.json"))
                } else {
                    None
                }
            })
            .unwrap();

        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        map.set_style_url(&format!("{base_url}/original-style.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            "/rewritten-style.json"
        );

        runtime.clear_resource_transform().unwrap();
        map.set_style_url(&format!("{base_url}/original-after-clear.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            "/original-after-clear.json"
        );

        map.close().unwrap();
        runtime.close().unwrap();
        server.join().unwrap();
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    #[test]
    // Spec coverage: BND-158 and BND-159.
    fn http_header_transform_reaches_requests_and_clear_stops_it() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let (base_url, requests, server) = spawn_recording_style_server(2);
        runtime
            .set_http_header_transform(|request| {
                assert_eq!(request.kind, ResourceKind::Style);
                vec![crate::HttpHeader::new("X-Map-Token", "secret")]
            })
            .unwrap();

        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        map.set_style_url(&format!("{base_url}/with-header.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        let first = requests.recv_timeout(Duration::from_secs(5)).unwrap();
        assert!(
            first
                .lines()
                .any(|line| line.eq_ignore_ascii_case("X-Map-Token: secret"))
        );

        runtime.clear_http_header_transform().unwrap();
        map.set_style_url(&format!("{base_url}/after-clear.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        let second = requests.recv_timeout(Duration::from_secs(5)).unwrap();
        assert!(
            !second
                .lines()
                .any(|line| line.to_ascii_lowercase().starts_with("x-map-token:"))
        );

        map.close().unwrap();
        runtime.close().unwrap();
        server.join().unwrap();
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    #[test]
    // Spec coverage: BND-158.
    fn http_header_transform_skips_non_http_urls() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        runtime.set_resource_transform(|_| None).unwrap();
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        runtime
            .set_http_header_transform(move |_| {
                callback_calls.fetch_add(1, Ordering::SeqCst);
                Vec::new()
            })
            .unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

        map.set_style_url("jar:file:/packaged/style.json").unwrap();
        let _ = wait_for_map_loading_failure(&mut runtime);
        assert_eq!(calls.load(Ordering::SeqCst), 0);

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    #[test]
    // Spec coverage: BND-159.
    fn http_header_transform_preserves_same_origin_and_strips_cross_origin_redirects() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let (origin_url, requests, servers) = spawn_redirect_style_servers();
        runtime
            .set_http_header_transform(|_| vec![crate::HttpHeader::new("X-Map-Token", "secret")])
            .unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

        map.set_style_url(&format!("{origin_url}/same-start.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            ("/same-start.json".to_owned(), true)
        );
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            ("/same-final.json".to_owned(), true)
        );

        map.set_style_url(&format!("{origin_url}/cross-start.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            ("/cross-start.json".to_owned(), true)
        );
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            ("/cross-final.json".to_owned(), false)
        );

        map.close().unwrap();
        runtime.close().unwrap();
        for server in servers {
            server.join().unwrap();
        }
    }

    // OpenHarmony's platform HTTP client has no redirect-decision hook, and the
    // browser's fetch transport follows redirects itself, so both report header
    // transforms unsupported rather than enabling a transport that cannot
    // satisfy the redirect contract.
    #[cfg(any(target_env = "ohos", target_os = "emscripten"))]
    #[test]
    fn http_header_transform_reports_unsupported() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let error = runtime
            .set_http_header_transform(|_| Vec::new())
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::Unsupported);
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-123.
    fn resource_transform_replacement_after_map_creation_releases_previous_state() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);
        runtime
            .set_resource_transform(move |_| {
                let _ = &first_callback;
                None
            })
            .unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        runtime
            .set_resource_transform(move |_| {
                let _ = &second_callback;
                None
            })
            .unwrap();

        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 2);

        map.close().unwrap();
        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&second), 1);
        assert_eq!(Arc::strong_count(&first), 1);
    }

    #[test]
    // Spec coverage: BND-123.
    fn runtime_teardown_releases_resource_transform_state() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        runtime
            .set_resource_transform(move |_| {
                let _ = &callback_token;
                None
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&token), 2);

        runtime.close().unwrap();

        assert_eq!(Arc::strong_count(&token), 1);
    }

    #[test]
    // Rust regression: documents the Rust binding's late transform-install
    // guard after map creation.
    fn resource_transform_installs_after_map_creation() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

        runtime.set_resource_transform(|_| None).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-140 and BND-123.
    fn resource_transform_clears_after_map_was_closed_and_releases_state() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        runtime
            .set_resource_transform(move |_| {
                let _ = &callback_token;
                None
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&token), 2);

        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        map.close().unwrap();

        runtime.clear_resource_transform().unwrap();

        assert_eq!(Arc::strong_count(&token), 2);

        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&token), 1);
    }

    #[test]
    // Spec coverage: BND-081, BND-090, and BND-092.
    fn a_drain_reports_map_events_in_queue_order_and_copies_outlive_the_batch() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        let map_id = map.id();

        let command_id = map.set_style_json(b"{").unwrap();
        let barrier = runtime.start_barrier().unwrap();
        assert!(barrier.wait(Duration::from_secs(5)).unwrap());
        barrier.discard().unwrap();
        barrier.release();

        let batch = runtime.drain_events(0).unwrap();
        let types = batch
            .iter()
            .map(|event| event.event_type())
            .collect::<Vec<_>>();
        assert!(
            types.len() > 1,
            "a failed style load should queue more than one event, got {types:?}"
        );
        let loading_failed = batch
            .iter()
            .find(|event| event.event_type() == RuntimeEventType::MapLoadingFailed)
            .expect("a malformed style should queue a loading-failed event");
        assert_eq!(loading_failed.source(), RuntimeEventSource::Map(map_id));
        assert!(
            loading_failed
                .message()
                .unwrap()
                .is_some_and(|message| !message.is_empty())
        );
        let command_finished = batch
            .iter()
            .find_map(|event| match event.payload() {
                RuntimeEventPayload::CommandFinished(finished)
                    if finished.command_id == command_id =>
                {
                    Some(finished)
                }
                _ => None,
            })
            .expect("the malformed style command should complete terminally");
        assert_eq!(command_finished.disposition, CommandDisposition::Failed);
        let owned = loading_failed.to_owned().unwrap();

        // A later drain leaves the owned batch readable.
        assert!(runtime.drain_events(0).unwrap().is_empty());
        assert_eq!(loading_failed.source(), RuntimeEventSource::Map(map_id));
        drop(batch);
        assert_eq!(owned.source, RuntimeEventSource::Map(map_id));
        assert_eq!(owned.event_type, RuntimeEventType::MapLoadingFailed);
        assert!(
            owned
                .message
                .as_deref()
                .is_some_and(|message| !message.is_empty())
        );

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-042.
    fn runtime_close_with_live_map_is_rust_invalid_state_and_retryable() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

        let error = runtime.close().unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(error.raw_status(), None);
        let runtime = error.into_handle();

        std::thread::sleep(std::time::Duration::from_millis(1));
        map.close().unwrap();
        runtime.close().unwrap();
    }
}
