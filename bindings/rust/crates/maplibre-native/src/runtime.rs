use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::fmt;
use std::marker::PhantomData;
use std::rc::{Rc, Weak};

use maplibre_core::AmbientCacheOperation;
use maplibre_native_core as maplibre_core;
use maplibre_native_sys as sys;

use crate::events::{
    MapId, OfflineRegionDownloadState, OfflineRegionStatus, RuntimeEvent, RuntimeEventSource,
    empty_runtime_event,
};
use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::map::MapState;
use crate::resource::{ResourceProviderState, ResourceTransformState};
use crate::{
    Error, ErrorKind, HandleOperationError, MapHandle, MapOptions, ResourceProviderDecision, Result,
};
#[cfg(test)]
use crate::{Geometry, LatLngBounds};

pub use maplibre_core::runtime::{OfflineRegionDefinition, OfflineRegionInfo, RuntimeOptions};
pub(crate) use maplibre_core::runtime::{
    OfflineRegionDefinitionNativeExt, RuntimeOptionsNativeExt,
};

#[derive(Debug)]
pub(crate) struct RuntimeState {
    handle: ThreadAffineNativeHandle<sys::mln_runtime>,
    next_map_id: Cell<u64>,
    has_created_map: Cell<bool>,
    map_ids: RefCell<HashMap<usize, MapId>>,
    map_states: RefCell<HashMap<usize, Weak<MapState>>>,
    resource_transform: RefCell<Option<Box<ResourceTransformState>>>,
    resource_provider: RefCell<Option<Box<ResourceProviderState>>>,
}

impl RuntimeState {
    fn new(ptr: std::ptr::NonNull<sys::mln_runtime>) -> Self {
        // SAFETY: ptr came from successful mln_runtime_create and is paired
        // with the matching runtime destroy function.
        let handle = unsafe {
            ThreadAffineNativeHandle::from_raw(ptr, sys::mln_runtime_destroy, "mln_runtime")
        };
        Self {
            handle,
            next_map_id: Cell::new(1),
            has_created_map: Cell::new(false),
            map_ids: RefCell::new(HashMap::new()),
            map_states: RefCell::new(HashMap::new()),
            resource_transform: RefCell::new(None),
            resource_provider: RefCell::new(None),
        }
    }

