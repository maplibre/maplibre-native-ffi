use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::ffi::CString;
use std::fmt;
use std::rc::{Rc, Weak};

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::events::{
    MapId, OfflineRegionDownloadState, OfflineRegionStatus, RuntimeEvent, RuntimeEventSource,
    empty_runtime_event,
};
use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::map::MapState;
use crate::resource::{
    ResourceProviderState, ResourceTransformState, noop_resource_transform_descriptor,
};
use crate::{
    Error, ErrorKind, Geometry, HandleOperationError, LatLngBounds, MapHandle, MapOptions,
    ResourceProviderDecision, Result,
};

/// Options used when creating a runtime.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
#[non_exhaustive]
pub struct RuntimeOptions {
    /// Filesystem root for `asset://` URLs.
    pub asset_path: Option<String>,
    /// Cache database path.
    pub cache_path: Option<String>,
    /// Maximum ambient cache size in bytes.
    pub maximum_cache_size: Option<u64>,
}

impl RuntimeOptions {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_asset_path(mut self, asset_path: impl Into<String>) -> Self {
        self.asset_path = Some(asset_path.into());
        self
    }

    pub fn with_cache_path(mut self, cache_path: impl Into<String>) -> Self {
        self.cache_path = Some(cache_path.into());
        self
    }

    pub fn with_maximum_cache_size(mut self, maximum_cache_size: u64) -> Self {
        self.maximum_cache_size = Some(maximum_cache_size);
        self
    }

    pub fn clear_asset_path(mut self) -> Self {
        self.asset_path = None;
        self
    }

    pub fn clear_cache_path(mut self) -> Self {
        self.cache_path = None;
        self
    }

    pub fn clear_maximum_cache_size(mut self) -> Self {
        self.maximum_cache_size = None;
        self
    }

    fn to_native(&self) -> Result<NativeRuntimeOptions> {
        support::validate_abi_version()?;
        NativeRuntimeOptions::new(self)
    }
}

#[derive(Debug)]
struct NativeRuntimeOptions {
    asset_path: Option<CString>,
    cache_path: Option<CString>,
    maximum_cache_size: Option<u64>,
}

/// Ambient cache maintenance operation for a runtime.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum AmbientCacheOperation {
    /// Reset the ambient cache database.
    ResetDatabase,
    /// Pack the ambient cache database.
    PackDatabase,
    /// Mark ambient cache resources as invalid.
    Invalidate,
    /// Clear ambient cache resources.
    Clear,
}

impl AmbientCacheOperation {
    const fn raw_value(self) -> u32 {
        match self {
            Self::ResetDatabase => sys::MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE,
            Self::PackDatabase => sys::MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE,
            Self::Invalidate => sys::MLN_AMBIENT_CACHE_OPERATION_INVALIDATE,
            Self::Clear => sys::MLN_AMBIENT_CACHE_OPERATION_CLEAR,
        }
    }

    fn to_native(self) -> u32 {
        self.raw_value()
    }
}

/// Region descriptor used to create or inspect offline regions.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum OfflineRegionDefinition {
    TilePyramid {
        style_url: String,
        bounds: LatLngBounds,
        min_zoom: f64,
        max_zoom: f64,
        pixel_ratio: f32,
        include_ideographs: bool,
    },
    GeometryRegion {
        style_url: String,
        geometry: Geometry,
        min_zoom: f64,
        max_zoom: f64,
        pixel_ratio: f32,
        include_ideographs: bool,
    },
}

/// Offline region snapshot copied from native storage.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct OfflineRegionInfo {
    pub id: i64,
    pub definition: OfflineRegionDefinition,
    pub metadata: Vec<u8>,
}

enum NativeOfflineRegionDefinition {
    TilePyramid {
        style_url: CString,
        bounds: LatLngBounds,
        min_zoom: f64,
        max_zoom: f64,
        pixel_ratio: f32,
        include_ideographs: bool,
    },
    GeometryRegion {
        style_url: CString,
        geometry: crate::geometry::NativeGeometry,
        min_zoom: f64,
        max_zoom: f64,
        pixel_ratio: f32,
        include_ideographs: bool,
    },
}

