use std::cell::{Cell, RefCell};
use std::fmt;
use std::marker::PhantomData;
use std::rc::Rc;
use std::time::Duration;

use maplibre_core::AmbientCacheOperation;
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::events::{OfflineRegionDownloadState, OfflineRegionStatus, RuntimeEventBatch};
use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::resource::{HttpHeaderTransformState, ResourceProviderState, ResourceTransformState};
use crate::{
    Error, ErrorKind, HandleOperationError, OfflineOperationTakeError, ResourceProviderDecision,
    Result, RuntimeEventMask,
};
#[cfg(test)]
use crate::{LatLngBounds, MapHandle, MapOptions};

pub use maplibre_core::runtime::{OfflineRegionDefinition, OfflineRegionInfo, RuntimeOptions};
pub(crate) use maplibre_core::runtime::{
    OfflineRegionDefinitionNativeExt, RuntimeOptionsNativeExt,
};

#[derive(Debug)]
pub(crate) struct RuntimeState {
    handle: ThreadAffineNativeHandle<sys::mln_runtime>,
    resource_transform: RefCell<Option<Box<ResourceTransformState>>>,
    http_header_transform: RefCell<Option<Box<HttpHeaderTransformState>>>,
    resource_provider: RefCell<Option<Box<ResourceProviderState>>>,
}

impl RuntimeState {
    fn new(native: sys::mln_runtime) -> Result<Self> {
        // SAFETY: native came from successful mln_runtime_create and is paired
        // with the matching runtime destroy function.
        let handle = unsafe {
            ThreadAffineNativeHandle::from_handle(native, sys::mln_runtime_destroy, "mln_runtime")
        }?;
        Ok(Self {
            handle,
            resource_transform: RefCell::new(None),
            http_header_transform: RefCell::new(None),
            resource_provider: RefCell::new(None),
        })
    }

    pub(crate) fn native(&self) -> Result<sys::mln_runtime> {
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("RuntimeHandle"))
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn close(&self) -> Result<()> {
        self.handle.close()?;
        self.resource_transform.borrow_mut().take();
        self.http_header_transform.borrow_mut().take();
        self.resource_provider.borrow_mut().take();
        Ok(())
    }

    fn set_resource_provider<F>(&self, callback: F) -> Result<()>
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
    ) -> Result<()>
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
    ) -> Result<()> {
        let runtime = self.native()?;

        // SAFETY: runtime is live and descriptor's user_data points to the boxed
        // replacement, kept alive on success. Native retires the previous
        // provider before returning, so the state dropped below is unreachable.
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_resource_provider(runtime, &descriptor)
        })?;
        self.resource_provider.borrow_mut().replace(replacement);
        Ok(())
    }

    fn clear_resource_provider(&self) -> Result<()> {
        let runtime = self.native()?;

        // SAFETY: runtime is live. Native clear waits for in-flight provider
        // callbacks before returning, so dropping Rust callback state below is safe.
        maplibre_core::check(unsafe { sys::mln_runtime_clear_resource_provider(runtime) })?;
        self.resource_provider.borrow_mut().take();
        Ok(())
    }

    fn set_resource_transform<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        let runtime = self.native()?;
        let replacement = ResourceTransformState::new(callback);
        let descriptor = replacement.descriptor();

        // SAFETY: runtime is live and descriptor's user_data points to
        // replacement, kept alive on success. On failure native preserves the
        // previous transform and replacement is dropped below.
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_resource_transform(runtime, &descriptor)
        })?;
        self.resource_transform.borrow_mut().replace(replacement);
        Ok(())
    }

    fn clear_resource_transform(&self) -> Result<()> {
        let runtime = self.native()?;

        // SAFETY: runtime is live. Native clear waits for in-flight transform
        // callbacks before returning, so dropping Rust callback state below is safe.
        maplibre_core::check(unsafe { sys::mln_runtime_clear_resource_transform(runtime) })?;
        self.resource_transform.borrow_mut().take();
        Ok(())
    }

    fn set_http_header_transform<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::HttpHeaderTransformRequest) -> Vec<crate::HttpHeader> + Send + Sync + 'static,
    {
        let runtime = self.native()?;
        let replacement = HttpHeaderTransformState::new(callback);
        let descriptor = replacement.descriptor();
        // SAFETY: descriptor retains replacement through native registration.
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_http_header_transform(runtime, &descriptor)
        })?;
        self.http_header_transform.borrow_mut().replace(replacement);
        Ok(())
    }

    fn clear_http_header_transform(&self) -> Result<()> {
        let runtime = self.native()?;
        // SAFETY: native waits for in-flight callbacks before returning.
        maplibre_core::check(unsafe { sys::mln_runtime_clear_http_header_transform(runtime) })?;
        self.http_header_transform.borrow_mut().take();
        Ok(())
    }
}

/// Owner-thread runtime handle for MapLibre Native work and event draining.
pub struct RuntimeHandle {
    pub(crate) inner: Rc<RuntimeState>,
}

impl fmt::Debug for RuntimeHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RuntimeHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

/// Owner-thread offline database operation token that must be taken or discarded.
pub struct OfflineOperationHandle<T> {
    runtime: Rc<RuntimeState>,
    operation_id: sys::mln_offline_operation_id,
    operation_kind: maplibre_core::OfflineOperationKind,
    result_kind: maplibre_core::OfflineOperationResultKind,
    live: Cell<bool>,
    _result: PhantomData<fn() -> T>,
    _thread_affine: PhantomData<Rc<()>>,
}