    pub(crate) fn as_ptr(&self) -> Result<*mut sys::mln_runtime> {
        let ptr = self.handle.as_ptr();
        if ptr.is_null() {
            Err(closed_handle_error("RuntimeHandle"))
        } else {
            Ok(ptr)
        }
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn close(&self) -> Result<()> {
        self.handle.close()?;
        self.resource_transform.borrow_mut().take();
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
        self.check_resource_callbacks_allowed()?;
        let runtime = self.as_ptr()?;
        let replacement = ResourceProviderState::new(callback);
        let descriptor = replacement.descriptor();

        // SAFETY: runtime is live. descriptor contains a C trampoline and a
        // user_data pointer to replacement, which remains alive on success. On
        // failure, native preserves the previous provider and replacement is
        // dropped below.
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_resource_provider(runtime, &descriptor)
        })?;
        self.resource_provider.borrow_mut().replace(replacement);
        Ok(())
    }

    fn set_resource_transform<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        let runtime = self.as_ptr()?;
        let replacement = ResourceTransformState::new(callback);
        let descriptor = replacement.descriptor();

        // SAFETY: runtime is live. descriptor contains a C trampoline and a
        // user_data pointer to replacement, which remains alive on success. On
        // failure, native preserves the previous transform and replacement is
        // dropped below.
        maplibre_core::check(unsafe {
            sys::mln_runtime_set_resource_transform(runtime, &descriptor)
        })?;
        self.resource_transform.borrow_mut().replace(replacement);
        Ok(())
    }

    fn clear_resource_transform(&self) -> Result<()> {
        let runtime = self.as_ptr()?;

        // SAFETY: runtime is live. Native clear waits for in-flight transform
        // callbacks before returning, so dropping Rust callback state below is safe.
        maplibre_core::check(unsafe { sys::mln_runtime_clear_resource_transform(runtime) })?;
        self.resource_transform.borrow_mut().take();
        Ok(())
    }

    fn check_resource_callbacks_allowed(&self) -> Result<()> {
        if self.has_created_map.get() {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "resource callbacks must be configured before creating maps from the runtime",
            ));
        }
        Ok(())
    }

    pub(crate) fn register_map(&self, ptr: *mut sys::mln_map) -> MapId {
        self.has_created_map.set(true);
        let id = MapId::new(self.next_map_id.get());
        self.next_map_id.set(id.get().saturating_add(1));
        self.map_ids.borrow_mut().insert(ptr as usize, id);
        id
    }

    pub(crate) fn register_map_state(&self, ptr: *mut sys::mln_map, state: Weak<MapState>) {
        if !ptr.is_null() {
            self.map_states.borrow_mut().insert(ptr as usize, state);
        }
    }

    pub(crate) fn unregister_map(&self, ptr: *mut sys::mln_map) {
        if !ptr.is_null() {
            self.map_ids.borrow_mut().remove(&(ptr as usize));
            self.map_states.borrow_mut().remove(&(ptr as usize));
        }
    }

    fn apply_event_side_effects(&self, raw: &sys::mln_runtime_event) {
        if raw.source_type != sys::MLN_RUNTIME_EVENT_SOURCE_MAP {
            return;
        }
        let state = self
            .map_states
            .borrow()
            .get(&(raw.source as usize))
            .and_then(Weak::upgrade);
        let Some(state) = state else {
            return;
        };
        if raw.type_ == sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED {
            state.release_detached_custom_geometry_sources();
        }
    }

    #[cfg(test)]
    pub(crate) fn apply_event_side_effects_for_testing(&self, raw: &sys::mln_runtime_event) {
        self.apply_event_side_effects(raw);
    }

    fn source_for_event(&self, raw: &sys::mln_runtime_event) -> RuntimeEventSource {
        match raw.source_type {
            sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME => RuntimeEventSource::Runtime,
            sys::MLN_RUNTIME_EVENT_SOURCE_MAP => self
                .map_ids
                .borrow()
                .get(&(raw.source as usize))
                .copied()
                .map(RuntimeEventSource::Map)
                .unwrap_or(RuntimeEventSource::UnknownMap),
            source_type => RuntimeEventSource::Unknown(source_type),
        }
    }
}

/// Owner-thread runtime handle for MapLibre Native work and event polling.
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

    /// Returns the native operation ID.
    pub fn id(&self) -> u64 {
        self.operation_id
    }

    /// Returns the operation kind expected for this handle.
    pub fn operation_kind(&self) -> maplibre_core::OfflineOperationKind {
        self.operation_kind
    }

    /// Returns the result kind expected for this handle.
    pub fn result_kind(&self) -> maplibre_core::OfflineOperationResultKind {
        self.result_kind
    }

    /// Reports whether this handle still owns runtime operation state.
    pub fn is_live(&self) -> bool {
        self.live.get()
    }

    fn runtime_ptr(&self) -> Result<*mut sys::mln_runtime> {
        if !self.live.get() {
            return Err(closed_handle_error("OfflineOperationHandle"));
        }
        self.runtime.as_ptr()
    }

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
        self.live.set(false);
        Ok(())
    }
}

impl<T> Drop for OfflineOperationHandle<T> {
    fn drop(&mut self) {
        if !self.live.get() {
            return;
        }
        if let Ok(runtime) = self.runtime.as_ptr() {
            // SAFETY: Safe Rust keeps this !Send/!Sync handle on the runtime owner thread.
            let status =
                unsafe { sys::mln_runtime_offline_operation_discard(runtime, self.operation_id) };
            if status == sys::MLN_STATUS_OK {
                self.live.set(false);
            }
        }
    }
}

impl OfflineOperationHandle<OfflineRegionInfo> {
    /// Takes a completed create/update operation result as copied region info.
    pub fn take(self) -> Result<OfflineRegionInfo> {
        let runtime = self.runtime_ptr()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_offline_region_snapshot>::new();
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
        maplibre_core::check(status)?;
        self.mark_consumed();
        // SAFETY: On success, the C API returns an owned snapshot handle;
        // core copies and releases it.
        unsafe {
            maplibre_core::runtime::copy_offline_region_snapshot(
                out.into_non_null("mln_offline_region_snapshot")?,
            )
        }
    }
}