impl NativeOfflineRegionDefinition {
    fn new(definition: &OfflineRegionDefinition) -> Result<Self> {
        match definition {
            OfflineRegionDefinition::TilePyramid {
                style_url,
                bounds,
                min_zoom,
                max_zoom,
                pixel_ratio,
                include_ideographs,
            } => Ok(Self::TilePyramid {
                style_url: support::string::c_string(style_url)?,
                bounds: *bounds,
                min_zoom: *min_zoom,
                max_zoom: *max_zoom,
                pixel_ratio: *pixel_ratio,
                include_ideographs: *include_ideographs,
            }),
            OfflineRegionDefinition::GeometryRegion {
                style_url,
                geometry,
                min_zoom,
                max_zoom,
                pixel_ratio,
                include_ideographs,
            } => Ok(Self::GeometryRegion {
                style_url: support::string::c_string(style_url)?,
                geometry: geometry.try_to_native()?,
                min_zoom: *min_zoom,
                max_zoom: *max_zoom,
                pixel_ratio: *pixel_ratio,
                include_ideographs: *include_ideographs,
            }),
        }
    }

    fn to_raw(&self) -> sys::mln_offline_region_definition {
        match self {
            Self::TilePyramid {
                style_url,
                bounds,
                min_zoom,
                max_zoom,
                pixel_ratio,
                include_ideographs,
            } => sys::mln_offline_region_definition {
                size: std::mem::size_of::<sys::mln_offline_region_definition>() as u32,
                type_: sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
                data: sys::mln_offline_region_definition__bindgen_ty_1 {
                    tile_pyramid: sys::mln_offline_tile_pyramid_region_definition {
                        size: std::mem::size_of::<sys::mln_offline_tile_pyramid_region_definition>()
                            as u32,
                        style_url: style_url.as_ptr(),
                        bounds: bounds.to_native(),
                        min_zoom: *min_zoom,
                        max_zoom: *max_zoom,
                        pixel_ratio: *pixel_ratio,
                        include_ideographs: *include_ideographs,
                    },
                },
            },
            Self::GeometryRegion {
                style_url,
                geometry,
                min_zoom,
                max_zoom,
                pixel_ratio,
                include_ideographs,
            } => sys::mln_offline_region_definition {
                size: std::mem::size_of::<sys::mln_offline_region_definition>() as u32,
                type_: sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY,
                data: sys::mln_offline_region_definition__bindgen_ty_1 {
                    geometry: sys::mln_offline_geometry_region_definition {
                        size: std::mem::size_of::<sys::mln_offline_geometry_region_definition>()
                            as u32,
                        style_url: style_url.as_ptr(),
                        geometry: geometry.as_ptr(),
                        min_zoom: *min_zoom,
                        max_zoom: *max_zoom,
                        pixel_ratio: *pixel_ratio,
                        include_ideographs: *include_ideographs,
                    },
                },
            },
        }
    }
}

fn empty_offline_region_info() -> sys::mln_offline_region_info {
    sys::mln_offline_region_info {
        size: std::mem::size_of::<sys::mln_offline_region_info>() as u32,
        id: 0,
        definition: sys::mln_offline_region_definition {
            size: std::mem::size_of::<sys::mln_offline_region_definition>() as u32,
            type_: 0,
            data: sys::mln_offline_region_definition__bindgen_ty_1 {
                tile_pyramid: sys::mln_offline_tile_pyramid_region_definition {
                    size: std::mem::size_of::<sys::mln_offline_tile_pyramid_region_definition>()
                        as u32,
                    style_url: std::ptr::null(),
                    bounds: crate::map::empty_bounds(),
                    min_zoom: 0.0,
                    max_zoom: 0.0,
                    pixel_ratio: 0.0,
                    include_ideographs: false,
                },
            },
        },
        metadata: std::ptr::null(),
        metadata_size: 0,
    }
}