impl<T> fmt::Debug for OfflineOperationHandle<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("OfflineOperationHandle")
            .field("operation_id", &self.operation_id)
            .field("operation_kind", &self.operation_kind)
            .field("result_kind", &self.result_kind)
            .field("live", &self.live.get())
            .finish()
    }
}

impl<T> OfflineOperationHandle<T> {
    fn new(
        runtime: Rc<RuntimeState>,
        operation_id: sys::mln_offline_operation_id,
        operation_kind: maplibre_core::OfflineOperationKind,
        result_kind: maplibre_core::OfflineOperationResultKind,
    ) -> Result<Self> {
        if operation_id == 0 {
            return Err(Error::invalid_argument(
                "offline operation id must not be zero",
            ));
        }
        Ok(Self {
            runtime,
            operation_id,
            operation_kind,
            result_kind,
            live: Cell::new(true),
            _result: PhantomData,
            _thread_affine: PhantomData,
        })
    }

    fn runtime_ptr(&self) -> Result<sys::mln_runtime> {
        if !self.live.get() {
            return Err(closed_handle_error("OfflineOperationHandle"));
        }
        self.runtime.native()
    }

    /// Retires this handle after native took or discarded the operation, which
    /// also drops the operation's undrained completion event from the native
    /// queue.
    fn mark_consumed(&self) {
        self.live.set(false);
    }

    /// Discards runtime-owned state for this offline operation.
    #[allow(clippy::result_large_err)]
    pub fn discard(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if !self.live.get() {
            return Ok(());
        }
        let runtime = match self.runtime_ptr() {
            Ok(runtime) => runtime,
            Err(error) => return Err(HandleOperationError::new(error, self)),
        };
        let status =
            unsafe { sys::mln_runtime_offline_operation_discard(runtime, self.operation_id) };
        if let Err(error) = maplibre_core::check(status) {
            return Err(HandleOperationError::new(error, self));
        }
        self.mark_consumed();
        Ok(())
    }
}

impl<T> Drop for OfflineOperationHandle<T> {
    fn drop(&mut self) {
        if !self.live.get() {
            return;
        }
        if let Ok(runtime) = self.runtime.native() {
            // SAFETY: Safe Rust keeps this !Send/!Sync handle on the runtime owner thread.
            let status =
                unsafe { sys::mln_runtime_offline_operation_discard(runtime, self.operation_id) };
            if status == sys::MLN_STATUS_OK {
                self.mark_consumed();
            }
        }
    }
}

impl OfflineOperationHandle<OfflineRegionInfo> {
    /// Takes a completed create/update operation result as copied region info.
    #[allow(clippy::result_large_err)]
    pub fn take(self) -> std::result::Result<OfflineRegionInfo, OfflineOperationTakeError<Self>> {
        let runtime = match self.runtime_ptr() {
            Ok(runtime) => runtime,
            Err(error) => return Err(OfflineOperationTakeError::retryable(error, self)),
        };
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_snapshot>::new();
        let status = match self.operation_kind {
            maplibre_core::OfflineOperationKind::RegionCreate => unsafe {
                sys::mln_runtime_offline_region_create_take_result(
                    runtime,
                    self.operation_id,
                    out.as_mut_ptr(),
                )
            },
            maplibre_core::OfflineOperationKind::RegionUpdateMetadata => unsafe {
                sys::mln_runtime_offline_region_update_metadata_take_result(
                    runtime,
                    self.operation_id,
                    out.as_mut_ptr(),
                )
            },
            _ => sys::MLN_STATUS_INVALID_STATE,
        };
        if let Err(error) = maplibre_core::check(status) {
            return Err(OfflineOperationTakeError::retryable(error, self));
        }
        self.mark_consumed();
        let snapshot = match out.into_live("mln_offline_region_snapshot") {
            Ok(snapshot) => snapshot,
            Err(error) => return Err(OfflineOperationTakeError::consumed(error)),
        };
        // SAFETY: On success, the C API returns an owned snapshot handle;
        // core copies and releases it.
        unsafe { maplibre_core::runtime::copy_offline_region_snapshot(snapshot) }
            .map_err(OfflineOperationTakeError::consumed)
    }
}

impl OfflineOperationHandle<Option<OfflineRegionInfo>> {
    /// Takes a completed get operation result as optional copied region info.
    #[allow(clippy::result_large_err)]
    pub fn take(
        self,
    ) -> std::result::Result<Option<OfflineRegionInfo>, OfflineOperationTakeError<Self>> {
        let runtime = match self.runtime_ptr() {
            Ok(runtime) => runtime,
            Err(error) => return Err(OfflineOperationTakeError::retryable(error, self)),
        };
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_snapshot>::new();
        let mut found = false;
        let status = unsafe {
            sys::mln_runtime_offline_region_get_take_result(
                runtime,
                self.operation_id,
                out.as_mut_ptr(),
                &mut found,
            )
        };
        if let Err(error) = maplibre_core::check(status) {
            return Err(OfflineOperationTakeError::retryable(error, self));
        }
        self.mark_consumed();
        if !found {
            return Ok(None);
        }
        let snapshot = match out.into_live("mln_offline_region_snapshot") {
            Ok(snapshot) => snapshot,
            Err(error) => return Err(OfflineOperationTakeError::consumed(error)),
        };
        // SAFETY: When found is true, the C API returns an owned snapshot
        // handle; core copies and releases it.
        Ok(Some(
            unsafe { maplibre_core::runtime::copy_offline_region_snapshot(snapshot) }
                .map_err(OfflineOperationTakeError::consumed)?,
        ))
    }
}