impl OfflineOperationHandle<Option<OfflineRegionInfo>> {
    /// Takes a completed get operation result as optional copied region info.
    pub fn take(self) -> Result<Option<OfflineRegionInfo>> {
        let runtime = self.runtime_ptr()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_offline_region_snapshot>::new();
        let mut found = false;
        let status = unsafe {
            sys::mln_runtime_offline_region_get_take_result(
                runtime,
                self.operation_id,
                out.as_mut_ptr(),
                &mut found,
            )
        };
        maplibre_core::check(status)?;
        self.mark_consumed();
        if !found {
            return Ok(None);
        }
        // SAFETY: When found is true, the C API returns an owned snapshot
        // handle; core copies and releases it.
        Ok(Some(unsafe {
            maplibre_core::runtime::copy_offline_region_snapshot(
                out.into_non_null("mln_offline_region_snapshot")?,
            )
        }?))
    }
}

impl OfflineOperationHandle<Vec<OfflineRegionInfo>> {
    /// Takes a completed list/merge operation result as copied region info.
    pub fn take(self) -> Result<Vec<OfflineRegionInfo>> {
        let runtime = self.runtime_ptr()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_offline_region_list>::new();
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
        maplibre_core::check(status)?;
        self.mark_consumed();
        // SAFETY: On success, the C API returns an owned list handle; core
        // copies and releases it.
        unsafe {
            maplibre_core::runtime::copy_offline_region_list(
                out.into_non_null("mln_offline_region_list")?,
            )
        }
    }
}

impl OfflineOperationHandle<OfflineRegionStatus> {
    /// Takes a completed status operation result as copied status data.
    pub fn take(self) -> Result<OfflineRegionStatus> {
        let runtime = self.runtime_ptr()?;
        let mut raw = maplibre_core::events::empty_offline_region_status_native();
        let status = unsafe {
            sys::mln_runtime_offline_region_get_status_take_result(
                runtime,
                self.operation_id,
                &mut raw,
            )
        };
        maplibre_core::check(status)?;
        self.mark_consumed();
        Ok(maplibre_core::events::offline_region_status_from_native(
            raw,
        ))
    }
}

impl RuntimeHandle {
    /// Creates a runtime on the current thread using native default options.
    pub fn new() -> Result<Self> {
        maplibre_core::validate_abi_version()?;
        Self::create_with_native_options_after_abi_validation(std::ptr::null())
    }

    /// Creates a runtime on the current thread using explicit options.
    pub fn with_options(options: &RuntimeOptions) -> Result<Self> {
        let native_options = options.to_native()?;
        let raw_options = native_options.to_raw();
        Self::create_with_native_options_after_abi_validation(&raw_options)
    }

    fn create_with_native_options_after_abi_validation(
        options: *const sys::mln_runtime_options,
    ) -> Result<Self> {
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_runtime>::new();
        // SAFETY: options is either null to request native defaults or points to
        // a materialized mln_runtime_options value whose backing strings live
        // for this call. out is a valid null-initialized out-pointer owned by
        // this call.
        maplibre_core::check(unsafe { sys::mln_runtime_create(options, out.as_mut_ptr()) })?;
        let ptr = out_handle(out, "mln_runtime")?;

        Ok(Self {
            inner: Rc::new(RuntimeState::new(ptr)),
        })
    }

    /// Creates a map owned by this runtime with native default map options.
    pub fn create_map(&self) -> Result<MapHandle> {
        MapHandle::new(self)
    }

    /// Creates a map owned by this runtime with explicit map options.
    pub fn create_map_with_options(&self, options: &MapOptions) -> Result<MapHandle> {
        MapHandle::with_options(self, options)
    }

