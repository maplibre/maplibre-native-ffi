use std::collections::{HashMap, HashSet};
use std::ffi::{CStr, CString, c_void};
use std::os::raw::c_char;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr::NonNull;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex, OnceLock, Weak};

use maplibre_native_core::{self as core, handle::NativeHandleState};
use maplibre_native_sys as sys;
use napi::bindgen_prelude::{BigInt, Result, Uint8Array};
use napi::threadsafe_function::{ThreadsafeFunction, ThreadsafeFunctionCallMode};
use napi_derive::napi;

use crate::error;

#[napi(object)]
#[derive(Default)]
pub struct RuntimeOptions {
    pub asset_path: Option<String>,
    pub cache_path: Option<String>,
    pub maximum_cache_size: Option<BigInt>,
}

#[napi(object)]
pub struct OfflineOperationStart {
    pub operation_id: BigInt,
}

#[napi(object)]
pub struct OfflineRegionDefinitionInput {
    pub kind: String,
    pub style_url: String,
    pub bounds: Option<crate::values::LatLngBounds>,
    pub geometry: Option<String>,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub pixel_ratio: f64,
    pub include_ideographs: Option<bool>,
}

#[napi(object)]
pub struct OfflineRegionDefinitionValue {
    pub kind: String,
    pub style_url: String,
    pub bounds: Option<crate::values::LatLngBounds>,
    pub geometry: Option<serde_json::Value>,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub pixel_ratio: f64,
    pub include_ideographs: bool,
}

#[napi(object)]
pub struct OfflineRegionInfoValue {
    pub id: BigInt,
    pub definition: OfflineRegionDefinitionValue,
    pub metadata: Uint8Array,
}

#[napi(object)]
pub struct OfflineRegionStatusValue {
    pub download_state: String,
    pub raw_download_state: u32,
    pub completed_resource_count: BigInt,
    pub completed_resource_size: BigInt,
    pub completed_tile_count: BigInt,
    pub required_tile_count: BigInt,
    pub completed_tile_size: BigInt,
    pub required_resource_count: BigInt,
    pub required_resource_count_is_precise: bool,
    pub complete: bool,
}

#[napi(object)]
pub struct RuntimeEvent {
    pub event_type: String,
    pub raw_event_type: u32,
    pub source_type: String,
    pub raw_source_type: u32,
    pub source_address: BigInt,
    pub code: i32,
    pub message: Option<String>,
    pub payload_kind: String,
    pub payload: RuntimeEventPayloadValue,
}

#[napi(object)]
pub struct RuntimeEventPayloadValue {
    pub kind: String,
    pub raw_type: u32,
    pub render_frame: Option<RenderFrameEventValue>,
    pub render_map: Option<RenderMapEventValue>,
    pub style_image_missing: Option<StyleImageMissingEventValue>,
    pub tile_action: Option<TileActionEventValue>,
    pub offline_region_status: Option<OfflineRegionStatusEventValue>,
    pub offline_region_response_error: Option<OfflineRegionResponseErrorEventValue>,
    pub offline_region_tile_count_limit: Option<OfflineRegionTileCountLimitEventValue>,
    pub offline_operation_completed: Option<OfflineOperationCompletedEventValue>,
    pub unknown: Option<UnknownRuntimeEventPayloadValue>,
}

#[napi(object)]
pub struct RenderingStatsValue {
    pub encoding_time: f64,
    pub rendering_time: f64,
    pub frame_count: BigInt,
    pub draw_call_count: BigInt,
    pub total_draw_call_count: BigInt,
}

#[napi(object)]
pub struct RenderFrameEventValue {
    pub mode: String,
    pub raw_mode: u32,
    pub needs_repaint: bool,
    pub placement_changed: bool,
    pub stats: RenderingStatsValue,
}

#[napi(object)]
pub struct RenderMapEventValue {
    pub mode: String,
    pub raw_mode: u32,
}

#[napi(object)]
pub struct StyleImageMissingEventValue {
    pub image_id: String,
}

#[napi(object)]
pub struct TileIdValue {
    pub overscaled_z: u32,
    pub wrap: i32,
    pub canonical_z: u32,
    pub canonical_x: u32,
    pub canonical_y: u32,
}

#[napi(object)]
pub struct TileActionEventValue {
    pub operation: String,
    pub raw_operation: u32,
    pub tile_id: TileIdValue,
    pub source_id: String,
}

#[napi(object)]
pub struct OfflineRegionStatusEventValue {
    pub region_id: BigInt,
    pub status: OfflineRegionStatusValue,
}

#[napi(object)]
pub struct OfflineRegionResponseErrorEventValue {
    pub region_id: BigInt,
    pub reason: String,
    pub raw_reason: u32,
}

#[napi(object)]
pub struct OfflineRegionTileCountLimitEventValue {
    pub region_id: BigInt,
    pub limit: BigInt,
}

#[napi(object)]
pub struct OfflineOperationCompletedEventValue {
    pub operation_id: BigInt,
    pub operation_kind: String,
    pub raw_operation_kind: u32,
    pub result_kind: String,
    pub raw_result_kind: u32,
    pub result_status: i32,
    pub found: bool,
}

#[napi(object)]
pub struct UnknownRuntimeEventPayloadValue {
    pub raw_type: u32,
    pub bytes: Uint8Array,
}

#[napi(object)]
pub struct ResourceRouteInput {
    pub kind: Option<String>,
    pub url: Option<String>,
    pub url_prefix: Option<String>,
}

#[napi(object)]
pub struct ResourceTransformRuleInput {
    pub kind: Option<String>,
    pub url: Option<String>,
    pub url_prefix: Option<String>,
    pub replacement_url: Option<String>,
    pub replacement_url_prefix: Option<String>,
}

#[napi(object)]
pub struct ResourceByteRange {
    pub start: BigInt,
    pub end: BigInt,
}

#[napi(object)]
pub struct ResourceProviderRequest {
    pub url: String,
    pub kind: String,
    pub raw_kind: u32,
    pub loading_method: String,
    pub raw_loading_method: u32,
    pub priority: String,
    pub raw_priority: u32,
    pub usage: String,
    pub raw_usage: u32,
    pub storage_policy: String,
    pub raw_storage_policy: u32,
    pub range: Option<ResourceByteRange>,
    pub prior_modified_unix_ms: Option<BigInt>,
    pub prior_expires_unix_ms: Option<BigInt>,
    pub prior_etag: Option<String>,
    pub prior_data: Uint8Array,
    pub completion_token: String,
}

#[napi(object)]
pub struct ResourceResponseInput {
    pub status: Option<String>,
    pub error_reason: Option<String>,
    pub bytes: Option<Uint8Array>,
    pub error_message: Option<String>,
    pub must_revalidate: Option<bool>,
    pub modified_unix_ms: Option<BigInt>,
    pub expires_unix_ms: Option<BigInt>,
    pub etag: Option<String>,
    pub retry_after_unix_ms: Option<BigInt>,
}

static RESOURCE_REQUEST_TOKEN_IDS: AtomicU64 = AtomicU64::new(1);
static RESOURCE_REQUEST_HANDLES: OnceLock<Mutex<HashMap<String, ResourceRequestRegistration>>> =
    OnceLock::new();

#[derive(Clone)]
struct ResourceRequestRegistration {
    handle: Arc<core::resource::ResourceRequestHandleState>,
    provider: Weak<ResourceProviderState>,
}

struct ResourceMatcher {
    kind: Option<u32>,
    url: Option<String>,
    url_prefix: Option<String>,
}

struct ResourceTransformRule {
    matcher: ResourceMatcher,
    replacement_url: Option<CString>,
    replacement_url_prefix: Option<String>,
}

struct ResourceTransformState {
    rules: Vec<ResourceTransformRule>,
}

struct ResourceProviderState {
    routes: Vec<ResourceMatcher>,
    callback: ThreadsafeFunction<ResourceProviderRequest>,
    pending_completion_tokens: Mutex<HashSet<String>>,
}