impl OfflineOperationHandle<Vec<OfflineRegionInfo>> {
    /// Takes a completed list/merge operation result as copied region info.
    #[allow(clippy::result_large_err)]
    pub fn take(
        self,
    ) -> std::result::Result<Vec<OfflineRegionInfo>, OfflineOperationTakeError<Self>> {
        let runtime = match self.runtime_ptr() {
            Ok(runtime) => runtime,
            Err(error) => return Err(OfflineOperationTakeError::retryable(error, self)),
        };
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_offline_region_list>::new();
        let status = match self.operation_kind {
            maplibre_core::OfflineOperationKind::RegionsList => unsafe {
                sys::mln_runtime_offline_regions_list_take_result(
                    runtime,
                    self.operation_id,
                    out.as_mut_ptr(),
                )
            },
            maplibre_core::OfflineOperationKind::RegionsMergeDatabase => unsafe {
                sys::mln_runtime_offline_regions_merge_database_take_result(
                    runtime,
                    self.operation_id,
                    out.as_mut_ptr(),
                )
            },
            _ => sys::MLN_STATUS_INVALID_STATE,
        };
        if let Err(error) = maplibre_core::check(status) {
            return Err(OfflineOperationTakeError::retryable(error, self));
        }
        self.mark_consumed();
        let list = match out.into_live("mln_offline_region_list") {
            Ok(list) => list,
            Err(error) => return Err(OfflineOperationTakeError::consumed(error)),
        };
        // SAFETY: On success, the C API returns an owned list handle; core
        // copies and releases it.
        unsafe { maplibre_core::runtime::copy_offline_region_list(list) }
            .map_err(OfflineOperationTakeError::consumed)
    }
}

impl OfflineOperationHandle<OfflineRegionStatus> {
    /// Takes a completed status operation result as copied status data.
    #[allow(clippy::result_large_err)]
    pub fn take(self) -> std::result::Result<OfflineRegionStatus, HandleOperationError<Self>> {
        let runtime = match self.runtime_ptr() {
            Ok(runtime) => runtime,
            Err(error) => return Err(HandleOperationError::new(error, self)),
        };
        let mut raw = maplibre_core::events::empty_offline_region_status_native();
        let status = unsafe {
            sys::mln_runtime_offline_region_get_status_take_result(
                runtime,
                self.operation_id,
                &mut raw,
            )
        };
        if let Err(error) = maplibre_core::check(status) {
            return Err(HandleOperationError::new(error, self));
        }
        self.mark_consumed();
        Ok(maplibre_core::events::offline_region_status_from_native(
            raw,
        ))
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
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_runtime>::new();
        // SAFETY: options is either null to request native defaults or points to
        // a materialized mln_runtime_options value whose backing strings live
        // for this call. out is a valid null-initialized out-pointer owned by
        // this call.
        maplibre_core::check(unsafe { sys::mln_runtime_create(options, out.as_mut_ptr()) })?;
        let ptr = out_handle(out, "mln_runtime")?;

        Ok(Self {
            inner: Rc::new(RuntimeState::new(ptr)?),
        })
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
    pub fn set_resource_provider<F>(&self, callback: F) -> Result<()>
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
    pub fn clear_resource_provider(&self) -> Result<()> {
        self.inner.clear_resource_provider()
    }

    /// Installs or replaces the runtime-scoped network URL transform.
    ///
    /// Native may invoke the closure from worker or network threads, so keep it
    /// quick and call no MapLibre Native APIs from it. `Some(url)` replaces the
    /// request URL; `None`, an empty string, or a panic keeps the original.
    pub fn set_resource_transform<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        self.inner.set_resource_transform(callback)
    }

    /// Clears the runtime-scoped network URL transform. The clear waits for
    /// in-flight transform callbacks before returning and then releases the
    /// Rust callback state.
    pub fn clear_resource_transform(&self) -> Result<()> {
        self.inner.clear_resource_transform()
    }