    /// Installs or replaces the runtime-scoped network resource provider.
    ///
    /// The provider must be installed before creating maps from this runtime.
    /// Native code may invoke it from worker or network threads, so the closure
    /// must be thread-safe and `'static`. Keep the closure quick, and do not
    /// call map or runtime APIs from it. Return `PassThrough` to let native
    /// networking handle the request. Return `Handle` to complete or release
    /// the provided `ResourceRequestHandle` inline or later. If the callback
    /// completes the handle inline, the wrapper returns native `Handle` even
    /// when the closure returns `PassThrough`, preventing native double
    /// handling.
    pub fn set_resource_provider<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceRequest, crate::ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        self.inner.set_resource_provider(callback)
    }

    /// Installs or replaces the runtime-scoped network URL transform.
    ///
    /// The transform may be installed before or after creating maps from this
    /// runtime. Native code may invoke it from worker or network threads, so
    /// the closure must be thread-safe and `'static`. Keep the closure quick,
    /// and do not call MapLibre Native APIs from it. Returning `Some(url)`
    /// replaces the request URL; returning `None` or an empty string keeps the
    /// original URL. Panics are contained and treated by native code as no
    /// rewrite.
    pub fn set_resource_transform<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        self.inner.set_resource_transform(callback)
    }

    /// Clears the runtime-scoped network URL transform.
    ///
    /// Clearing may happen before or after creating maps from this runtime.
    /// Native clear waits for in-flight transform callbacks before returning,
    /// so this method can release Rust callback state after a successful clear.
    pub fn clear_resource_transform(&self) -> Result<()> {
        self.inner.clear_resource_transform()
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
        let runtime = self.inner.as_ptr()?;
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

    /// Starts creating an offline region.
    pub fn start_create_offline_region(
        &self,
        definition: &OfflineRegionDefinition,
        metadata: &[u8],
    ) -> Result<OfflineOperationHandle<OfflineRegionInfo>> {
        let runtime = self.inner.as_ptr()?;
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
        let runtime = self.inner.as_ptr()?;
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
        let runtime = self.inner.as_ptr()?;
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
        let runtime = self.inner.as_ptr()?;
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
        let runtime = self.inner.as_ptr()?;
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
        let runtime = self.inner.as_ptr()?;
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
        let runtime = self.inner.as_ptr()?;
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
        let runtime = self.inner.as_ptr()?;
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
        let runtime = self.inner.as_ptr()?;
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
        let runtime = self.inner.as_ptr()?;
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

    /// Runs one pending owner-thread task for this runtime.
    pub fn run_once(&self) -> Result<()> {
        let runtime = self.inner.as_ptr()?;
        // SAFETY: runtime is a live runtime handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_runtime_run_once(runtime) })
    }

    /// Polls one queued runtime event and copies it into an owned Rust value.
    pub fn poll_event(&self) -> Result<Option<RuntimeEvent>> {
        let runtime = self.inner.as_ptr()?;
        let mut event = empty_runtime_event();
        let mut has_event = false;

        // SAFETY: runtime is live, event points to initialized writable storage
        // with a valid size field, and has_event points to writable bool storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_poll_event(runtime, &mut event, &mut has_event)
        })?;
        if !has_event {
            return Ok(None);
        }

        let raw_event = event;
        let source = self.inner.source_for_event(&raw_event);
        let event = RuntimeEvent::from_native(&raw_event, source)?;
        self.inner.apply_event_side_effects(&raw_event);
        Ok(Some(event))
    }

    /// Polls and discards one queued runtime event, returning whether one was present.
    pub fn discard_one_event(&self) -> Result<bool> {
        let runtime = self.inner.as_ptr()?;
        let mut event = empty_runtime_event();
        let mut has_event = false;

        // SAFETY: runtime is live, event points to initialized writable storage
        // with a valid size field, and has_event points to writable bool storage.
        // The event is intentionally not decoded because this method only
        // drains native storage.
        maplibre_core::check(unsafe {
            sys::mln_runtime_poll_event(runtime, &mut event, &mut has_event)
        })?;
        if has_event {
            self.inner.apply_event_side_effects(&event);
        }
        Ok(has_event)
    }

    /// Polls and discards queued runtime events until the queue is empty.
    pub fn drain_events(&self) -> Result<usize> {
        let mut count = 0;
        while self.discard_one_event()? {
            count += 1;
        }
        Ok(count)
    }

    /// Explicitly destroys the runtime.
    ///
    /// Native destruction errors are returned. When destruction fails, the
    /// underlying native handle remains live in the shared state so child
    /// handles that retain the runtime can still close safely.
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

#[cfg(test)]
mod tests {
    use std::sync::Arc;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::time::{Duration, SystemTime, UNIX_EPOCH};