fn copy_offline_region_info(raw: &sys::mln_offline_region_info) -> Result<OfflineRegionInfo> {
    Ok(OfflineRegionInfo {
        id: raw.id,
        definition: copy_offline_region_definition(&raw.definition)?,
        metadata: copy_metadata(raw.metadata, raw.metadata_size)?,
    })
}

fn copy_offline_region_definition(
    raw: &sys::mln_offline_region_definition,
) -> Result<OfflineRegionDefinition> {
    match raw.type_ {
        sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID => {
            // SAFETY: Active union member is selected by raw.type_.
            let tile = unsafe { raw.data.tile_pyramid };
            Ok(OfflineRegionDefinition::TilePyramid {
                // SAFETY: Native snapshot/list storage owns a NUL-terminated style URL.
                style_url: unsafe { support::string::copy_c_string(tile.style_url) }?,
                bounds: LatLngBounds::from_native(tile.bounds),
                min_zoom: tile.min_zoom,
                max_zoom: tile.max_zoom,
                pixel_ratio: tile.pixel_ratio,
                include_ideographs: tile.include_ideographs,
            })
        }
        sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY => {
            // SAFETY: Active union member is selected by raw.type_.
            let geometry = unsafe { raw.data.geometry };
            if geometry.geometry.is_null() {
                return Err(Error::invalid_argument(
                    "offline region geometry must not be null",
                ));
            }
            Ok(OfflineRegionDefinition::GeometryRegion {
                // SAFETY: Native snapshot/list storage owns a NUL-terminated style URL.
                style_url: unsafe { support::string::copy_c_string(geometry.style_url) }?,
                // SAFETY: geometry.geometry is non-null and borrowed from live snapshot/list storage.
                geometry: unsafe { Geometry::from_native_with_depth(&*geometry.geometry, 0) }?,
                min_zoom: geometry.min_zoom,
                max_zoom: geometry.max_zoom,
                pixel_ratio: geometry.pixel_ratio,
                include_ideographs: geometry.include_ideographs,
            })
        }
        type_ => Err(Error::invalid_argument(format!(
            "unknown offline region definition type: {type_}"
        ))),
    }
}

fn copy_metadata(ptr: *const u8, len: usize) -> Result<Vec<u8>> {
    if len == 0 {
        return Ok(Vec::new());
    }
    if ptr.is_null() {
        return Err(Error::invalid_argument(
            "offline region metadata must not be null when size is nonzero",
        ));
    }
    // SAFETY: Metadata is borrowed from live snapshot/list storage and copied immediately.
    Ok(unsafe { std::slice::from_raw_parts(ptr, len) }.to_vec())
}

fn metadata_ptr(metadata: &[u8]) -> *const u8 {
    if metadata.is_empty() {
        std::ptr::null()
    } else {
        metadata.as_ptr()
    }
}

impl NativeRuntimeOptions {
    fn new(options: &RuntimeOptions) -> Result<Self> {
        Ok(Self {
            asset_path: support::string::optional_c_string(options.asset_path.as_deref())?,
            cache_path: support::string::optional_c_string(options.cache_path.as_deref())?,
            maximum_cache_size: options.maximum_cache_size,
        })
    }