#[napi(js_name = "NativeRuntimeHandle")]
pub struct NativeRuntimeHandle {
    state: NativeHandleState<sys::mln_runtime>,
    has_created_map: AtomicBool,
    resource_transform: Mutex<Option<Arc<ResourceTransformState>>>,
    resource_provider: Mutex<Option<Arc<ResourceProviderState>>>,
}

#[napi(js_name = "createNativeRuntimeHandle")]
pub fn create_native_runtime_handle(
    options: Option<RuntimeOptions>,
) -> Result<NativeRuntimeHandle> {
    let options = options.unwrap_or_default().into_core()?;
    let native_options =
        core::runtime::runtime_options_to_native(&options).map_err(error::from_core)?;
    let mut runtime = std::ptr::null_mut();

    core::check(unsafe { sys::mln_runtime_create(&native_options.to_raw(), &mut runtime) })
        .map_err(error::from_core)?;
    let state = unsafe { NativeHandleState::from_raw_ptr(runtime, "RuntimeHandle") }
        .map_err(error::from_core)?;
    Ok(NativeRuntimeHandle {
        state,
        has_created_map: AtomicBool::new(false),
        resource_transform: Mutex::new(None),
        resource_provider: Mutex::new(None),
    })
}

#[napi(js_name = "nativeResourceRequestComplete")]
pub fn native_resource_request_complete(
    completion_token: String,
    response: ResourceResponseInput,
) -> Result<()> {
    validate_resource_request_completion_token(&completion_token)?;
    let response = resource_response_from_input(response)?;
    let registration = resource_request_handles()
        .lock()
        .map_err(|_| error::invalid_state("resource request registry lock is poisoned"))?
        .get(&completion_token)
        .cloned()
        .ok_or_else(|| error::invalid_state("ResourceRequestHandle is closed"))?;
    unregister_resource_request_handle(&completion_token);
    registration
        .handle
        .complete(&response)
        .map_err(error::from_core)
}

#[napi(js_name = "nativeResourceRequestCancelled")]
pub fn native_resource_request_cancelled(completion_token: String) -> Result<bool> {
    validate_resource_request_completion_token(&completion_token)?;
    let registration = resource_request_handles()
        .lock()
        .map_err(|_| error::invalid_state("resource request registry lock is poisoned"))?
        .get(&completion_token)
        .cloned()
        .ok_or_else(|| error::invalid_state("ResourceRequestHandle is closed"))?;
    registration.handle.is_cancelled().map_err(error::from_core)
}

#[napi(js_name = "nativeResourceRequestClose")]
pub fn native_resource_request_close(completion_token: String) -> Result<()> {
    validate_resource_request_completion_token(&completion_token)?;
    if let Some(registration) = unregister_resource_request_handle(&completion_token) {
        registration.handle.close();
    }
    Ok(())
}

#[napi]
impl NativeRuntimeHandle {
    #[napi]
    pub fn close(&self) -> Result<()> {
        unsafe { self.state.close_status(sys::mln_runtime_destroy) }.map_err(error::from_core)?;
        self.release_resource_callback_state();
        Ok(())
    }

    #[napi(getter)]
    pub fn closed(&self) -> bool {
        self.state.is_closed()
    }