    use super::*;
    use crate::{
        ErrorKind, OfflineOperationCompletedEvent, ResourceKind, ResourceProviderDecision,
        ResourceResponse, RuntimeEventPayload, RuntimeEventSource, RuntimeEventType,
    };

    const PROVIDER_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;

    fn wait_for_operation<T>(
        runtime: &RuntimeHandle,
        operation: &OfflineOperationHandle<T>,
    ) -> Result<OfflineOperationCompletedEvent> {
        loop {
            runtime.run_once()?;
            while let Some(event) = runtime.poll_event()? {
                let RuntimeEventPayload::OfflineOperationCompleted(completed) = event.payload
                else {
                    continue;
                };
                if completed.operation_id != operation.id() {
                    continue;
                }
                assert_eq!(completed.operation_kind, operation.operation_kind());
                assert_eq!(
                    completed.raw_operation_kind,
                    operation.operation_kind().raw_value()
                );
                assert_eq!(completed.result_kind, operation.result_kind());
                assert_eq!(
                    completed.raw_result_kind,
                    operation.result_kind().raw_value()
                );
                if completed.result_status != sys::MLN_STATUS_OK {
                    return Err(Error::from_status_and_diagnostic(
                        completed.result_status,
                        event.message.unwrap_or_default(),
                    ));
                }
                return Ok(completed);
            }
            std::thread::sleep(Duration::from_millis(1));
        }
    }

    #[test]
    fn runtime_ambient_cache_operations_use_real_c_abi() {
        let runtime =
            RuntimeHandle::with_options(&RuntimeOptions::new().with_maximum_cache_size(0)).unwrap();

        for operation in [
            AmbientCacheOperation::PackDatabase,
            AmbientCacheOperation::Invalidate,
            AmbientCacheOperation::Clear,
            AmbientCacheOperation::ResetDatabase,
        ] {
            let operation = runtime.start_ambient_cache_operation(operation).unwrap();
            let completed = wait_for_operation(&runtime, &operation).unwrap();
            assert_eq!(completed.operation_id, operation.id());
            operation.discard().unwrap();
        }

        runtime.close().unwrap();
    }

    #[test]
    fn offline_region_apis_use_real_c_abi() {
        let runtime =
            RuntimeHandle::with_options(&RuntimeOptions::new().with_cache_path(":memory:"))
                .unwrap();
        let definition = test_offline_region_definition("custom://offline-style.json");

        let create = runtime
            .start_create_offline_region(&definition, b"abc")
            .unwrap();
        wait_for_operation(&runtime, &create).unwrap();
        let created = create.take().unwrap();
        assert_eq!(created.definition, definition);
        assert_eq!(created.metadata, b"abc");

        let geometry_definition = OfflineRegionDefinition::GeometryRegion {
            style_url: "custom://offline-geometry-style.json".into(),
            geometry: Geometry::Point(crate::LatLng::new(37.5, -122.5)),
            min_zoom: 0.0,
            max_zoom: 1.0,
            pixel_ratio: 1.0,
            include_ideographs: false,
        };
        let create_geometry = runtime
            .start_create_offline_region(&geometry_definition, b"geo")
            .unwrap();
        wait_for_operation(&runtime, &create_geometry).unwrap();
        let geometry_region = create_geometry.take().unwrap();
        assert_eq!(geometry_region.definition, geometry_definition);
        assert_eq!(geometry_region.metadata, b"geo");

        let get = runtime.start_offline_region(created.id).unwrap();
        wait_for_operation(&runtime, &get).unwrap();
        let fetched = get.take().unwrap().unwrap();
        assert_eq!(fetched, created);

        let list = runtime.start_offline_regions().unwrap();
        wait_for_operation(&runtime, &list).unwrap();
        let listed = list.take().unwrap();
        assert!(listed.iter().any(|region| region.id == created.id));

        let update = runtime
            .start_update_offline_region_metadata(created.id, b"")
            .unwrap();
        wait_for_operation(&runtime, &update).unwrap();
        let updated = update.take().unwrap();
        assert_eq!(updated.id, created.id);
        assert!(updated.metadata.is_empty());

        let status_operation = runtime.start_offline_region_status(created.id).unwrap();
        wait_for_operation(&runtime, &status_operation).unwrap();
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
        wait_for_operation(&runtime, &set_inactive).unwrap();
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
        wait_for_operation(&runtime, &observe).unwrap();
        observe.discard().unwrap();
        let unobserve = runtime
            .start_set_offline_region_observed(created.id, false)
            .unwrap();
        wait_for_operation(&runtime, &unobserve).unwrap();
        unobserve.discard().unwrap();
        let invalidate = runtime.start_invalidate_offline_region(created.id).unwrap();
        wait_for_operation(&runtime, &invalidate).unwrap();
        invalidate.discard().unwrap();
        let delete = runtime.start_delete_offline_region(created.id).unwrap();
        wait_for_operation(&runtime, &delete).unwrap();
        delete.discard().unwrap();
        let delete_geometry = runtime
            .start_delete_offline_region(geometry_region.id)
            .unwrap();
        wait_for_operation(&runtime, &delete_geometry).unwrap();
        delete_geometry.discard().unwrap();

        let missing_created = runtime.start_offline_region(created.id).unwrap();
        wait_for_operation(&runtime, &missing_created).unwrap();
        assert!(missing_created.take().unwrap().is_none());
        let missing_geometry = runtime.start_offline_region(geometry_region.id).unwrap();
        wait_for_operation(&runtime, &missing_geometry).unwrap();
        assert!(missing_geometry.take().unwrap().is_none());

        runtime.close().unwrap();
    }