    fn to_raw(&self) -> sys::mln_runtime_options {
        // SAFETY: Default constructor takes no arguments and initializes size,
        // flags, and default values for this C ABI version.
        let mut raw = unsafe { sys::mln_runtime_options_default() };
        if let Some(asset_path) = &self.asset_path {
            raw.asset_path = asset_path.as_ptr();
        }
        if let Some(cache_path) = &self.cache_path {
            raw.cache_path = cache_path.as_ptr();
        }
        if let Some(maximum_cache_size) = self.maximum_cache_size {
            raw.flags |= sys::MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE;
            raw.maximum_cache_size = maximum_cache_size;
        }
        raw
    }
}

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
        support::check(unsafe { sys::mln_runtime_set_resource_provider(runtime, &descriptor) })?;
        self.resource_provider.borrow_mut().replace(replacement);
        Ok(())
    }

    fn set_resource_transform<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        self.check_resource_callbacks_allowed()?;
        let runtime = self.as_ptr()?;
        let replacement = ResourceTransformState::new(callback);
        let descriptor = replacement.descriptor();

        // SAFETY: runtime is live. descriptor contains a C trampoline and a
        // user_data pointer to replacement, which remains alive on success. On
        // failure, native preserves the previous transform and replacement is
        // dropped below.
        support::check(unsafe { sys::mln_runtime_set_resource_transform(runtime, &descriptor) })?;
        self.resource_transform.borrow_mut().replace(replacement);
        Ok(())
    }

    fn clear_resource_transform(&self) -> Result<()> {
        self.check_resource_callbacks_allowed()?;
        let runtime = self.as_ptr()?;
        let descriptor = noop_resource_transform_descriptor();

        // SAFETY: runtime is live. The C ABI has no null clear operation, so a
        // no-op transform with static function state restores pass-through
        // behavior without retaining Rust callback state.
        support::check(unsafe { sys::mln_runtime_set_resource_transform(runtime, &descriptor) })?;
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

fn copy_offline_region_snapshot(
    ptr: std::ptr::NonNull<sys::mln_offline_region_snapshot>,
) -> Result<OfflineRegionInfo> {
    // SAFETY: ptr is an owned snapshot returned by the C API and released by the guard.
    let snapshot = unsafe { support::handle::offline_region_snapshot(ptr.as_ptr()) }?;
    let mut raw = empty_offline_region_info();
    // SAFETY: snapshot is live and raw points to writable storage with size initialized.
    support::check(unsafe { sys::mln_offline_region_snapshot_get(snapshot.as_ptr(), &mut raw) })?;
    copy_offline_region_info(&raw)
}

fn copy_offline_region_list(
    ptr: std::ptr::NonNull<sys::mln_offline_region_list>,
) -> Result<Vec<OfflineRegionInfo>> {
    // SAFETY: ptr is an owned list returned by the C API and released by the guard.
    let list = unsafe { support::handle::offline_region_list(ptr.as_ptr()) }?;
    let mut count = 0;
    // SAFETY: list is live and count points to writable storage.
    support::check(unsafe { sys::mln_offline_region_list_count(list.as_ptr(), &mut count) })?;
    let mut regions = Vec::with_capacity(count);
    for index in 0..count {
        let mut raw = empty_offline_region_info();
        // SAFETY: list is live, index is in range, and raw points to writable storage.
        support::check(unsafe {
            sys::mln_offline_region_list_get(list.as_ptr(), index, &mut raw)
        })?;
        regions.push(copy_offline_region_info(&raw)?);
    }
    Ok(regions)
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

impl RuntimeHandle {
    /// Creates a runtime on the current thread using native default options.
    pub fn new() -> Result<Self> {
        support::validate_abi_version()?;
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
        let mut out = support::ptr::OutPtr::<sys::mln_runtime>::new();
        // SAFETY: options is either null to request native defaults or points to
        // a materialized mln_runtime_options value whose backing strings live
        // for this call. out is a valid null-initialized out-pointer owned by
        // this call.
        support::check(unsafe { sys::mln_runtime_create(options, out.as_mut_ptr()) })?;
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
    /// The transform must be installed before creating maps from this runtime.
    /// Native code may invoke it from worker or network threads, so the closure
    /// must be thread-safe and `'static`. Keep the closure quick, and do not
    /// call MapLibre Native APIs from it. Returning `Some(url)` replaces the
    /// request URL; returning `None` or an empty string keeps the original URL.
    /// Panics are contained and treated by native code as no rewrite.
    pub fn set_resource_transform<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        self.inner.set_resource_transform(callback)
    }

    /// Clears the runtime-scoped network URL transform.
    ///
    /// Like installation, clearing must happen before creating maps from this
    /// runtime. The current C ABI has no null clear operation. This method
    /// installs a native no-op transform before releasing Rust callback state,
    /// restoring pass-through URL behavior while honoring native install
    /// constraints.
    pub fn clear_resource_transform(&self) -> Result<()> {
        self.inner.clear_resource_transform()
    }

    /// Runs an ambient cache maintenance operation for this runtime.
    pub fn run_ambient_cache_operation(&self, operation: AmbientCacheOperation) -> Result<()> {
        let runtime = self.inner.as_ptr()?;
        // SAFETY: runtime is a live runtime handle owned by this wrapper, and
        // operation is materialized from the closed Rust enum domain.
        support::check(unsafe {
            sys::mln_runtime_run_ambient_cache_operation(runtime, operation.to_native())
        })
    }

    /// Creates an offline region and returns its copied native snapshot.
    pub fn create_offline_region(
        &self,
        definition: &OfflineRegionDefinition,
        metadata: &[u8],
    ) -> Result<OfflineRegionInfo> {
        let runtime = self.inner.as_ptr()?;
        let definition = NativeOfflineRegionDefinition::new(definition)?;
        let mut out = support::ptr::OutPtr::<sys::mln_offline_region_snapshot>::new();
        let raw_definition = definition.to_raw();
        // SAFETY: runtime is live. raw_definition points into definition-owned
        // string and geometry storage, metadata storage is valid for this call,
        // and out is a null-initialized out-pointer.
        support::check(unsafe {
            sys::mln_runtime_offline_region_create(
                runtime,
                &raw_definition,
                metadata_ptr(metadata),
                metadata.len(),
                out.as_mut_ptr(),
            )
        })?;
        copy_offline_region_snapshot(out.into_non_null("mln_offline_region_snapshot")?)
    }

    /// Gets an offline region snapshot by ID.
    pub fn offline_region(&self, region_id: i64) -> Result<Option<OfflineRegionInfo>> {
        let runtime = self.inner.as_ptr()?;
        let mut out = support::ptr::OutPtr::<sys::mln_offline_region_snapshot>::new();
        let mut found = false;
        // SAFETY: runtime is live, out is a null-initialized out-pointer, and
        // found points to writable bool storage.
        support::check(unsafe {
            sys::mln_runtime_offline_region_get(runtime, region_id, out.as_mut_ptr(), &mut found)
        })?;
        if found {
            Ok(Some(copy_offline_region_snapshot(
                out.into_non_null("mln_offline_region_snapshot")?,
            )?))
        } else {
            Ok(None)
        }
    }

    /// Lists offline regions in this runtime's database.
    pub fn offline_regions(&self) -> Result<Vec<OfflineRegionInfo>> {
        let runtime = self.inner.as_ptr()?;
        let mut out = support::ptr::OutPtr::<sys::mln_offline_region_list>::new();
        // SAFETY: runtime is live and out is a null-initialized out-pointer.
        support::check(unsafe {
            sys::mln_runtime_offline_regions_list(runtime, out.as_mut_ptr())
        })?;
        copy_offline_region_list(out.into_non_null("mln_offline_region_list")?)
    }

    /// Merges offline regions from another database path.
    pub fn merge_offline_regions_database(&self, path: &str) -> Result<Vec<OfflineRegionInfo>> {
        let runtime = self.inner.as_ptr()?;
        let path = support::string::c_string(path)?;
        let mut out = support::ptr::OutPtr::<sys::mln_offline_region_list>::new();
        // SAFETY: runtime is live, path is a NUL-terminated string valid for
        // this call, and out is a null-initialized out-pointer.
        support::check(unsafe {
            sys::mln_runtime_offline_regions_merge_database(
                runtime,
                path.as_ptr(),
                out.as_mut_ptr(),
            )
        })?;
        copy_offline_region_list(out.into_non_null("mln_offline_region_list")?)
    }

    /// Updates opaque metadata for an offline region.
    pub fn update_offline_region_metadata(
        &self,
        region_id: i64,
        metadata: &[u8],
    ) -> Result<OfflineRegionInfo> {
        let runtime = self.inner.as_ptr()?;
        let mut out = support::ptr::OutPtr::<sys::mln_offline_region_snapshot>::new();
        // SAFETY: runtime is live, metadata storage is valid for this call, and
        // out is a null-initialized out-pointer.
        support::check(unsafe {
            sys::mln_runtime_offline_region_update_metadata(
                runtime,
                region_id,
                metadata_ptr(metadata),
                metadata.len(),
                out.as_mut_ptr(),
            )
        })?;
        copy_offline_region_snapshot(out.into_non_null("mln_offline_region_snapshot")?)
    }

    /// Gets the current completed/download status for an offline region.
    pub fn offline_region_status(&self, region_id: i64) -> Result<OfflineRegionStatus> {
        let runtime = self.inner.as_ptr()?;
        let mut raw = sys::mln_offline_region_status {
            size: std::mem::size_of::<sys::mln_offline_region_status>() as u32,
            download_state: sys::MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE,
            completed_resource_count: 0,
            completed_resource_size: 0,
            completed_tile_count: 0,
            required_tile_count: 0,
            completed_tile_size: 0,
            required_resource_count: 0,
            required_resource_count_is_precise: false,
            complete: false,
        };
        // SAFETY: runtime is live and raw points to initialized writable storage.
        support::check(unsafe {
            sys::mln_runtime_offline_region_get_status(runtime, region_id, &mut raw)
        })?;
        Ok(OfflineRegionStatus::from_native(raw))
    }

    /// Enables or disables runtime events for an offline region.
    pub fn set_offline_region_observed(&self, region_id: i64, observed: bool) -> Result<()> {
        let runtime = self.inner.as_ptr()?;
        // SAFETY: runtime is live and values are passed by value.
        support::check(unsafe {
            sys::mln_runtime_offline_region_set_observed(runtime, region_id, observed)
        })
    }

    /// Sets an offline region's native download state.
    pub fn set_offline_region_download_state(
        &self,
        region_id: i64,
        state: OfflineRegionDownloadState,
    ) -> Result<()> {
        let runtime = self.inner.as_ptr()?;
        let state = match state {
            OfflineRegionDownloadState::Inactive => sys::MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE,
            OfflineRegionDownloadState::Active => sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
            OfflineRegionDownloadState::Unknown(raw) => {
                return Err(Error::invalid_argument(format!(
                    "unknown offline region download state cannot be set: {raw}"
                )));
            }
        };
        // SAFETY: runtime is live, region_id is passed by value, and state is a
        // closed Rust enum domain mapped to the C ABI value.
        support::check(unsafe {
            sys::mln_runtime_offline_region_set_download_state(runtime, region_id, state)
        })
    }

    /// Invalidates cached resources for an offline region.
    pub fn invalidate_offline_region(&self, region_id: i64) -> Result<()> {
        let runtime = self.inner.as_ptr()?;
        // SAFETY: runtime is live and region_id is passed by value.
        support::check(unsafe { sys::mln_runtime_offline_region_invalidate(runtime, region_id) })
    }

    /// Deletes an offline region.
    pub fn delete_offline_region(&self, region_id: i64) -> Result<()> {
        let runtime = self.inner.as_ptr()?;
        // SAFETY: runtime is live and region_id is passed by value.
        support::check(unsafe { sys::mln_runtime_offline_region_delete(runtime, region_id) })
    }

    /// Runs one pending owner-thread task for this runtime.
    pub fn run_once(&self) -> Result<()> {
        let runtime = self.inner.as_ptr()?;
        // SAFETY: runtime is a live runtime handle owned by this wrapper.
        support::check(unsafe { sys::mln_runtime_run_once(runtime) })
    }

    /// Polls one queued runtime event and copies it into an owned Rust value.
    pub fn poll_event(&self) -> Result<Option<RuntimeEvent>> {
        let runtime = self.inner.as_ptr()?;
        let mut event = empty_runtime_event();
        let mut has_event = false;

        // SAFETY: runtime is live, event points to initialized writable storage
        // with a valid size field, and has_event points to writable bool storage.
        support::check(unsafe {
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
        support::check(unsafe {
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
        ErrorKind, ResourceKind, ResourceProviderDecision, ResourceResponse, RuntimeEventSource,
        RuntimeEventType,
    };

    const PROVIDER_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;

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
            runtime.run_ambient_cache_operation(operation).unwrap();
        }

        runtime.close().unwrap();
    }

    #[test]
    fn offline_region_apis_use_real_c_abi() {
        let runtime =
            RuntimeHandle::with_options(&RuntimeOptions::new().with_cache_path(":memory:"))
                .unwrap();
        let definition = test_offline_region_definition("custom://offline-style.json");

        let created = runtime.create_offline_region(&definition, b"abc").unwrap();
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
        let geometry_region = runtime
            .create_offline_region(&geometry_definition, b"geo")
            .unwrap();
        assert_eq!(geometry_region.definition, geometry_definition);
        assert_eq!(geometry_region.metadata, b"geo");

        let fetched = runtime.offline_region(created.id).unwrap().unwrap();
        assert_eq!(fetched, created);

        let listed = runtime.offline_regions().unwrap();
        assert!(listed.iter().any(|region| region.id == created.id));

        let updated = runtime
            .update_offline_region_metadata(created.id, b"")
            .unwrap();
        assert_eq!(updated.id, created.id);
        assert!(updated.metadata.is_empty());

        let status = runtime.offline_region_status(created.id).unwrap();
        assert!(matches!(
            status.download_state,
            OfflineRegionDownloadState::Inactive | OfflineRegionDownloadState::Active
        ));

        runtime
            .set_offline_region_download_state(created.id, OfflineRegionDownloadState::Inactive)
            .unwrap();
        let error = runtime
            .set_offline_region_download_state(created.id, OfflineRegionDownloadState::Unknown(99))
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);

        runtime
            .set_offline_region_observed(created.id, true)
            .unwrap();
        runtime
            .set_offline_region_observed(created.id, false)
            .unwrap();
        runtime.invalidate_offline_region(created.id).unwrap();
        runtime.delete_offline_region(created.id).unwrap();
        runtime.delete_offline_region(geometry_region.id).unwrap();
        assert!(runtime.offline_region(created.id).unwrap().is_none());
        assert!(
            runtime
                .offline_region(geometry_region.id)
                .unwrap()
                .is_none()
        );

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
            side_runtime
                .create_offline_region(&definition, b"merge")
                .unwrap();
            side_runtime.close().unwrap();
        }

        let main_runtime = RuntimeHandle::with_options(
            &RuntimeOptions::new().with_cache_path(main_cache.to_string_lossy()),
        )
        .unwrap();
        let merged = main_runtime
            .merge_offline_regions_database(&side_cache.to_string_lossy())
            .unwrap();
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
    fn resource_transform_replacement_rolls_back_when_native_install_fails() {
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
        let error = runtime
            .set_resource_transform(move |_| {
                let _ = &second_callback;
                None
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
    fn resource_transform_rejects_install_after_map_creation() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();

        let error = runtime.set_resource_transform(|_| None).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn resource_transform_rejects_clear_after_map_was_closed_and_keeps_state_until_close() {
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

        let error = runtime.clear_resource_transform().unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(error.raw_status(), None);
        assert_eq!(Arc::strong_count(&token), 2);

        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&token), 1);
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