    /// Installs or replaces the runtime-scoped outgoing HTTP header transform.
    ///
    /// Native invokes the closure synchronously on worker or network threads
    /// after URL transformation. Returned headers are copied before the closure
    /// returns. Panics, duplicate names, and invalid headers leave the request
    /// unchanged.
    pub fn set_http_header_transform<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::HttpHeaderTransformRequest) -> Vec<crate::HttpHeader> + Send + Sync + 'static,
    {
        self.inner.set_http_header_transform(callback)
    }

    /// Clears the runtime-scoped outgoing HTTP header transform.
    pub fn clear_http_header_transform(&self) -> Result<()> {
        self.inner.clear_http_header_transform()
    }

    fn start_operation<T>(
        &self,
        operation_id: sys::mln_offline_operation_id,
        operation_kind: maplibre_core::OfflineOperationKind,
        result_kind: maplibre_core::OfflineOperationResultKind,
    ) -> Result<OfflineOperationHandle<T>> {
        OfflineOperationHandle::new(
            Rc::clone(&self.inner),
            operation_id,
            operation_kind,
            result_kind,
        )
    }

    /// Starts an ambient cache maintenance operation for this runtime.
    pub fn start_ambient_cache_operation(
        &self,
        operation: AmbientCacheOperation,
    ) -> Result<OfflineOperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_run_ambient_cache_operation_start(
                runtime,
                operation.to_native(),
                &mut operation_id,
            )
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::AmbientCache,
            maplibre_core::OfflineOperationResultKind::None,
        )
    }

    /// Starts a change to this runtime's maximum ambient cache size.
    ///
    /// MapLibre evicts ambient resources to fit the new budget, so lowering it
    /// discards cached resources. Offline regions are unaffected.
    pub fn start_set_maximum_ambient_cache_size(
        &self,
        size: u64,
    ) -> Result<OfflineOperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_maximum_ambient_cache_size_start(runtime, size, &mut operation_id)
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::SetMaximumAmbientCacheSize,
            maplibre_core::OfflineOperationResultKind::None,
        )
    }

    /// Starts creating an offline region.
    pub fn start_create_offline_region(
        &self,
        definition: &OfflineRegionDefinition,
        metadata: &[u8],
    ) -> Result<OfflineOperationHandle<OfflineRegionInfo>> {
        let runtime = self.inner.native()?;
        let definition = definition.to_native()?;
        let raw_definition = definition.to_raw();
        let mut operation_id: sys::mln_offline_operation_id = 0;
        // SAFETY: runtime is live. raw_definition points into definition-owned
        // string and geometry storage, metadata storage is valid for this call.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_create_start(
                runtime,
                &raw_definition,
                maplibre_core::runtime::metadata_ptr(metadata),
                metadata.len(),
                &mut operation_id,
            )
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::RegionCreate,
            maplibre_core::OfflineOperationResultKind::Region,
        )
    }

    /// Starts getting an offline region snapshot by ID.
    pub fn start_offline_region(
        &self,
        region_id: i64,
    ) -> Result<OfflineOperationHandle<Option<OfflineRegionInfo>>> {
        let runtime = self.inner.native()?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        // SAFETY: runtime is live and operation_id points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_get_start(runtime, region_id, &mut operation_id)
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::RegionGet,
            maplibre_core::OfflineOperationResultKind::OptionalRegion,
        )
    }

    /// Starts listing offline regions in this runtime's database.
    pub fn start_offline_regions(&self) -> Result<OfflineOperationHandle<Vec<OfflineRegionInfo>>> {
        let runtime = self.inner.native()?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        // SAFETY: runtime is live and operation_id points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_regions_list_start(runtime, &mut operation_id)
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::RegionsList,
            maplibre_core::OfflineOperationResultKind::RegionList,
        )
    }

    /// Starts merging offline regions from another database path.
    pub fn start_merge_offline_regions_database(
        &self,
        path: &str,
    ) -> Result<OfflineOperationHandle<Vec<OfflineRegionInfo>>> {
        let runtime = self.inner.native()?;
        let path = maplibre_core::string::c_string(path)?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        // SAFETY: runtime is live, path is NUL-terminated and valid for this
        // call, and operation_id points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_regions_merge_database_start(
                runtime,
                path.as_ptr(),
                &mut operation_id,
            )
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::RegionsMergeDatabase,
            maplibre_core::OfflineOperationResultKind::RegionList,
        )
    }

    /// Starts updating opaque metadata for an offline region.
    pub fn start_update_offline_region_metadata(
        &self,
        region_id: i64,
        metadata: &[u8],
    ) -> Result<OfflineOperationHandle<OfflineRegionInfo>> {
        let runtime = self.inner.native()?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        // SAFETY: runtime is live, metadata storage is valid for this call, and
        // operation_id points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_update_metadata_start(
                runtime,
                region_id,
                maplibre_core::runtime::metadata_ptr(metadata),
                metadata.len(),
                &mut operation_id,
            )
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::RegionUpdateMetadata,
            maplibre_core::OfflineOperationResultKind::Region,
        )
    }

    /// Starts getting the current completed/download status for an offline region.
    pub fn start_offline_region_status(
        &self,
        region_id: i64,
    ) -> Result<OfflineOperationHandle<OfflineRegionStatus>> {
        let runtime = self.inner.native()?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        // SAFETY: runtime is live and operation_id points to writable storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_get_status_start(runtime, region_id, &mut operation_id)
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::RegionGetStatus,
            maplibre_core::OfflineOperationResultKind::RegionStatus,
        )
    }

    /// Starts enabling or disabling runtime events for an offline region.
    pub fn start_set_offline_region_observed(
        &self,
        region_id: i64,
        observed: bool,
    ) -> Result<OfflineOperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_set_observed_start(
                runtime,
                region_id,
                observed,
                &mut operation_id,
            )
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::RegionSetObserved,
            maplibre_core::OfflineOperationResultKind::None,
        )
    }

    /// Starts setting an offline region's native download state.
    pub fn start_set_offline_region_download_state(
        &self,
        region_id: i64,
        state: OfflineRegionDownloadState,
    ) -> Result<OfflineOperationHandle<()>> {
        let runtime = self.inner.native()?;
        let state = state.raw_for_set()?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_set_download_state_start(
                runtime,
                region_id,
                state,
                &mut operation_id,
            )
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::RegionSetDownloadState,
            maplibre_core::OfflineOperationResultKind::None,
        )
    }

    /// Starts invalidating cached resources for an offline region.
    pub fn start_invalidate_offline_region(
        &self,
        region_id: i64,
    ) -> Result<OfflineOperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_invalidate_start(runtime, region_id, &mut operation_id)
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::RegionInvalidate,
            maplibre_core::OfflineOperationResultKind::None,
        )
    }

    /// Starts deleting an offline region.
    pub fn start_delete_offline_region(
        &self,
        region_id: i64,
    ) -> Result<OfflineOperationHandle<()>> {
        let runtime = self.inner.native()?;
        let mut operation_id: sys::mln_offline_operation_id = 0;
        maplibre_core::check(unsafe {
            sys::mln_runtime_offline_region_delete_start(runtime, region_id, &mut operation_id)
        })?;
        self.start_operation(
            operation_id,
            maplibre_core::OfflineOperationKind::RegionDelete,
            maplibre_core::OfflineOperationResultKind::None,
        )
    }

    /// Advances this runtime: parks the owner thread when `timeout` allows it,
    /// then drains the owner-thread task queues. Drain the queued runtime
    /// events with [`Self::drain_events`] afterwards.
    ///
    /// `Some(Duration::ZERO)` drains and returns, a longer `Some` parks for up
    /// to that long, and `None` parks until a wake arrives. The drain runs
    /// every task queued when it begins plus every task those enqueue, so a
    /// single call can span a full style parse.
    ///
    /// `budget` bounds the drain. `None` drains without a bound; a `Some`
    /// value stops the drain at the first task boundary after that long,
    /// measured from the start of the drain. The first queued task always
    /// runs, so a bounded pump always makes progress, and tasks left behind
    /// set the wake flag so the next pump returns without parking and
    /// continues them. The budget bounds the task queues alone: expired timers
    /// and ready file descriptors are serviced regardless, and a single task
    /// runs to completion once started, so one long task can overrun the
    /// budget.
    ///
    /// A parking call returns as soon as the runtime's wake flag is set, and
    /// returns without parking while unread runtime events are queued. Timers
    /// and ready file descriptors set the flag only when they queue
    /// owner-thread work, so pass a bounded timeout to cap how long a call
    /// waits.
    ///
    /// A non-zero timeout blocks the calling thread. Call it outside any lock
    /// that a thread signalling a [`WakeSource`] takes, and outside native
    /// callbacks.
    pub fn pump(&self, timeout: Option<Duration>, budget: Option<Duration>) -> Result<()> {
        let runtime = self.inner.native()?;
        let timeout_ms = timeout.map_or(-1, |timeout| {
            i64::try_from(timeout.as_millis()).unwrap_or(i64::MAX)
        });
        let budget_ms = budget.map_or(-1, |budget| {
            i64::try_from(budget.as_millis()).unwrap_or(i64::MAX)
        });
        // SAFETY: runtime is a live runtime handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_runtime_pump(runtime, timeout_ms, budget_ms) })
    }

    /// Acquires a [`WakeSource`] that releases this runtime's parked owner
    /// thread. The returned source is usable from any thread.
    pub fn wake_source(&self) -> Result<WakeSource> {
        let runtime = self.inner.native()?;
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_wake_source>::new();
        // SAFETY: runtime is a live runtime handle owned by this wrapper, and
        // out is a valid null-initialized out-pointer owned by this call.
        maplibre_core::check(unsafe {
            sys::mln_runtime_wake_source_acquire(runtime, out.as_mut_ptr())
        })?;
        Ok(WakeSource {
            handle: out_handle(out, "mln_wake_source")?,
        })
    }

    /// Drains this runtime's queued events into one batch borrowed from
    /// runtime-owned storage.
    ///
    /// `max_events` bounds the drain. Zero drains every queued event, and a
    /// positive value drains at most that many and reports the rest through
    /// [`RuntimeEventBatch::remaining`].
    ///
    /// The returned batch borrows this handle, so the next drain waits until
    /// the batch is dropped, and an event read out of the batch borrows the
    /// batch. Take [`crate::RuntimeEventRef::to_owned`] for a value that
    /// outlives either.
    ///
    /// A drain invalidates the batch before it, which the mutable borrow turns
    /// into a compile error:
    ///
    /// ```compile_fail,E0499
    /// # use maplibre_native_ffi::{RuntimeHandle, RuntimeOptions};
    /// let mut runtime = RuntimeHandle::with_options(&RuntimeOptions::default()).unwrap();
    /// let batch = runtime.drain_events(0).unwrap();
    /// let next = runtime.drain_events(0).unwrap();
    /// let _ = batch.len();
    /// ```
    pub fn drain_events(&mut self, max_events: usize) -> Result<RuntimeEventBatch<'_>> {
        let runtime = self.inner.native()?;
        // SAFETY: The default constructor takes no arguments and fills the size
        // field for this C ABI version.
        let mut raw = unsafe { sys::mln_runtime_event_batch_default() };
        // SAFETY: runtime is live, and raw points to writable batch storage
        // whose size field this call reads.
        maplibre_core::check(unsafe {
            sys::mln_runtime_drain_events(runtime, max_events, &mut raw)
        })?;
        // SAFETY: raw came from a successful drain on this runtime, so its
        // storage stays readable until the next drain, which the returned
        // batch's borrow of this handle keeps out of the way.
        Ok(unsafe { RuntimeEventBatch::new(raw) })
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

    /// Explicitly destroys the runtime. A failed destroy leaves the native
    /// handle live so child handles can still close safely.
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if self.inner.is_closed() {
            return Ok(());
        }
        if Rc::strong_count(&self.inner) > 1 {
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

/// Releases a runtime owner thread parked in [`RuntimeHandle::pump`].
///
/// Signalling and destruction are callable from any thread. Each source holds
/// its own reference to the runtime's wake state, so it stays usable after the
/// runtime closes, where signalling does nothing.
#[derive(Debug)]
pub struct WakeSource {
    handle: sys::mln_wake_source,
}

impl WakeSource {
    /// Sets the runtime's wake flag and releases the parked owner thread.
    ///
    /// A signal raised while the owner thread is running sets the wake flag,
    /// so the next [`RuntimeHandle::pump`] returns without parking. Signalling
    /// after the runtime closes succeeds and does nothing.
    pub fn signal(&self) -> Result<()> {
        // SAFETY: handle is a live wake source owned by this wrapper, and
        // native accepts signals from any thread.
        maplibre_core::check(unsafe { sys::mln_wake_source_signal(self.handle) })
    }
}

impl Drop for WakeSource {
    fn drop(&mut self) {
        // SAFETY: handle is a live wake source this wrapper owns and destroys
        // exactly once, and native accepts destruction from any thread.
        unsafe { sys::mln_wake_source_destroy(self.handle) };
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
        ErrorKind, OfflineOperationCompletedEvent, ResourceErrorReason, ResourceKind,
        ResourceProviderDecision, ResourceResponse, RuntimeEvent, RuntimeEventPayload,
        RuntimeEventSource, RuntimeEventType,
    };
    use maplibre_core::{OfflineOperationKind as Op, OfflineOperationResultKind as OpResult};

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
        runtime: &mut RuntimeHandle,
        operation: &OfflineOperationHandle<T>,
        operation_kind: Op,
        result_kind: OpResult,
    ) -> Result<OfflineOperationCompletedEvent> {
        let deadline = Instant::now() + Duration::from_secs(30);
        loop {
            if Instant::now() >= deadline {
                return Err(Error::new(
                    ErrorKind::InvalidState,
                    None,
                    format!(
                        "timed out waiting for offline operation {:?}/{:?} with id {}",
                        operation_kind, result_kind, operation.operation_id
                    ),
                ));
            }
            runtime.pump(Some(Duration::ZERO), None)?;
            let mut outcome = None;
            for event in runtime.drain_events(0)?.iter() {
                let RuntimeEventPayload::OfflineOperationCompleted(completed) = event.payload()
                else {
                    continue;
                };
                if completed.operation_id != operation.operation_id {
                    continue;
                }
                assert_eq!(completed.operation_kind, operation_kind);
                assert_eq!(completed.raw_operation_kind, operation_kind.raw_value());
                assert_eq!(completed.result_kind, result_kind);
                assert_eq!(completed.raw_result_kind, result_kind.raw_value());
                outcome = Some((completed, event.message()?.unwrap_or_default().to_owned()));
                break;
            }
            if let Some((completed, message)) = outcome {
                if completed.result_status != sys::MLN_STATUS_OK {
                    return Err(Error::from_status_and_diagnostic(
                        completed.result_status,
                        message,
                    ));
                }
                return Ok(completed);
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
            let completed =
                wait_for_operation(&mut runtime, &operation, Op::AmbientCache, OpResult::None)
                    .unwrap();
            assert_eq!(completed.operation_id, operation.operation_id);
            operation.discard().unwrap();
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

        // Raising then lowering the budget both report through the same event.
        for size in [8 * 1024 * 1024, 0] {
            let operation = runtime.start_set_maximum_ambient_cache_size(size).unwrap();
            let completed = wait_for_operation(
                &mut runtime,
                &operation,
                Op::SetMaximumAmbientCacheSize,
                OpResult::None,
            )
            .unwrap();
            assert_eq!(completed.operation_id, operation.operation_id);
            operation.discard().unwrap();
        }

        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-084.
    fn offline_take_result_failure_returns_live_handle() {
        let mut options = RuntimeOptions::default();
        options.cache_path = Some(":memory:".into());
        let runtime = RuntimeHandle::with_options(&options).unwrap();
        let ambient = runtime
            .start_ambient_cache_operation(AmbientCacheOperation::Clear)
            .unwrap();
        let region_result = runtime
            .start_operation::<OfflineRegionInfo>(
                ambient.operation_id,
                maplibre_core::OfflineOperationKind::RegionCreate,
                maplibre_core::OfflineOperationResultKind::Region,
            )
            .unwrap();

        let error = region_result.take().unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        let region_result = error.into_retryable().unwrap().into_handle();
        region_result.discard().unwrap();
        drop(ambient);
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
        wait_for_operation(&mut runtime, &create, Op::RegionCreate, OpResult::Region).unwrap();
        let created = create.take().unwrap();
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
        wait_for_operation(
            &mut runtime,
            &create_geometry,
            Op::RegionCreate,
            OpResult::Region,
        )
        .unwrap();
        let geometry_region = create_geometry.take().unwrap();
        assert_eq!(geometry_region.definition, geometry_definition);
        assert_eq!(geometry_region.metadata, b"geo");

        let get = runtime.start_offline_region(created.id).unwrap();
        wait_for_operation(&mut runtime, &get, Op::RegionGet, OpResult::OptionalRegion).unwrap();
        let fetched = get.take().unwrap().unwrap();
        assert_eq!(fetched, created);

        let list = runtime.start_offline_regions().unwrap();
        wait_for_operation(&mut runtime, &list, Op::RegionsList, OpResult::RegionList).unwrap();
        let listed = list.take().unwrap();
        assert!(listed.iter().any(|region| region.id == created.id));

        let update = runtime
            .start_update_offline_region_metadata(created.id, b"")
            .unwrap();
        wait_for_operation(
            &mut runtime,
            &update,
            Op::RegionUpdateMetadata,
            OpResult::Region,
        )
        .unwrap();
        let updated = update.take().unwrap();
        assert_eq!(updated.id, created.id);
        assert!(updated.metadata.is_empty());

        let status_operation = runtime.start_offline_region_status(created.id).unwrap();
        wait_for_operation(
            &mut runtime,
            &status_operation,
            Op::RegionGetStatus,
            OpResult::RegionStatus,
        )
        .unwrap();
        let status = status_operation.take().unwrap();
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
        wait_for_operation(
            &mut runtime,
            &set_inactive,
            Op::RegionSetDownloadState,
            OpResult::None,
        )
        .unwrap();
        set_inactive.discard().unwrap();
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
        wait_for_operation(
            &mut runtime,
            &observe,
            Op::RegionSetObserved,
            OpResult::None,
        )
        .unwrap();
        observe.discard().unwrap();
        let unobserve = runtime
            .start_set_offline_region_observed(created.id, false)
            .unwrap();
        wait_for_operation(
            &mut runtime,
            &unobserve,
            Op::RegionSetObserved,
            OpResult::None,
        )
        .unwrap();
        unobserve.discard().unwrap();
        let invalidate = runtime.start_invalidate_offline_region(created.id).unwrap();
        wait_for_operation(
            &mut runtime,
            &invalidate,
            Op::RegionInvalidate,
            OpResult::None,
        )
        .unwrap();
        invalidate.discard().unwrap();
        let delete = runtime.start_delete_offline_region(created.id).unwrap();
        wait_for_operation(&mut runtime, &delete, Op::RegionDelete, OpResult::None).unwrap();
        delete.discard().unwrap();
        let delete_geometry = runtime
            .start_delete_offline_region(geometry_region.id)
            .unwrap();
        wait_for_operation(
            &mut runtime,
            &delete_geometry,
            Op::RegionDelete,
            OpResult::None,
        )
        .unwrap();
        delete_geometry.discard().unwrap();

        let missing_created = runtime.start_offline_region(created.id).unwrap();
        wait_for_operation(
            &mut runtime,
            &missing_created,
            Op::RegionGet,
            OpResult::OptionalRegion,
        )
        .unwrap();
        assert!(missing_created.take().unwrap().is_none());
        let missing_geometry = runtime.start_offline_region(geometry_region.id).unwrap();
        wait_for_operation(
            &mut runtime,
            &missing_geometry,
            Op::RegionGet,
            OpResult::OptionalRegion,
        )
        .unwrap();
        assert!(missing_geometry.take().unwrap().is_none());

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
            wait_for_operation(
                &mut side_runtime,
                &create,
                Op::RegionCreate,
                OpResult::Region,
            )
            .unwrap();
            create.take().unwrap();
            side_runtime.close().unwrap();
        }

        let mut main_options = RuntimeOptions::default();
        main_options.cache_path = Some(main_cache.to_string_lossy().into_owned());
        let mut main_runtime = RuntimeHandle::with_options(&main_options).unwrap();
        let merge = main_runtime
            .start_merge_offline_regions_database(&side_cache.to_string_lossy())
            .unwrap();
        wait_for_operation(
            &mut main_runtime,
            &merge,
            Op::RegionsMergeDatabase,
            OpResult::RegionList,
        )
        .unwrap();
        let merged = merge.take().unwrap();
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

        runtime.pump(Some(Duration::ZERO), None).unwrap();
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

    fn drain_holds_event_type(runtime: &mut RuntimeHandle, event_type: RuntimeEventType) -> bool {
        runtime
            .drain_events(0)
            .unwrap()
            .iter()
            .any(|event| event.event_type() == event_type)
    }

    fn wait_for_runtime_event(runtime: &mut RuntimeHandle, event_type: RuntimeEventType) -> bool {
        for _ in 0..100 {
            let _ = runtime.pump(Some(Duration::ZERO), None);
            if drain_holds_event_type(runtime, event_type) {
                return true;
            }
            std::thread::sleep(Duration::from_millis(10));
        }
        false
    }

    fn wait_for_map_loading_failure(runtime: &mut RuntimeHandle) -> RuntimeEvent {
        for _ in 0..100 {
            runtime.pump(Some(Duration::ZERO), None).unwrap();
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
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();

        runtime.pump(Some(Duration::ZERO), None).unwrap();
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

    // Pumps until the runtime is idle, so a park that follows is released by
    // the signal the test raises.
    fn quiesce(runtime: &mut RuntimeHandle) {
        for _ in 0..100 {
            runtime.pump(Some(Duration::ZERO), None).unwrap();
            if runtime.drain_events(0).unwrap().is_empty() {
                return;
            }
        }
        panic!("the runtime kept producing events while idle");
    }

    // Drives the runtime the way a parked host does, so a missing wake shows up
    // as an expired timeout.
    fn park_for_runtime_event(runtime: &mut RuntimeHandle, event_type: RuntimeEventType) -> bool {
        let started = Instant::now();
        for _ in 0..20 {
            runtime.pump(Some(Duration::from_secs(10)), None).unwrap();
            assert!(
                started.elapsed() < Duration::from_secs(5),
                "parks sat out their timeouts instead of taking wakes"
            );
            if drain_holds_event_type(runtime, event_type) {
                return true;
            }
        }
        false
    }

    #[test]
    // Spec coverage: BND-088.
    fn parked_owner_thread_wakes_for_native_work_and_for_a_wake_source() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        runtime
            .set_resource_provider(move |request, handle| {
                if request.requested_url != "custom://style.json" {
                    return ResourceProviderDecision::PassThrough;
                }
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
        assert!(park_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));

        // A source moved to another thread matches a host's submission path,
        // and the park it releases has no other work to end it.
        let source = runtime.wake_source().unwrap();
        quiesce(&mut runtime);
        let signaller = std::thread::spawn(move || {
            std::thread::sleep(Duration::from_millis(20));
            source.signal().unwrap();
            source
        });
        let started = Instant::now();
        runtime.pump(Some(Duration::from_secs(10)), None).unwrap();
        assert!(
            started.elapsed() < Duration::from_secs(5),
            "the parked owner thread timed out instead of taking the signal"
        );
        let source = signaller.join().unwrap();

        // A wake source stays usable after its runtime closes, so hosts tear
        // the two down in either order.
        map.close().unwrap();
        runtime.close().unwrap();
        source.signal().unwrap();
    }

    #[test]
    // Spec coverage: BND-089.
    fn a_pump_clears_the_wake_flag_it_returns_on() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let source = runtime.wake_source().unwrap();
        quiesce(&mut runtime);

        source.signal().unwrap();
        let started = Instant::now();
        runtime.pump(Some(Duration::from_secs(10)), None).unwrap();
        assert!(
            started.elapsed() < Duration::from_secs(5),
            "a pump waited even though the wake flag was set"
        );

        // The pump above cleared the wake flag, so this one waits its full timeout.
        let started = Instant::now();
        runtime
            .pump(Some(Duration::from_millis(200)), None)
            .unwrap();
        assert!(
            started.elapsed() >= Duration::from_millis(100),
            "the first pump left the wake flag set"
        );

        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-090.
    fn a_bounded_drain_reports_the_events_it_left_queued() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        map.set_style_json(PROVIDER_STYLE_JSON.as_bytes()).unwrap();
        runtime.pump(Some(Duration::ZERO), None).unwrap();

        let bounded = runtime.drain_events(1).unwrap();
        assert_eq!(bounded.len(), 1);
        assert!(
            bounded.remaining() > 0,
            "a style load should queue more than one event"
        );
        let rest = runtime.drain_events(0).unwrap();
        assert!(rest.len() > 1, "one drain should report the whole queue");
        assert_eq!(rest.remaining(), 0);
        let first = rest.iter().next().unwrap();
        assert_eq!(first.source(), RuntimeEventSource::Map(map.id()));

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-060 and BND-091.
    fn a_creation_mask_narrows_a_runtime_before_its_first_operation() {
        let mut options = crate::RuntimeOptions::default();
        options.event_mask = RuntimeEventMask::OFFLINE_OPERATION_COMPLETED;
        let mut runtime = RuntimeHandle::with_options(&options).unwrap();

        assert_eq!(
            runtime.event_mask().unwrap(),
            RuntimeEventMask::OFFLINE_OPERATION_COMPLETED
        );

        // The narrowed runtime still reports the completion of the operation
        // this test starts, and the ambient cache operation needs no database.
        let operation = runtime
            .start_ambient_cache_operation(AmbientCacheOperation::Clear)
            .unwrap();
        let completed =
            wait_for_operation(&mut runtime, &operation, Op::AmbientCache, OpResult::None).unwrap();
        assert_eq!(completed.operation_id, operation.operation_id);
        operation.discard().unwrap();

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
        assert!(read_back.contains(RuntimeEventMask::OFFLINE_OPERATION_COMPLETED));
        assert!(read_back.contains(RuntimeEventMask::MAP_STYLE_LOADED));

        let undefined = RuntimeEventMask::from_bits_retain(1 << 63);
        let error = runtime.set_event_mask(undefined).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert_eq!(runtime.event_mask().unwrap(), read_back);

        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-020, BND-022, BND-190, and BND-191.
    fn runtime_wrong_thread_status_maps_error_and_copies_diagnostic() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let runtime_handle = runtime.inner.native().unwrap();

        let error = std::thread::spawn(move || {
            // SAFETY: This intentionally exercises the C API's owner-thread
            // validation path with a live runtime handle from another thread.
            maplibre_core::check(unsafe { sys::mln_runtime_pump(runtime_handle, 0, -1) })
                .unwrap_err()
        })
        .join()
        .unwrap();

        assert_eq!(error.kind(), ErrorKind::WrongThread);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_WRONG_THREAD));
        assert!(!error.diagnostic().is_empty());
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
        assert_eq!(Arc::strong_count(&first), 1);
        assert_eq!(Arc::strong_count(&second), 2);

        runtime.clear_resource_provider().unwrap();
        assert_eq!(Arc::strong_count(&second), 1);

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
        assert_eq!(Arc::strong_count(&first), 1);
        assert_eq!(Arc::strong_count(&second), 2);

        runtime.clear_resource_transform().unwrap();
        assert_eq!(Arc::strong_count(&second), 1);
        runtime.close().unwrap();
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
        assert!(
            map.style_layer_ids()
                .unwrap()
                .iter()
                .any(|id| id == "rewritten")
        );

        runtime.clear_resource_transform().unwrap();
        map.set_style_url(&format!("{origin}/__fixture/original-after-clear.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert!(
            map.style_layer_ids()
                .unwrap()
                .iter()
                .any(|id| id == "original-after-clear")
        );

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

        assert_eq!(Arc::strong_count(&first), 1);
        assert_eq!(Arc::strong_count(&second), 2);

        map.close().unwrap();
        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&second), 1);
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

        assert_eq!(Arc::strong_count(&token), 1);

        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-081, BND-090, and BND-092.
    fn a_drain_reports_map_events_in_queue_order_and_copies_outlive_the_batch() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
        let map_id = map.id();

        let error = map.set_style_json(b"{").unwrap_err();
        assert_eq!(error.kind(), ErrorKind::NativeError);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_NATIVE_ERROR));

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
        let owned = loading_failed.to_owned().unwrap();

        // The next drain ends the previous batch's window; the copy is
        // untouched.
        assert!(runtime.drain_events(0).unwrap().is_empty());
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

        runtime.pump(Some(Duration::ZERO), None).unwrap();
        map.close().unwrap();
        runtime.close().unwrap();
    }
}