    #[test]
    fn offline_region_merge_database_uses_real_c_abi() {
        let base = unique_temp_dir("maplibre-rust-offline-merge");
        let main_cache = base.join("main.db");
        let side_cache = base.join("side.db");
        std::fs::create_dir_all(&base).unwrap();

        let definition = test_offline_region_definition("custom://merge-style.json");
        {
            let side_runtime = RuntimeHandle::with_options(
                &RuntimeOptions::new().with_cache_path(side_cache.to_string_lossy()),
            )
            .unwrap();
            let create = side_runtime
                .start_create_offline_region(&definition, b"merge")
                .unwrap();
            wait_for_operation(&side_runtime, &create).unwrap();
            create.take().unwrap();
            side_runtime.close().unwrap();
        }

        let main_runtime = RuntimeHandle::with_options(
            &RuntimeOptions::new().with_cache_path(main_cache.to_string_lossy()),
        )
        .unwrap();
        let merge = main_runtime
            .start_merge_offline_regions_database(&side_cache.to_string_lossy())
            .unwrap();
        wait_for_operation(&main_runtime, &merge).unwrap();
        let merged = merge.take().unwrap();
        assert_eq!(merged.len(), 1);
        assert_eq!(merged[0].definition, definition);
        assert_eq!(merged[0].metadata, b"merge");
        main_runtime.close().unwrap();

        std::fs::remove_dir_all(base).unwrap();
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

    fn unique_temp_dir(prefix: &str) -> std::path::PathBuf {
        let nanos = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        std::env::temp_dir().join(format!("{prefix}-{}-{nanos}", std::process::id()))
    }

    #[test]
    fn runtime_create_with_explicit_options_uses_real_c_abi() {
        let runtime = RuntimeHandle::with_options(
            &RuntimeOptions::new()
                .with_asset_path("")
                .with_cache_path("")
                .with_maximum_cache_size(0),
        )
        .unwrap();

        runtime.run_once().unwrap();
        runtime.close().unwrap();
    }

    fn wait_for_runtime_event(runtime: &RuntimeHandle, event_type: RuntimeEventType) -> bool {
        for _ in 0..100 {
            let _ = runtime.run_once();
            while let Ok(Some(event)) = runtime.poll_event() {
                if event.event_type == event_type {
                    return true;
                }
            }
            std::thread::sleep(Duration::from_millis(10));
        }
        false
    }

    #[test]
    fn runtime_create_run_poll_drain_and_close() {
        let runtime = RuntimeHandle::new().unwrap();

        runtime.run_once().unwrap();
        let _ = runtime.poll_event().unwrap();
        let _ = runtime.discard_one_event().unwrap();
        runtime.drain_events().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn resource_provider_installs_replaces_and_releases_state() {
        let runtime = RuntimeHandle::new().unwrap();
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

        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&second), 1);
    }