    #[napi(js_name = "runOnce")]
    pub fn run_once(&self) -> Result<()> {
        core::check(unsafe { sys::mln_runtime_run_once(self.state.as_ptr()) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "setResourceProviderRoutes")]
    pub fn set_resource_provider_routes(
        &self,
        routes: Vec<ResourceRouteInput>,
        callback: ThreadsafeFunction<ResourceProviderRequest>,
    ) -> Result<()> {
        if self.has_created_map.load(Ordering::Acquire) {
            return Err(error::invalid_state(
                "resource provider routes must be configured before creating maps from the runtime",
            ));
        }
        let provider = Arc::new(ResourceProviderState {
            routes: routes
                .into_iter()
                .map(resource_matcher_from_input)
                .collect::<Result<Vec<_>>>()?,
            callback,
            pending_completion_tokens: Mutex::new(HashSet::new()),
        });
        let descriptor = core::resource::resource_provider_descriptor(
            Some(resource_provider_trampoline),
            Arc::as_ptr(&provider) as *mut c_void,
        );
        let mut provider_slot = self
            .resource_provider
            .lock()
            .map_err(|_| error::invalid_argument("resource provider state lock is poisoned"))?;
        core::check(unsafe {
            sys::mln_runtime_set_resource_provider(self.state.as_ptr(), &descriptor)
        })
        .map_err(error::from_core)?;
        let replaced = provider_slot.replace(provider);
        drop(replaced);
        Ok(())
    }

    #[napi(js_name = "setResourceTransformRules")]
    pub fn set_resource_transform_rules(
        &self,
        rules: Vec<ResourceTransformRuleInput>,
    ) -> Result<()> {
        let transform = Arc::new(ResourceTransformState {
            rules: rules
                .into_iter()
                .map(resource_transform_rule_from_input)
                .collect::<Result<Vec<_>>>()?,
        });
        let descriptor = core::resource::resource_transform_descriptor(
            Some(resource_transform_trampoline),
            Arc::as_ptr(&transform) as *mut c_void,
        );
        let mut transform_slot = self
            .resource_transform
            .lock()
            .map_err(|_| error::invalid_argument("resource transform state lock is poisoned"))?;
        core::check(unsafe {
            sys::mln_runtime_set_resource_transform(self.state.as_ptr(), &descriptor)
        })
        .map_err(error::from_core)?;
        drop(transform_slot.replace(transform));
        Ok(())
    }

    #[napi(js_name = "clearResourceTransform")]
    pub fn clear_resource_transform(&self) -> Result<()> {
        core::check(unsafe { sys::mln_runtime_clear_resource_transform(self.state.as_ptr()) })
            .map_err(error::from_core)?;
        self.resource_transform
            .lock()
            .map_err(|_| error::invalid_argument("resource transform state lock is poisoned"))?
            .take();
        Ok(())
    }

    #[napi(js_name = "runAmbientCacheOperation")]
    pub fn run_ambient_cache_operation(&self, operation: String) -> Result<OfflineOperationStart> {
        let operation = ambient_cache_operation_from_string(&operation)?;
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_run_ambient_cache_operation_start(
                self.state.as_ptr(),
                operation,
                &mut operation_id,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionsList")]
    pub fn offline_regions_list(&self) -> Result<OfflineOperationStart> {
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_offline_regions_list_start(self.state.as_ptr(), &mut operation_id)
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionGet")]
    pub fn offline_region_get(&self, region_id: BigInt) -> Result<OfflineOperationStart> {
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_offline_region_get_start(
                self.state.as_ptr(),
                bigint_to_i64(region_id, "regionId")?,
                &mut operation_id,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionsMergeDatabase")]
    pub fn offline_regions_merge_database(&self, path: String) -> Result<OfflineOperationStart> {
        let path = core::string::c_string(&path).map_err(error::from_core)?;
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_offline_regions_merge_database_start(
                self.state.as_ptr(),
                path.as_ptr(),
                &mut operation_id,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionUpdateMetadata")]
    pub fn offline_region_update_metadata(
        &self,
        region_id: BigInt,
        metadata: Option<Uint8Array>,
    ) -> Result<OfflineOperationStart> {
        let metadata = metadata
            .map(|metadata| metadata.to_vec())
            .unwrap_or_default();
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_offline_region_update_metadata_start(
                self.state.as_ptr(),
                bigint_to_i64(region_id, "regionId")?,
                core::runtime::metadata_ptr(&metadata),
                metadata.len(),
                &mut operation_id,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionGetStatus")]
    pub fn offline_region_get_status(&self, region_id: BigInt) -> Result<OfflineOperationStart> {
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_offline_region_get_status_start(
                self.state.as_ptr(),
                bigint_to_i64(region_id, "regionId")?,
                &mut operation_id,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionSetObserved")]
    pub fn offline_region_set_observed(
        &self,
        region_id: BigInt,
        observed: bool,
    ) -> Result<OfflineOperationStart> {
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_offline_region_set_observed_start(
                self.state.as_ptr(),
                bigint_to_i64(region_id, "regionId")?,
                observed,
                &mut operation_id,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionSetDownloadState")]
    pub fn offline_region_set_download_state(
        &self,
        region_id: BigInt,
        state: String,
    ) -> Result<OfflineOperationStart> {
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_offline_region_set_download_state_start(
                self.state.as_ptr(),
                bigint_to_i64(region_id, "regionId")?,
                offline_region_download_state_from_string(&state)?,
                &mut operation_id,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionInvalidate")]
    pub fn offline_region_invalidate(&self, region_id: BigInt) -> Result<OfflineOperationStart> {
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_offline_region_invalidate_start(
                self.state.as_ptr(),
                bigint_to_i64(region_id, "regionId")?,
                &mut operation_id,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionDelete")]
    pub fn offline_region_delete(&self, region_id: BigInt) -> Result<OfflineOperationStart> {
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_offline_region_delete_start(
                self.state.as_ptr(),
                bigint_to_i64(region_id, "regionId")?,
                &mut operation_id,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionCreate")]
    pub fn offline_region_create(
        &self,
        definition: OfflineRegionDefinitionInput,
        metadata: Option<Uint8Array>,
    ) -> Result<OfflineOperationStart> {
        let definition = offline_region_definition_from_input(definition)?;
        let native_definition = core::runtime::offline_region_definition_to_native(&definition)
            .map_err(error::from_core)?;
        let raw_definition = native_definition.to_raw();
        let metadata = metadata
            .map(|metadata| metadata.to_vec())
            .unwrap_or_default();
        let mut operation_id = 0;
        core::check(unsafe {
            sys::mln_runtime_offline_region_create_start(
                self.state.as_ptr(),
                &raw_definition,
                core::runtime::metadata_ptr(&metadata),
                metadata.len(),
                &mut operation_id,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_operation_start(operation_id))
    }

    #[napi(js_name = "offlineRegionCreateTakeResult")]
    pub fn offline_region_create_take_result(
        &self,
        operation_id: BigInt,
    ) -> Result<OfflineRegionInfoValue> {
        let mut snapshot = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_runtime_offline_region_create_take_result(
                self.state.as_ptr(),
                bigint_to_u64(operation_id, "operationId")?,
                &mut snapshot,
            )
        })
        .map_err(error::from_core)?;
        copy_offline_region_snapshot_value(snapshot)
    }

    #[napi(js_name = "offlineRegionGetTakeResult")]
    pub fn offline_region_get_take_result(
        &self,
        operation_id: BigInt,
    ) -> Result<Option<OfflineRegionInfoValue>> {
        let mut snapshot = std::ptr::null_mut();
        let mut found = false;
        core::check(unsafe {
            sys::mln_runtime_offline_region_get_take_result(
                self.state.as_ptr(),
                bigint_to_u64(operation_id, "operationId")?,
                &mut snapshot,
                &mut found,
            )
        })
        .map_err(error::from_core)?;
        if !found {
            return Ok(None);
        }
        Ok(Some(copy_offline_region_snapshot_value(snapshot)?))
    }

    #[napi(js_name = "offlineRegionsListTakeResult")]
    pub fn offline_regions_list_take_result(
        &self,
        operation_id: BigInt,
    ) -> Result<Vec<OfflineRegionInfoValue>> {
        let mut list = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_runtime_offline_regions_list_take_result(
                self.state.as_ptr(),
                bigint_to_u64(operation_id, "operationId")?,
                &mut list,
            )
        })
        .map_err(error::from_core)?;
        copy_offline_region_list_value(list)
    }

    #[napi(js_name = "offlineRegionsMergeDatabaseTakeResult")]
    pub fn offline_regions_merge_database_take_result(
        &self,
        operation_id: BigInt,
    ) -> Result<Vec<OfflineRegionInfoValue>> {
        let mut list = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_runtime_offline_regions_merge_database_take_result(
                self.state.as_ptr(),
                bigint_to_u64(operation_id, "operationId")?,
                &mut list,
            )
        })
        .map_err(error::from_core)?;
        copy_offline_region_list_value(list)
    }

    #[napi(js_name = "offlineRegionUpdateMetadataTakeResult")]
    pub fn offline_region_update_metadata_take_result(
        &self,
        operation_id: BigInt,
    ) -> Result<OfflineRegionInfoValue> {
        let mut snapshot = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_runtime_offline_region_update_metadata_take_result(
                self.state.as_ptr(),
                bigint_to_u64(operation_id, "operationId")?,
                &mut snapshot,
            )
        })
        .map_err(error::from_core)?;
        copy_offline_region_snapshot_value(snapshot)
    }

    #[napi(js_name = "offlineRegionGetStatusTakeResult")]
    pub fn offline_region_get_status_take_result(
        &self,
        operation_id: BigInt,
    ) -> Result<OfflineRegionStatusValue> {
        let mut raw_status = core::events::empty_offline_region_status_native();
        core::check(unsafe {
            sys::mln_runtime_offline_region_get_status_take_result(
                self.state.as_ptr(),
                bigint_to_u64(operation_id, "operationId")?,
                &mut raw_status,
            )
        })
        .map_err(error::from_core)?;
        Ok(offline_region_status_value_from_native(raw_status))
    }

    #[napi(js_name = "discardOfflineOperation")]
    pub fn discard_offline_operation(&self, operation_id: BigInt) -> Result<()> {
        core::check(unsafe {
            sys::mln_runtime_offline_operation_discard(
                self.state.as_ptr(),
                bigint_to_u64(operation_id, "operationId")?,
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "pollEvent")]
    pub fn poll_event(&self) -> Result<Option<RuntimeEvent>> {
        let mut raw = core::events::empty_runtime_event();
        let mut has_event = false;
        core::check(unsafe {
            sys::mln_runtime_poll_event(self.state.as_ptr(), &mut raw, &mut has_event)
        })
        .map_err(error::from_core)?;
        if !has_event {
            return Ok(None);
        }

        let copied =
            unsafe { core::events::runtime_event_from_native(&raw) }.map_err(error::from_core)?;
        Ok(Some(RuntimeEvent::from_copied(copied, raw.type_)))
    }
}

unsafe extern "C" fn resource_provider_trampoline(
    user_data: *mut c_void,
    request: *const sys::mln_resource_request,
    handle: *mut sys::mln_resource_request_handle,
) -> u32 {
    catch_unwind(AssertUnwindSafe(|| unsafe {
        resource_provider_trampoline_inner(user_data, request, handle)
    }))
    .unwrap_or(core::resource::UNKNOWN_PROVIDER_DECISION)
}

unsafe fn resource_provider_trampoline_inner(
    user_data: *mut c_void,
    request: *const sys::mln_resource_request,
    handle: *mut sys::mln_resource_request_handle,
) -> u32 {
    if user_data.is_null() || request.is_null() || handle.is_null() {
        return core::resource::UNKNOWN_PROVIDER_DECISION;
    }
    let provider_ptr = user_data as *const ResourceProviderState;
    // SAFETY: user_data was created from Arc::as_ptr and native retains it while
    // the runtime state owns the provider. Increment before from_raw to create
    // an owned clone for this callback invocation.
    unsafe { Arc::increment_strong_count(provider_ptr) };
    let provider = unsafe { Arc::from_raw(provider_ptr) };
    let raw_request = unsafe { &*request };
    let url = if provider.routes.iter().any(|route| route.needs_url()) {
        if raw_request.url.is_null() {
            return sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
        }
        Some(unsafe { CStr::from_ptr(raw_request.url) }.to_string_lossy())
    } else {
        None
    };
    if !provider
        .routes
        .iter()
        .any(|route| resource_matcher_matches_borrowed(route, raw_request.kind, url.as_deref()))
    {
        return sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
    }
    let request = match unsafe { core::resource::copy_resource_request(raw_request) } {
        Ok(request) => request,
        Err(_) => return core::resource::UNKNOWN_PROVIDER_DECISION,
    };
    let handle_state = match unsafe {
        core::resource::ResourceRequestHandleState::new(
            handle,
            core::resource::ResourceRequestHandleFns::NATIVE,
        )
    } {
        Ok(handle_state) => handle_state,
        Err(_) => return core::resource::UNKNOWN_PROVIDER_DECISION,
    };
    let Some(completion_token) = register_resource_request_handle(handle_state.clone(), &provider)
    else {
        return handle_state.finish_provider_exception();
    };
    let provider_request = resource_provider_request_from_core(request, completion_token.clone());
    let status = provider.callback.call(
        Ok(provider_request),
        ThreadsafeFunctionCallMode::NonBlocking,
    );
    if !matches!(status, napi::Status::Ok) {
        unregister_resource_request_handle(&completion_token);
        return handle_state.finish_provider_exception();
    }
    handle_state.finish_provider_decision(core::resource::ResourceProviderDecision::Handle)
}

unsafe extern "C" fn resource_transform_trampoline(
    user_data: *mut c_void,
    kind: u32,
    url: *const c_char,
    out_response: *mut sys::mln_resource_transform_response,
) -> sys::mln_status {
    catch_unwind(AssertUnwindSafe(|| unsafe {
        resource_transform_trampoline_inner(user_data, kind, url, out_response)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

unsafe fn resource_transform_trampoline_inner(
    user_data: *mut c_void,
    kind: u32,
    url: *const c_char,
    out_response: *mut sys::mln_resource_transform_response,
) -> sys::mln_status {
    let init_status =
        unsafe { core::resource::initialize_resource_transform_response(out_response) };
    if init_status != sys::MLN_STATUS_OK {
        return init_status;
    }
    if user_data.is_null() || url.is_null() {
        return sys::MLN_STATUS_INVALID_ARGUMENT;
    }

    let transform_ptr = user_data as *const ResourceTransformState;
    // SAFETY: user_data was created from Arc::as_ptr. Take an owned Arc clone
    // while the callback reads rules so concurrent clear/replace cannot free
    // the callback state mid-call.
    unsafe { Arc::increment_strong_count(transform_ptr) };
    let transform = unsafe { Arc::from_raw(transform_ptr) };
    let url = unsafe { CStr::from_ptr(url) }.to_string_lossy();
    let Some(rule) = transform
        .rules
        .iter()
        .find(|rule| resource_matcher_matches(&rule.matcher, kind, &url))
    else {
        return sys::MLN_STATUS_OK;
    };

    if let Some(replacement) = &rule.replacement_url {
        return unsafe {
            sys::mln_resource_transform_response_set_url(
                out_response,
                replacement.as_bytes().as_ptr().cast(),
                replacement.as_bytes().len(),
            )
        };
    }

    let Some(replacement_url_prefix) = &rule.replacement_url_prefix else {
        return sys::MLN_STATUS_OK;
    };
    let Some(url_prefix) = &rule.matcher.url_prefix else {
        return sys::MLN_STATUS_OK;
    };
    let suffix = url.strip_prefix(url_prefix).unwrap_or_default();
    let replacement = format!("{replacement_url_prefix}{suffix}");
    unsafe {
        sys::mln_resource_transform_response_set_url(
            out_response,
            replacement.as_bytes().as_ptr().cast(),
            replacement.len(),
        )
    }
}

impl RuntimeEvent {
    fn from_copied(event: core::CopiedRuntimeEvent, raw_event_type: u32) -> Self {
        let payload_kind = runtime_event_payload_kind(&event.payload).to_owned();
        Self {
            event_type: runtime_event_type_name(event.event_type).to_owned(),
            raw_event_type,
            source_type: runtime_event_source_type_name(event.source.source_type).to_owned(),
            raw_source_type: event.source.source_type,
            source_address: BigInt::from(event.source.source_address as u64),
            code: event.code,
            message: event.message,
            payload_kind,
            payload: runtime_event_payload_to_value(event.payload),
        }
    }
}

fn resource_request_handles() -> &'static Mutex<HashMap<String, ResourceRequestRegistration>> {
    RESOURCE_REQUEST_HANDLES.get_or_init(|| Mutex::new(HashMap::new()))
}

fn register_resource_request_handle(
    handle: Arc<core::resource::ResourceRequestHandleState>,
    provider: &Arc<ResourceProviderState>,
) -> Option<String> {
    let token_id = RESOURCE_REQUEST_TOKEN_IDS.fetch_add(1, Ordering::Relaxed);
    let completion_token = format!("resource-request:{token_id}");
    {
        let Ok(mut handles) = resource_request_handles().lock() else {
            return None;
        };
        handles.insert(
            completion_token.clone(),
            ResourceRequestRegistration {
                handle,
                provider: Arc::downgrade(provider),
            },
        );
    }
    let pending = provider.pending_completion_tokens.lock();
    let Ok(mut pending) = pending else {
        let _ = resource_request_handles()
            .lock()
            .map(|mut handles| handles.remove(&completion_token));
        return None;
    };
    pending.insert(completion_token.clone());
    Some(completion_token)
}

fn unregister_resource_request_handle(
    completion_token: &str,
) -> Option<ResourceRequestRegistration> {
    let registration = resource_request_handles()
        .lock()
        .ok()
        .and_then(|mut handles| handles.remove(completion_token));
    if let Some(registration) = &registration
        && let Some(provider) = registration.provider.upgrade()
        && let Ok(mut pending) = provider.pending_completion_tokens.lock()
    {
        pending.remove(completion_token);
    }
    registration
}

fn resource_matcher_from_input(input: ResourceRouteInput) -> Result<ResourceMatcher> {
    Ok(ResourceMatcher {
        kind: input
            .kind
            .as_deref()
            .map(resource_kind_from_name)
            .transpose()?,
        url: input.url,
        url_prefix: input.url_prefix,
    })
}

fn resource_transform_rule_from_input(
    input: ResourceTransformRuleInput,
) -> Result<ResourceTransformRule> {
    let replacement_url = input
        .replacement_url
        .map(|url| {
            CString::new(url)
                .map_err(|_| error::invalid_argument("replacementUrl must not contain null bytes"))
        })
        .transpose()?;
    let replacement_url_prefix = input.replacement_url_prefix;
    if replacement_url.is_some() == replacement_url_prefix.is_some() {
        return Err(error::invalid_argument(
            "resource transform rule must set exactly one of replacementUrl or replacementUrlPrefix",
        ));
    }
    if replacement_url_prefix.is_some() && input.url_prefix.is_none() {
        return Err(error::invalid_argument(
            "resource transform rule with replacementUrlPrefix must also set urlPrefix",
        ));
    }
    if let Some(prefix) = &replacement_url_prefix {
        CString::new(prefix.as_str()).map_err(|_| {
            error::invalid_argument("replacementUrlPrefix must not contain null bytes")
        })?;
    }
    Ok(ResourceTransformRule {
        matcher: resource_matcher_from_input(ResourceRouteInput {
            kind: input.kind,
            url: input.url,
            url_prefix: input.url_prefix,
        })?,
        replacement_url,
        replacement_url_prefix,
    })
}

fn resource_matcher_matches(matcher: &ResourceMatcher, raw_kind: u32, url: &str) -> bool {
    resource_matcher_matches_borrowed(matcher, raw_kind, Some(url))
}

fn resource_matcher_matches_borrowed(
    matcher: &ResourceMatcher,
    raw_kind: u32,
    url: Option<&str>,
) -> bool {
    if matcher.kind.is_some_and(|kind| kind != raw_kind) {
        return false;
    }
    if matcher.url.is_some() && matcher.url.as_deref() != url {
        return false;
    }
    if matcher
        .url_prefix
        .as_deref()
        .is_some_and(|prefix| !url.is_some_and(|url| url.starts_with(prefix)))
    {
        return false;
    }
    true
}

impl ResourceMatcher {
    fn needs_url(&self) -> bool {
        self.url.is_some() || self.url_prefix.is_some()
    }
}

fn validate_resource_request_completion_token(completion_token: &str) -> Result<()> {
    let token_id = completion_token
        .strip_prefix("resource-request:")
        .ok_or_else(|| error::invalid_argument("ResourceRequestHandle token is invalid"))?;
    if token_id.is_empty() || token_id.parse::<u64>().is_err() {
        return Err(error::invalid_argument(
            "ResourceRequestHandle token is invalid",
        ));
    }
    Ok(())
}

fn resource_provider_request_from_core(
    request: core::resource::ResourceRequest,
    completion_token: String,
) -> ResourceProviderRequest {
    ResourceProviderRequest {
        url: request.url,
        kind: resource_kind_name(request.kind).to_owned(),
        raw_kind: request.raw_kind,
        loading_method: resource_loading_method_name(request.loading_method).to_owned(),
        raw_loading_method: request.raw_loading_method,
        priority: resource_priority_name(request.priority).to_owned(),
        raw_priority: request.raw_priority,
        usage: resource_usage_name(request.usage).to_owned(),
        raw_usage: request.raw_usage,
        storage_policy: resource_storage_policy_name(request.storage_policy).to_owned(),
        raw_storage_policy: request.raw_storage_policy,
        range: request.range.map(|range| ResourceByteRange {
            start: BigInt::from(range.start),
            end: BigInt::from(range.end),
        }),
        prior_modified_unix_ms: request.prior_modified_unix_ms.map(BigInt::from),
        prior_expires_unix_ms: request.prior_expires_unix_ms.map(BigInt::from),
        prior_etag: request.prior_etag,
        prior_data: Uint8Array::from(request.prior_data),
        completion_token,
    }
}

fn resource_response_from_input(input: ResourceResponseInput) -> Result<core::ResourceResponse> {
    let status = resource_response_status_from_string(input.status.as_deref().unwrap_or("ok"))?;
    let error_reason =
        resource_error_reason_from_string(input.error_reason.as_deref().unwrap_or("none"))?;
    let mut response = core::ResourceResponse::default();
    response.status = status;
    response.error_reason = error_reason;
    response.bytes = input.bytes.map(|bytes| bytes.to_vec()).unwrap_or_default();
    response.error_message = input.error_message;
    response.must_revalidate = input.must_revalidate.unwrap_or(false);
    response.modified_unix_ms = input
        .modified_unix_ms
        .map(|value| bigint_to_i64(value, "modifiedUnixMs"))
        .transpose()?;
    response.expires_unix_ms = input
        .expires_unix_ms
        .map(|value| bigint_to_i64(value, "expiresUnixMs"))
        .transpose()?;
    response.etag = input.etag;
    response.retry_after_unix_ms = input
        .retry_after_unix_ms
        .map(|value| bigint_to_i64(value, "retryAfterUnixMs"))
        .transpose()?;
    Ok(response)
}

fn resource_response_status_from_string(value: &str) -> Result<core::ResourceResponseStatus> {
    match value {
        "ok" => Ok(core::ResourceResponseStatus::Ok),
        "error" => Ok(core::ResourceResponseStatus::Error),
        "noContent" => Ok(core::ResourceResponseStatus::NoContent),
        "notModified" => Ok(core::ResourceResponseStatus::NotModified),
        other => Err(error::invalid_argument(format!(
            "resource response status must be 'ok', 'error', 'noContent', or 'notModified', got '{other}'"
        ))),
    }
}

fn resource_error_reason_from_string(value: &str) -> Result<core::ResourceErrorReason> {
    match value {
        "none" => Ok(core::ResourceErrorReason::None),
        "notFound" => Ok(core::ResourceErrorReason::NotFound),
        "server" => Ok(core::ResourceErrorReason::Server),
        "connection" => Ok(core::ResourceErrorReason::Connection),
        "rateLimit" => Ok(core::ResourceErrorReason::RateLimit),
        "other" => Ok(core::ResourceErrorReason::Other),
        other => Err(error::invalid_argument(format!(
            "resource error reason must be 'none', 'notFound', 'server', 'connection', 'rateLimit', or 'other', got '{other}'"
        ))),
    }
}

fn resource_kind_name(kind: core::ResourceKind) -> &'static str {
    match kind {
        core::ResourceKind::Unknown => "unknown",
        core::ResourceKind::Style => "style",
        core::ResourceKind::Source => "source",
        core::ResourceKind::Tile => "tile",
        core::ResourceKind::Glyphs => "glyphs",
        core::ResourceKind::SpriteImage => "sprite-image",
        core::ResourceKind::SpriteJson => "sprite-json",
        core::ResourceKind::Image => "image",
        core::ResourceKind::UnknownRaw(_) => "unknown",
        _ => "unknown",
    }
}

fn resource_kind_from_name(kind: &str) -> Result<u32> {
    match kind {
        "unknown" => Ok(sys::MLN_RESOURCE_KIND_UNKNOWN),
        "style" => Ok(sys::MLN_RESOURCE_KIND_STYLE),
        "source" => Ok(sys::MLN_RESOURCE_KIND_SOURCE),
        "tile" => Ok(sys::MLN_RESOURCE_KIND_TILE),
        "glyphs" => Ok(sys::MLN_RESOURCE_KIND_GLYPHS),
        "sprite-image" => Ok(sys::MLN_RESOURCE_KIND_SPRITE_IMAGE),
        "sprite-json" => Ok(sys::MLN_RESOURCE_KIND_SPRITE_JSON),
        "image" => Ok(sys::MLN_RESOURCE_KIND_IMAGE),
        other => Err(error::invalid_argument(format!(
            "resource kind must be 'unknown', 'style', 'source', 'tile', 'glyphs', 'sprite-image', 'sprite-json', or 'image', got '{other}'"
        ))),
    }
}

fn resource_loading_method_name(value: core::ResourceLoadingMethod) -> &'static str {
    match value {
        core::ResourceLoadingMethod::All => "all",
        core::ResourceLoadingMethod::CacheOnly => "cacheOnly",
        core::ResourceLoadingMethod::NetworkOnly => "networkOnly",
        core::ResourceLoadingMethod::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn resource_priority_name(value: core::ResourcePriority) -> &'static str {
    match value {
        core::ResourcePriority::Low => "low",
        core::ResourcePriority::Regular => "regular",
        core::ResourcePriority::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn resource_usage_name(value: core::ResourceUsage) -> &'static str {
    match value {
        core::ResourceUsage::Online => "online",
        core::ResourceUsage::Offline => "offline",
        core::ResourceUsage::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn resource_storage_policy_name(value: core::ResourceStoragePolicy) -> &'static str {
    match value {
        core::ResourceStoragePolicy::Permanent => "permanent",
        core::ResourceStoragePolicy::Volatile => "volatile",
        core::ResourceStoragePolicy::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn runtime_event_source_type_name(raw: u32) -> &'static str {
    match raw {
        sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME => "runtime",
        sys::MLN_RUNTIME_EVENT_SOURCE_MAP => "map",
        _ => "unknown",
    }
}

fn runtime_event_payload_kind(payload: &core::RuntimeEventPayload) -> &'static str {
    match payload {
        core::RuntimeEventPayload::None => "none",
        core::RuntimeEventPayload::RenderFrame(_) => "render-frame",
        core::RuntimeEventPayload::RenderMap(_) => "render-map",
        core::RuntimeEventPayload::StyleImageMissing(_) => "style-image-missing",
        core::RuntimeEventPayload::TileAction(_) => "tile-action",
        core::RuntimeEventPayload::OfflineRegionStatus(_) => "offline-region-status",
        core::RuntimeEventPayload::OfflineRegionResponseError(_) => "offline-region-response-error",
        core::RuntimeEventPayload::OfflineRegionTileCountLimit(_) => {
            "offline-region-tile-count-limit"
        }
        core::RuntimeEventPayload::OfflineOperationCompleted(_) => "offline-operation-completed",
        core::RuntimeEventPayload::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn runtime_event_payload_to_value(payload: core::RuntimeEventPayload) -> RuntimeEventPayloadValue {
    match payload {
        core::RuntimeEventPayload::None => {
            empty_runtime_event_payload("none", sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE)
        }
        core::RuntimeEventPayload::RenderFrame(event) => RuntimeEventPayloadValue {
            kind: "render-frame".to_owned(),
            raw_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME,
            render_frame: Some(RenderFrameEventValue {
                mode: render_mode_name(event.mode).to_owned(),
                raw_mode: render_mode_raw(event.mode),
                needs_repaint: event.needs_repaint,
                placement_changed: event.placement_changed,
                stats: rendering_stats_to_value(event.stats),
            }),
            ..empty_runtime_event_payload_fields()
        },
        core::RuntimeEventPayload::RenderMap(event) => RuntimeEventPayloadValue {
            kind: "render-map".to_owned(),
            raw_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP,
            render_map: Some(RenderMapEventValue {
                mode: render_mode_name(event.mode).to_owned(),
                raw_mode: render_mode_raw(event.mode),
            }),
            ..empty_runtime_event_payload_fields()
        },
        core::RuntimeEventPayload::StyleImageMissing(event) => RuntimeEventPayloadValue {
            kind: "style-image-missing".to_owned(),
            raw_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING,
            style_image_missing: Some(StyleImageMissingEventValue {
                image_id: event.image_id,
            }),
            ..empty_runtime_event_payload_fields()
        },
        core::RuntimeEventPayload::TileAction(event) => RuntimeEventPayloadValue {
            kind: "tile-action".to_owned(),
            raw_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION,
            tile_action: Some(TileActionEventValue {
                operation: tile_operation_name(event.operation).to_owned(),
                raw_operation: tile_operation_raw(event.operation),
                tile_id: tile_id_to_value(event.tile_id),
                source_id: event.source_id,
            }),
            ..empty_runtime_event_payload_fields()
        },
        core::RuntimeEventPayload::OfflineRegionStatus(event) => RuntimeEventPayloadValue {
            kind: "offline-region-status".to_owned(),
            raw_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS,
            offline_region_status: Some(OfflineRegionStatusEventValue {
                region_id: BigInt::from(event.region_id),
                status: offline_region_status_to_value(event.status),
            }),
            ..empty_runtime_event_payload_fields()
        },
        core::RuntimeEventPayload::OfflineRegionResponseError(event) => RuntimeEventPayloadValue {
            kind: "offline-region-response-error".to_owned(),
            raw_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR,
            offline_region_response_error: Some(OfflineRegionResponseErrorEventValue {
                region_id: BigInt::from(event.region_id),
                reason: resource_error_reason_name(event.reason).to_owned(),
                raw_reason: event.reason.raw_value(),
            }),
            ..empty_runtime_event_payload_fields()
        },
        core::RuntimeEventPayload::OfflineRegionTileCountLimit(event) => RuntimeEventPayloadValue {
            kind: "offline-region-tile-count-limit".to_owned(),
            raw_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT,
            offline_region_tile_count_limit: Some(OfflineRegionTileCountLimitEventValue {
                region_id: BigInt::from(event.region_id),
                limit: BigInt::from(event.limit),
            }),
            ..empty_runtime_event_payload_fields()
        },
        core::RuntimeEventPayload::OfflineOperationCompleted(event) => RuntimeEventPayloadValue {
            kind: "offline-operation-completed".to_owned(),
            raw_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED,
            offline_operation_completed: Some(OfflineOperationCompletedEventValue {
                operation_id: BigInt::from(event.operation_id),
                operation_kind: offline_operation_kind_name(event.operation_kind).to_owned(),
                raw_operation_kind: event.raw_operation_kind,
                result_kind: offline_operation_result_kind_name(event.result_kind).to_owned(),
                raw_result_kind: event.raw_result_kind,
                result_status: event.result_status,
                found: event.found,
            }),
            ..empty_runtime_event_payload_fields()
        },
        core::RuntimeEventPayload::Unknown(payload) => RuntimeEventPayloadValue {
            kind: "unknown".to_owned(),
            raw_type: payload.raw_type,
            unknown: Some(UnknownRuntimeEventPayloadValue {
                raw_type: payload.raw_type,
                bytes: Uint8Array::from(payload.bytes),
            }),
            ..empty_runtime_event_payload_fields()
        },
        _ => empty_runtime_event_payload("unknown", 0),
    }
}

fn empty_runtime_event_payload(kind: &str, raw_type: u32) -> RuntimeEventPayloadValue {
    RuntimeEventPayloadValue {
        kind: kind.to_owned(),
        raw_type,
        ..empty_runtime_event_payload_fields()
    }
}

fn empty_runtime_event_payload_fields() -> RuntimeEventPayloadValue {
    RuntimeEventPayloadValue {
        kind: String::new(),
        raw_type: 0,
        render_frame: None,
        render_map: None,
        style_image_missing: None,
        tile_action: None,
        offline_region_status: None,
        offline_region_response_error: None,
        offline_region_tile_count_limit: None,
        offline_operation_completed: None,
        unknown: None,
    }
}

fn rendering_stats_to_value(stats: core::RenderingStats) -> RenderingStatsValue {
    RenderingStatsValue {
        encoding_time: stats.encoding_time,
        rendering_time: stats.rendering_time,
        frame_count: BigInt::from(stats.frame_count),
        draw_call_count: BigInt::from(stats.draw_call_count),
        total_draw_call_count: BigInt::from(stats.total_draw_call_count),
    }
}

fn tile_id_to_value(tile_id: core::TileId) -> TileIdValue {
    TileIdValue {
        overscaled_z: tile_id.overscaled_z,
        wrap: tile_id.wrap,
        canonical_z: tile_id.canonical_z,
        canonical_x: tile_id.canonical_x,
        canonical_y: tile_id.canonical_y,
    }
}

fn render_mode_name(mode: core::RenderMode) -> &'static str {
    match mode {
        core::RenderMode::Partial => "partial",
        core::RenderMode::Full => "full",
        core::RenderMode::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn render_mode_raw(mode: core::RenderMode) -> u32 {
    match mode {
        core::RenderMode::Partial => sys::MLN_RENDER_MODE_PARTIAL,
        core::RenderMode::Full => sys::MLN_RENDER_MODE_FULL,
        core::RenderMode::Unknown(raw) => raw,
        _ => sys::MLN_RENDER_MODE_PARTIAL,
    }
}

fn tile_operation_name(operation: core::TileOperation) -> &'static str {
    match operation {
        core::TileOperation::RequestedFromCache => "requestedFromCache",
        core::TileOperation::RequestedFromNetwork => "requestedFromNetwork",
        core::TileOperation::LoadFromNetwork => "loadFromNetwork",
        core::TileOperation::LoadFromCache => "loadFromCache",
        core::TileOperation::StartParse => "startParse",
        core::TileOperation::EndParse => "endParse",
        core::TileOperation::Error => "error",
        core::TileOperation::Cancelled => "cancelled",
        core::TileOperation::Null => "null",
        core::TileOperation::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn tile_operation_raw(operation: core::TileOperation) -> u32 {
    match operation {
        core::TileOperation::RequestedFromCache => sys::MLN_TILE_OPERATION_REQUESTED_FROM_CACHE,
        core::TileOperation::RequestedFromNetwork => sys::MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK,
        core::TileOperation::LoadFromNetwork => sys::MLN_TILE_OPERATION_LOAD_FROM_NETWORK,
        core::TileOperation::LoadFromCache => sys::MLN_TILE_OPERATION_LOAD_FROM_CACHE,
        core::TileOperation::StartParse => sys::MLN_TILE_OPERATION_START_PARSE,
        core::TileOperation::EndParse => sys::MLN_TILE_OPERATION_END_PARSE,
        core::TileOperation::Error => sys::MLN_TILE_OPERATION_ERROR,
        core::TileOperation::Cancelled => sys::MLN_TILE_OPERATION_CANCELLED,
        core::TileOperation::Null => sys::MLN_TILE_OPERATION_NULL,
        core::TileOperation::Unknown(raw) => raw,
        _ => sys::MLN_TILE_OPERATION_NULL,
    }
}

fn resource_error_reason_name(reason: core::ResourceErrorReason) -> &'static str {
    match reason {
        core::ResourceErrorReason::None => "none",
        core::ResourceErrorReason::NotFound => "notFound",
        core::ResourceErrorReason::Server => "server",
        core::ResourceErrorReason::Connection => "connection",
        core::ResourceErrorReason::RateLimit => "rateLimit",
        core::ResourceErrorReason::Other => "other",
        core::ResourceErrorReason::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn offline_operation_kind_name(kind: core::OfflineOperationKind) -> &'static str {
    match kind {
        core::OfflineOperationKind::AmbientCache => "ambientCache",
        core::OfflineOperationKind::RegionCreate => "regionCreate",
        core::OfflineOperationKind::RegionGet => "regionGet",
        core::OfflineOperationKind::RegionsList => "regionsList",
        core::OfflineOperationKind::RegionsMergeDatabase => "regionsMergeDatabase",
        core::OfflineOperationKind::RegionUpdateMetadata => "regionUpdateMetadata",
        core::OfflineOperationKind::RegionGetStatus => "regionGetStatus",
        core::OfflineOperationKind::RegionSetObserved => "regionSetObserved",
        core::OfflineOperationKind::RegionSetDownloadState => "regionSetDownloadState",
        core::OfflineOperationKind::RegionInvalidate => "regionInvalidate",
        core::OfflineOperationKind::RegionDelete => "regionDelete",
        core::OfflineOperationKind::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn offline_operation_result_kind_name(kind: core::OfflineOperationResultKind) -> &'static str {
    match kind {
        core::OfflineOperationResultKind::None => "none",
        core::OfflineOperationResultKind::Region => "region",
        core::OfflineOperationResultKind::OptionalRegion => "optionalRegion",
        core::OfflineOperationResultKind::RegionList => "regionList",
        core::OfflineOperationResultKind::RegionStatus => "regionStatus",
        core::OfflineOperationResultKind::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn runtime_event_type_name(event_type: core::RuntimeEventType) -> &'static str {
    match event_type {
        core::RuntimeEventType::MapCameraWillChange => "map-camera-will-change",
        core::RuntimeEventType::MapCameraIsChanging => "map-camera-is-changing",
        core::RuntimeEventType::MapCameraDidChange => "map-camera-did-change",
        core::RuntimeEventType::MapStyleLoaded => "map-style-loaded",
        core::RuntimeEventType::MapLoadingStarted => "map-loading-started",
        core::RuntimeEventType::MapLoadingFinished => "map-loading-finished",
        core::RuntimeEventType::MapLoadingFailed => "map-loading-failed",
        core::RuntimeEventType::MapIdle => "map-idle",
        core::RuntimeEventType::MapRenderUpdateAvailable => "map-render-update-available",
        core::RuntimeEventType::MapRenderError => "map-render-error",
        core::RuntimeEventType::MapStillImageFinished => "map-still-image-finished",
        core::RuntimeEventType::MapStillImageFailed => "map-still-image-failed",
        core::RuntimeEventType::MapRenderFrameStarted => "map-render-frame-started",
        core::RuntimeEventType::MapRenderFrameFinished => "map-render-frame-finished",
        core::RuntimeEventType::MapRenderMapStarted => "map-render-map-started",
        core::RuntimeEventType::MapRenderMapFinished => "map-render-map-finished",
        core::RuntimeEventType::MapStyleImageMissing => "map-style-image-missing",
        core::RuntimeEventType::MapTileAction => "map-tile-action",
        core::RuntimeEventType::OfflineRegionStatusChanged => "offline-region-status-changed",
        core::RuntimeEventType::OfflineRegionResponseError => "offline-region-response-error",
        core::RuntimeEventType::OfflineRegionTileCountLimitExceeded => {
            "offline-region-tile-count-limit-exceeded"
        }
        core::RuntimeEventType::OfflineOperationCompleted => "offline-operation-completed",
        core::RuntimeEventType::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

impl NativeRuntimeHandle {
    pub(crate) fn as_ptr(&self) -> *mut sys::mln_runtime {
        self.state.as_ptr()
    }

    pub(crate) fn mark_map_created(&self) {
        self.has_created_map.store(true, Ordering::Release);
    }

    fn release_resource_callback_state(&self) {
        if let Ok(mut transform) = self.resource_transform.lock() {
            *transform = None;
        }
        if let Ok(mut provider) = self.resource_provider.lock() {
            *provider = None;
        }
    }
}

impl Drop for ResourceProviderState {
    fn drop(&mut self) {
        let pending = self
            .pending_completion_tokens
            .lock()
            .map(|mut pending| pending.drain().collect::<Vec<_>>())
            .unwrap_or_default();
        for completion_token in pending {
            if let Some(registration) = resource_request_handles()
                .lock()
                .ok()
                .and_then(|mut handles| handles.remove(&completion_token))
            {
                registration.handle.close();
            }
        }
    }
}

impl Drop for NativeRuntimeHandle {
    fn drop(&mut self) {
        if let Some(address) = self.state.leak_for_report() {
            crate::maplibre::report_native_handle_leak(self.state.type_name(), address);
            if let Ok(mut transform) = self.resource_transform.lock()
                && let Some(transform) = transform.take()
            {
                std::mem::forget(transform);
            }
            if let Ok(mut provider) = self.resource_provider.lock()
                && let Some(provider) = provider.take()
            {
                std::mem::forget(provider);
            }
        }
    }
}

impl RuntimeOptions {
    fn into_core(self) -> Result<core::RuntimeOptions> {
        let mut options = core::RuntimeOptions::default();
        if let Some(asset_path) = self.asset_path {
            options.asset_path = Some(asset_path);
        }
        if let Some(cache_path) = self.cache_path {
            options.cache_path = Some(cache_path);
        }
        if let Some(maximum_cache_size) = self.maximum_cache_size {
            options.maximum_cache_size = Some(maximum_cache_size_to_u64(maximum_cache_size)?);
        }
        Ok(options)
    }
}

fn copy_offline_region_snapshot_value(
    snapshot: *mut sys::mln_offline_region_snapshot,
) -> Result<OfflineRegionInfoValue> {
    let snapshot = NonNull::new(snapshot)
        .ok_or_else(|| error::invalid_argument("offline region snapshot result was null"))?;
    let info = unsafe { core::runtime::copy_offline_region_snapshot(snapshot) }
        .map_err(error::from_core)?;
    offline_region_info_to_value(info)
}

fn copy_offline_region_list_value(
    list: *mut sys::mln_offline_region_list,
) -> Result<Vec<OfflineRegionInfoValue>> {
    let list = NonNull::new(list)
        .ok_or_else(|| error::invalid_argument("offline region list result was null"))?;
    let regions =
        unsafe { core::runtime::copy_offline_region_list(list) }.map_err(error::from_core)?;
    regions
        .into_iter()
        .map(offline_region_info_to_value)
        .collect()
}

fn offline_region_info_to_value(info: core::OfflineRegionInfo) -> Result<OfflineRegionInfoValue> {
    Ok(OfflineRegionInfoValue {
        id: BigInt::from(info.id),
        definition: offline_region_definition_to_value(info.definition)?,
        metadata: Uint8Array::from(info.metadata),
    })
}

fn offline_region_definition_to_value(
    definition: core::OfflineRegionDefinition,
) -> Result<OfflineRegionDefinitionValue> {
    match definition {
        core::OfflineRegionDefinition::TilePyramid {
            style_url,
            bounds,
            min_zoom,
            max_zoom,
            pixel_ratio,
            include_ideographs,
        } => Ok(OfflineRegionDefinitionValue {
            kind: "tilePyramid".to_owned(),
            style_url,
            bounds: Some(crate::values::LatLngBounds::from_core(bounds)),
            geometry: None,
            min_zoom,
            max_zoom,
            pixel_ratio: f64::from(pixel_ratio),
            include_ideographs,
        }),
        core::OfflineRegionDefinition::GeometryRegion {
            style_url,
            geometry,
            min_zoom,
            max_zoom,
            pixel_ratio,
            include_ideographs,
        } => Ok(OfflineRegionDefinitionValue {
            kind: "geometry".to_owned(),
            style_url,
            bounds: None,
            geometry: Some(geometry_to_serde(geometry)),
            min_zoom,
            max_zoom,
            pixel_ratio: f64::from(pixel_ratio),
            include_ideographs,
        }),
        _ => Err(error::invalid_argument("unknown offline region definition")),
    }
}

fn offline_region_status_value_from_native(
    raw: sys::mln_offline_region_status,
) -> OfflineRegionStatusValue {
    offline_region_status_to_value(core::events::offline_region_status_from_native(raw))
}

fn offline_region_status_to_value(status: core::OfflineRegionStatus) -> OfflineRegionStatusValue {
    OfflineRegionStatusValue {
        download_state: offline_region_download_state_name(status.download_state).to_owned(),
        raw_download_state: offline_region_download_state_raw(status.download_state),
        completed_resource_count: BigInt::from(status.completed_resource_count),
        completed_resource_size: BigInt::from(status.completed_resource_size),
        completed_tile_count: BigInt::from(status.completed_tile_count),
        required_tile_count: BigInt::from(status.required_tile_count),
        completed_tile_size: BigInt::from(status.completed_tile_size),
        required_resource_count: BigInt::from(status.required_resource_count),
        required_resource_count_is_precise: status.required_resource_count_is_precise,
        complete: status.complete,
    }
}

fn geometry_to_serde(geometry: core::Geometry) -> serde_json::Value {
    match geometry {
        core::Geometry::Empty => serde_json::Value::Null,
        core::Geometry::Point(coordinate) => serde_json::json!({
            "type": "Point",
            "coordinates": [coordinate.longitude, coordinate.latitude]
        }),
        core::Geometry::LineString(coordinates) => serde_json::json!({
            "type": "LineString",
            "coordinates": coordinates_to_serde(coordinates)
        }),
        core::Geometry::Polygon(rings) => serde_json::json!({
            "type": "Polygon",
            "coordinates": rings.into_iter().map(coordinates_to_serde).collect::<Vec<_>>()
        }),
        core::Geometry::MultiPoint(coordinates) => serde_json::json!({
            "type": "MultiPoint",
            "coordinates": coordinates_to_serde(coordinates)
        }),
        core::Geometry::MultiLineString(lines) => serde_json::json!({
            "type": "MultiLineString",
            "coordinates": lines.into_iter().map(coordinates_to_serde).collect::<Vec<_>>()
        }),
        core::Geometry::MultiPolygon(polygons) => serde_json::json!({
            "type": "MultiPolygon",
            "coordinates": polygons
                .into_iter()
                .map(|rings| rings.into_iter().map(coordinates_to_serde).collect::<Vec<_>>())
                .collect::<Vec<_>>()
        }),
        core::Geometry::GeometryCollection(geometries) => serde_json::json!({
            "type": "GeometryCollection",
            "geometries": geometries.into_iter().map(geometry_to_serde).collect::<Vec<_>>()
        }),
        _ => serde_json::Value::Null,
    }
}

fn coordinates_to_serde(coordinates: Vec<core::LatLng>) -> serde_json::Value {
    serde_json::Value::Array(
        coordinates
            .into_iter()
            .map(|coordinate| serde_json::json!([coordinate.longitude, coordinate.latitude]))
            .collect(),
    )
}

fn offline_operation_start(operation_id: u64) -> OfflineOperationStart {
    OfflineOperationStart {
        operation_id: BigInt::from(operation_id),
    }
}

fn offline_region_definition_from_input(
    input: OfflineRegionDefinitionInput,
) -> Result<core::OfflineRegionDefinition> {
    let include_ideographs = input.include_ideographs.unwrap_or(true);
    match input.kind.as_str() {
        "tilePyramid" => Ok(core::OfflineRegionDefinition::TilePyramid {
            style_url: input.style_url,
            bounds: input
                .bounds
                .ok_or_else(|| {
                    error::invalid_argument("tile pyramid offline region requires bounds")
                })?
                .into_core(),
            min_zoom: input.min_zoom,
            max_zoom: input.max_zoom,
            pixel_ratio: input.pixel_ratio as f32,
            include_ideographs,
        }),
        "geometry" => Ok(core::OfflineRegionDefinition::GeometryRegion {
            style_url: input.style_url,
            geometry: crate::map::parse_geometry(input.geometry.ok_or_else(|| {
                error::invalid_argument("geometry offline region requires geometry")
            })?)?,
            min_zoom: input.min_zoom,
            max_zoom: input.max_zoom,
            pixel_ratio: input.pixel_ratio as f32,
            include_ideographs,
        }),
        other => Err(error::invalid_argument(format!(
            "offline region kind must be 'tilePyramid' or 'geometry', got '{other}'"
        ))),
    }
}

fn offline_region_download_state_name(state: core::OfflineRegionDownloadState) -> &'static str {
    match state {
        core::OfflineRegionDownloadState::Inactive => "inactive",
        core::OfflineRegionDownloadState::Active => "active",
        core::OfflineRegionDownloadState::Unknown(_) => "unknown",
        _ => "unknown",
    }
}

fn offline_region_download_state_raw(state: core::OfflineRegionDownloadState) -> u32 {
    match state {
        core::OfflineRegionDownloadState::Inactive => sys::MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE,
        core::OfflineRegionDownloadState::Active => sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
        core::OfflineRegionDownloadState::Unknown(raw) => raw,
        _ => sys::MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE,
    }
}

fn offline_region_download_state_from_string(state: &str) -> Result<u32> {
    match state {
        "inactive" => core::OfflineRegionDownloadState::Inactive
            .raw_for_set()
            .map_err(error::from_core),
        "active" => core::OfflineRegionDownloadState::Active
            .raw_for_set()
            .map_err(error::from_core),
        other => Err(error::invalid_argument(format!(
            "offline region download state must be 'inactive' or 'active', got '{other}'"
        ))),
    }
}

fn ambient_cache_operation_from_string(operation: &str) -> Result<u32> {
    match operation {
        "resetDatabase" => Ok(sys::MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE),
        "packDatabase" => Ok(sys::MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE),
        "invalidate" => Ok(sys::MLN_AMBIENT_CACHE_OPERATION_INVALIDATE),
        "clear" => Ok(sys::MLN_AMBIENT_CACHE_OPERATION_CLEAR),
        other => Err(error::invalid_argument(format!(
            "ambient cache operation must be 'resetDatabase', 'packDatabase', 'invalidate', or 'clear', got '{other}'"
        ))),
    }
}

fn maximum_cache_size_to_u64(value: BigInt) -> Result<u64> {
    bigint_to_u64(value, "maximumCacheSize")
}

fn bigint_to_i64(value: BigInt, field_name: &str) -> Result<i64> {
    let (value, lossless) = value.get_i64();
    if !lossless {
        return Err(error::invalid_argument(format!(
            "{field_name} must be a signed 64-bit bigint"
        )));
    }
    Ok(value)
}

fn bigint_to_u64(value: BigInt, field_name: &str) -> Result<u64> {
    let (signed, value, lossless) = value.get_u64();
    if signed || !lossless {
        return Err(error::invalid_argument(format!(
            "{field_name} must be a non-negative 64-bit bigint"
        )));
    }
    Ok(value)
}