    #[test]
    fn resource_provider_replacement_rolls_back_when_native_install_fails() {
        let runtime = RuntimeHandle::new().unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);
        runtime
            .set_resource_provider(move |_, _| {
                let _ = &first_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap();
        let map = runtime.create_map().unwrap();

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        let error = runtime
            .set_resource_provider(move |_, _| {
                let _ = &second_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 1);

        map.close().unwrap();
        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&first), 1);
    }

    #[test]
    fn resource_provider_rejects_install_after_map_was_closed() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();
        map.close().unwrap();

        let error = runtime
            .set_resource_provider(|_, _| ResourceProviderDecision::PassThrough)
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(error.raw_status(), None);
        runtime.close().unwrap();
    }

    #[test]
    fn resource_provider_completes_style_request_inline_through_c_abi() {
        let runtime = RuntimeHandle::new().unwrap();
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        runtime
            .set_resource_provider(move |request, handle| {
                if request.url != "custom://style.json" {
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

        let map = runtime.create_map().unwrap();
        map.set_style_url("custom://style.json").unwrap();

        assert!(wait_for_runtime_event(
            &runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(calls.load(Ordering::SeqCst), 1);
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn resource_provider_completes_style_request_from_another_thread() {
        let runtime = RuntimeHandle::new().unwrap();
        let (sender, receiver) = std::sync::mpsc::channel();
        runtime
            .set_resource_provider(move |request, handle| {
                if request.url == "custom://async-style.json" {
                    sender.send(handle).unwrap();
                    ResourceProviderDecision::Handle
                } else {
                    ResourceProviderDecision::PassThrough
                }
            })
            .unwrap();

        let map = runtime.create_map().unwrap();
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
            &runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn resource_transform_installs_replaces_clears_and_releases_state() {
        let runtime = RuntimeHandle::new().unwrap();
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

    #[test]
    fn resource_transform_replacement_after_map_creation_releases_previous_state() {
        let runtime = RuntimeHandle::new().unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);
        runtime
            .set_resource_transform(move |_| {
                let _ = &first_callback;
                None
            })
            .unwrap();
        let map = runtime.create_map().unwrap();

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
    fn runtime_teardown_releases_resource_transform_state() {
        let runtime = RuntimeHandle::new().unwrap();
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
    fn resource_transform_installs_after_map_creation() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();

        runtime.set_resource_transform(|_| None).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn resource_transform_clears_after_map_was_closed_and_releases_state() {
        let runtime = RuntimeHandle::new().unwrap();
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        runtime
            .set_resource_transform(move |_| {
                let _ = &callback_token;
                None
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&token), 2);

        let map = runtime.create_map().unwrap();
        map.close().unwrap();

        runtime.clear_resource_transform().unwrap();

        assert_eq!(Arc::strong_count(&token), 1);

        runtime.close().unwrap();
    }

    #[test]
    fn poll_event_returns_owned_map_event_and_source_id() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();
        let map_id = map.id();

        let error = map.set_style_json("{").unwrap_err();
        assert!(matches!(
            error.kind(),
            ErrorKind::InvalidArgument | ErrorKind::NativeError
        ));

        let mut loading_failed = None;
        for _ in 0..8 {
            let Some(event) = runtime.poll_event().unwrap() else {
                break;
            };
            if event.event_type == RuntimeEventType::MapLoadingFailed {
                loading_failed = Some(event);
                break;
            }
        }
        let event = loading_failed.expect("malformed style should enqueue loading-failed event");
        let copied_message = event.message.clone();

        let _ = runtime.poll_event().unwrap();

        assert_eq!(event.source, RuntimeEventSource::Map(map_id));
        assert_eq!(event.event_type, RuntimeEventType::MapLoadingFailed);
        assert_eq!(event.message, copied_message);
        assert!(
            event
                .message
                .as_deref()
                .is_some_and(|message| !message.is_empty())
        );

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn runtime_close_with_live_map_is_rust_invalid_state_and_retryable() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();

        let error = runtime.close().unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(error.raw_status(), None);
        let runtime = error.into_handle();

        runtime.run_once().unwrap();
        map.close().unwrap();
        runtime.close().unwrap();
    }
}
