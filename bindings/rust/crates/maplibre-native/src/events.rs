use std::mem;
use std::ptr;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::{Error, Result};

/// Rust-assigned identity for a map owned by a runtime.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct MapId(u64);

impl MapId {
    pub(crate) const fn new(value: u64) -> Self {
        Self(value)
    }

    /// Returns the runtime-local numeric map identity.
    pub const fn get(self) -> u64 {
        self.0
    }
}

/// Source object that emitted a runtime event.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum RuntimeEventSource {
    Runtime,
    Map(MapId),
    UnknownMap,
    Unknown(u32),
}

/// Runtime event type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum RuntimeEventType {
    MapCameraWillChange,
    MapCameraIsChanging,
    MapCameraDidChange,
    MapStyleLoaded,
    MapLoadingStarted,
    MapLoadingFinished,
    MapLoadingFailed,
    MapIdle,
    MapRenderUpdateAvailable,
    MapRenderError,
    MapStillImageFinished,
    MapStillImageFailed,
    MapRenderFrameStarted,
    MapRenderFrameFinished,
    MapRenderMapStarted,
    MapRenderMapFinished,
    MapStyleImageMissing,
    MapTileAction,
    OfflineRegionStatusChanged,
    OfflineRegionResponseError,
    OfflineRegionTileCountLimitExceeded,
    Unknown(u32),
}

impl RuntimeEventType {
    pub(crate) fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE => Self::MapCameraWillChange,
            sys::MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING => Self::MapCameraIsChanging,
            sys::MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE => Self::MapCameraDidChange,
            sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED => Self::MapStyleLoaded,
            sys::MLN_RUNTIME_EVENT_MAP_LOADING_STARTED => Self::MapLoadingStarted,
            sys::MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED => Self::MapLoadingFinished,
            sys::MLN_RUNTIME_EVENT_MAP_LOADING_FAILED => Self::MapLoadingFailed,
            sys::MLN_RUNTIME_EVENT_MAP_IDLE => Self::MapIdle,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE => Self::MapRenderUpdateAvailable,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_ERROR => Self::MapRenderError,
            sys::MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED => Self::MapStillImageFinished,
            sys::MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED => Self::MapStillImageFailed,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED => Self::MapRenderFrameStarted,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED => Self::MapRenderFrameFinished,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED => Self::MapRenderMapStarted,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED => Self::MapRenderMapFinished,
            sys::MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING => Self::MapStyleImageMissing,
            sys::MLN_RUNTIME_EVENT_MAP_TILE_ACTION => Self::MapTileAction,
            sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED => {
                Self::OfflineRegionStatusChanged
            }
            sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR => {
                Self::OfflineRegionResponseError
            }
            sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED => {
                Self::OfflineRegionTileCountLimitExceeded
            }
            _ => Self::Unknown(raw),
        }
    }
}

/// Render mode reported by render observer events.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum RenderMode {
    Partial,
    Full,
    Unknown(u32),
}

impl RenderMode {
    fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RENDER_MODE_PARTIAL => Self::Partial,
            sys::MLN_RENDER_MODE_FULL => Self::Full,
            _ => Self::Unknown(raw),
        }
    }
}

/// Tile operation reported by tile observer events.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum TileOperation {
    RequestedFromCache,
    RequestedFromNetwork,
    LoadFromNetwork,
    LoadFromCache,
    StartParse,
    EndParse,
    Error,
    Cancelled,
    Null,
    Unknown(u32),
}

impl TileOperation {
    fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_TILE_OPERATION_REQUESTED_FROM_CACHE => Self::RequestedFromCache,
            sys::MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK => Self::RequestedFromNetwork,
            sys::MLN_TILE_OPERATION_LOAD_FROM_NETWORK => Self::LoadFromNetwork,
            sys::MLN_TILE_OPERATION_LOAD_FROM_CACHE => Self::LoadFromCache,
            sys::MLN_TILE_OPERATION_START_PARSE => Self::StartParse,
            sys::MLN_TILE_OPERATION_END_PARSE => Self::EndParse,
            sys::MLN_TILE_OPERATION_ERROR => Self::Error,
            sys::MLN_TILE_OPERATION_CANCELLED => Self::Cancelled,
            sys::MLN_TILE_OPERATION_NULL => Self::Null,
            _ => Self::Unknown(raw),
        }
    }
}

/// Offline region download state copied from event payloads.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum OfflineRegionDownloadState {
    Inactive,
    Active,
    Unknown(u32),
}

impl OfflineRegionDownloadState {
    fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE => Self::Inactive,
            sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE => Self::Active,
            _ => Self::Unknown(raw),
        }
    }
}

/// Resource error reason copied from event payloads.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceErrorReason {
    None,
    NotFound,
    Server,
    Connection,
    RateLimit,
    Other,
    Unknown(u32),
}

impl ResourceErrorReason {
    fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RESOURCE_ERROR_REASON_NONE => Self::None,
            sys::MLN_RESOURCE_ERROR_REASON_NOT_FOUND => Self::NotFound,
            sys::MLN_RESOURCE_ERROR_REASON_SERVER => Self::Server,
            sys::MLN_RESOURCE_ERROR_REASON_CONNECTION => Self::Connection,
            sys::MLN_RESOURCE_ERROR_REASON_RATE_LIMIT => Self::RateLimit,
            sys::MLN_RESOURCE_ERROR_REASON_OTHER => Self::Other,
            _ => Self::Unknown(raw),
        }
    }
}

/// Rendering statistics copied from a render-frame event payload.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct RenderingStats {
    pub encoding_time: f64,
    pub rendering_time: f64,
    pub frame_count: i64,
    pub draw_call_count: i64,
    pub total_draw_call_count: i64,
}

impl RenderingStats {
    fn from_native(raw: sys::mln_rendering_stats) -> Self {
        Self {
            encoding_time: raw.encoding_time,
            rendering_time: raw.rendering_time,
            frame_count: raw.frame_count,
            draw_call_count: raw.draw_call_count,
            total_draw_call_count: raw.total_draw_call_count,
        }
    }
}

/// Render-frame event payload.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct RenderFrameEvent {
    pub mode: RenderMode,
    pub needs_repaint: bool,
    pub placement_changed: bool,
    pub stats: RenderingStats,
}

/// Render-map event payload.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct RenderMapEvent {
    pub mode: RenderMode,
}

/// Overscaled tile identity copied from a tile event payload.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct TileId {
    pub overscaled_z: u32,
    pub wrap: i32,
    pub canonical_z: u32,
    pub canonical_x: u32,
    pub canonical_y: u32,
}

impl TileId {
    fn from_native(raw: sys::mln_tile_id) -> Self {
        Self {
            overscaled_z: raw.overscaled_z,
            wrap: raw.wrap,
            canonical_z: raw.canonical_z,
            canonical_x: raw.canonical_x,
            canonical_y: raw.canonical_y,
        }
    }
}

/// Tile-action event payload.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct TileActionEvent {
    pub operation: TileOperation,
    pub tile_id: TileId,
    pub source_id: String,
}

/// Offline region status copied from native event payloads.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct OfflineRegionStatus {
    pub download_state: OfflineRegionDownloadState,
    pub completed_resource_count: u64,
    pub completed_resource_size: u64,
    pub completed_tile_count: u64,
    pub required_tile_count: u64,
    pub completed_tile_size: u64,
    pub required_resource_count: u64,
    pub required_resource_count_is_precise: bool,
    pub complete: bool,
}

impl OfflineRegionStatus {
    pub(crate) fn from_native(raw: sys::mln_offline_region_status) -> Self {
        Self {
            download_state: OfflineRegionDownloadState::from_raw(raw.download_state),
            completed_resource_count: raw.completed_resource_count,
            completed_resource_size: raw.completed_resource_size,
            completed_tile_count: raw.completed_tile_count,
            required_tile_count: raw.required_tile_count,
            completed_tile_size: raw.completed_tile_size,
            required_resource_count: raw.required_resource_count,
            required_resource_count_is_precise: raw.required_resource_count_is_precise,
            complete: raw.complete,
        }
    }
}

/// Offline region status-change event payload.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct OfflineRegionStatusEvent {
    pub region_id: i64,
    pub status: OfflineRegionStatus,
}

/// Offline region response-error event payload.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct OfflineRegionResponseErrorEvent {
    pub region_id: i64,
    pub reason: ResourceErrorReason,
}

/// Offline region tile-count-limit event payload.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct OfflineRegionTileCountLimitEvent {
    pub region_id: i64,
    pub limit: u64,
}

/// Style-image-missing event payload.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct StyleImageMissingEvent {
    pub image_id: String,
}

/// Unknown event payload preserved for forward compatibility.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct UnknownRuntimeEventPayload {
    pub raw_type: u32,
    pub bytes: Vec<u8>,
}

/// Owned event payload copied from runtime-owned native storage.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum RuntimeEventPayload {
    None,
    RenderFrame(RenderFrameEvent),
    RenderMap(RenderMapEvent),
    StyleImageMissing(StyleImageMissingEvent),
    TileAction(TileActionEvent),
    OfflineRegionStatus(OfflineRegionStatusEvent),
    OfflineRegionResponseError(OfflineRegionResponseErrorEvent),
    OfflineRegionTileCountLimit(OfflineRegionTileCountLimitEvent),
    Unknown(UnknownRuntimeEventPayload),
}

/// Owned runtime event copied from native poll storage.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct RuntimeEvent {
    pub event_type: RuntimeEventType,
    pub source: RuntimeEventSource,
    pub code: i32,
    pub message: Option<String>,
    pub payload: RuntimeEventPayload,
}

impl RuntimeEvent {
    pub(crate) fn from_native(
        raw: &sys::mln_runtime_event,
        source: RuntimeEventSource,
    ) -> Result<Self> {
        let message = copy_optional_text(raw.message, raw.message_size)?;
        let payload = copy_payload(raw)?;
        Ok(Self {
            event_type: RuntimeEventType::from_raw(raw.type_),
            source,
            code: raw.code,
            message,
            payload,
        })
    }
}

pub(crate) fn empty_runtime_event() -> sys::mln_runtime_event {
    sys::mln_runtime_event {
        size: mem::size_of::<sys::mln_runtime_event>() as u32,
        type_: 0,
        source_type: sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
        source: std::ptr::null_mut(),
        code: 0,
        payload_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE,
        payload: std::ptr::null(),
        payload_size: 0,
        message: std::ptr::null(),
        message_size: 0,
    }
}

fn copy_payload(raw: &sys::mln_runtime_event) -> Result<RuntimeEventPayload> {
    match raw.payload_type {
        sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE => Ok(RuntimeEventPayload::None),
        sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => {
            let payload = copy_payload_struct::<sys::mln_runtime_event_render_frame>(raw)?;
            Ok(RuntimeEventPayload::RenderFrame(RenderFrameEvent {
                mode: RenderMode::from_raw(payload.mode),
                needs_repaint: payload.needs_repaint,
                placement_changed: payload.placement_changed,
                stats: RenderingStats::from_native(payload.stats),
            }))
        }
        sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => {
            let payload = copy_payload_struct::<sys::mln_runtime_event_render_map>(raw)?;
            Ok(RuntimeEventPayload::RenderMap(RenderMapEvent {
                mode: RenderMode::from_raw(payload.mode),
            }))
        }
        sys::MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING => {
            let payload = copy_payload_struct::<sys::mln_runtime_event_style_image_missing>(raw)?;
            Ok(RuntimeEventPayload::StyleImageMissing(
                StyleImageMissingEvent {
                    image_id: copy_required_text(payload.image_id, payload.image_id_size)?,
                },
            ))
        }
        sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => {
            let payload = copy_payload_struct::<sys::mln_runtime_event_tile_action>(raw)?;
            Ok(RuntimeEventPayload::TileAction(TileActionEvent {
                operation: TileOperation::from_raw(payload.operation),
                tile_id: TileId::from_native(payload.tile_id),
                source_id: copy_required_text(payload.source_id, payload.source_id_size)?,
            }))
        }
        sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS => {
            let payload = copy_payload_struct::<sys::mln_runtime_event_offline_region_status>(raw)?;
            Ok(RuntimeEventPayload::OfflineRegionStatus(
                OfflineRegionStatusEvent {
                    region_id: payload.region_id,
                    status: OfflineRegionStatus::from_native(payload.status),
                },
            ))
        }
        sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR => {
            let payload =
                copy_payload_struct::<sys::mln_runtime_event_offline_region_response_error>(raw)?;
            Ok(RuntimeEventPayload::OfflineRegionResponseError(
                OfflineRegionResponseErrorEvent {
                    region_id: payload.region_id,
                    reason: ResourceErrorReason::from_raw(payload.reason),
                },
            ))
        }
        sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT => {
            let payload =
                copy_payload_struct::<sys::mln_runtime_event_offline_region_tile_count_limit>(raw)?;
            Ok(RuntimeEventPayload::OfflineRegionTileCountLimit(
                OfflineRegionTileCountLimitEvent {
                    region_id: payload.region_id,
                    limit: payload.limit,
                },
            ))
        }
        raw_type => Ok(RuntimeEventPayload::Unknown(UnknownRuntimeEventPayload {
            raw_type,
            bytes: copy_payload_bytes(raw)?,
        })),
    }
}

fn copy_payload_struct<T>(raw: &sys::mln_runtime_event) -> Result<T> {
    if raw.payload.is_null() || raw.payload_size < mem::size_of::<T>() {
        return Err(Error::invalid_argument(
            "runtime event payload pointer and size do not match payload type",
        ));
    }

    let mut value = mem::MaybeUninit::<T>::uninit();
    // SAFETY: value points to size_of::<T>() writable bytes. The native event
    // payload is valid for payload_size bytes until the next poll. Copying bytes
    // avoids alignment assumptions about runtime-owned payload storage.
    unsafe {
        ptr::copy_nonoverlapping(
            raw.payload.cast::<u8>(),
            value.as_mut_ptr().cast::<u8>(),
            mem::size_of::<T>(),
        );
        Ok(value.assume_init())
    }
}

fn copy_payload_bytes(raw: &sys::mln_runtime_event) -> Result<Vec<u8>> {
    if raw.payload_size == 0 {
        return Ok(Vec::new());
    }
    if raw.payload.is_null() {
        return Err(Error::invalid_argument(
            "runtime event payload must not be null when payload_size is nonzero",
        ));
    }

    // SAFETY: The C API says payload points to payload_size bytes until the
    // next poll. The caller copies immediately before polling again.
    let bytes = unsafe { std::slice::from_raw_parts(raw.payload.cast::<u8>(), raw.payload_size) };
    Ok(bytes.to_vec())
}

fn copy_optional_text(ptr: *const std::os::raw::c_char, len: usize) -> Result<Option<String>> {
    if len == 0 {
        return Ok(None);
    }
    Ok(Some(copy_required_text(ptr, len)?))
}

fn copy_required_text(ptr: *const std::os::raw::c_char, len: usize) -> Result<String> {
    if len == 0 {
        return Ok(String::new());
    }
    if ptr.is_null() {
        return Err(Error::invalid_argument(
            "runtime event text must not be null when length is nonzero",
        ));
    }

    let view = sys::mln_string_view {
        data: ptr,
        size: len,
    };
    // SAFETY: The C API says event text pointers are valid until the next poll.
    unsafe { support::string::copy_string_view(view) }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn unknown_payload_preserves_raw_type_bytes_and_message() {
        let bytes = [1u8, 2, 3, 4];
        let message = b"future payload";
        let raw = sys::mln_runtime_event {
            size: mem::size_of::<sys::mln_runtime_event>() as u32,
            type_: 999_001,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
            source: ptr::null_mut(),
            code: -7,
            payload_type: 999_002,
            payload: bytes.as_ptr().cast(),
            payload_size: bytes.len(),
            message: message.as_ptr().cast(),
            message_size: message.len(),
        };

        let event = RuntimeEvent::from_native(&raw, RuntimeEventSource::Runtime).unwrap();

        assert_eq!(event.event_type, RuntimeEventType::Unknown(999_001));
        assert_eq!(event.source, RuntimeEventSource::Runtime);
        assert_eq!(event.code, -7);
        assert_eq!(event.message.as_deref(), Some("future payload"));
        assert_eq!(
            event.payload,
            RuntimeEventPayload::Unknown(UnknownRuntimeEventPayload {
                raw_type: 999_002,
                bytes: bytes.to_vec(),
            })
        );
    }

    #[test]
    fn event_copy_survives_backing_buffer_mutation() {
        let mut bytes = [1u8, 2, 3, 4];
        let mut message = b"future payload".to_vec();
        let raw = sys::mln_runtime_event {
            size: mem::size_of::<sys::mln_runtime_event>() as u32,
            type_: 999_001,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
            source: ptr::null_mut(),
            code: -7,
            payload_type: 999_002,
            payload: bytes.as_ptr().cast(),
            payload_size: bytes.len(),
            message: message.as_ptr().cast(),
            message_size: message.len(),
        };

        let event = RuntimeEvent::from_native(&raw, RuntimeEventSource::Runtime).unwrap();
        bytes.fill(9);
        message.fill(b'x');

        assert_eq!(event.message.as_deref(), Some("future payload"));
        assert_eq!(
            event.payload,
            RuntimeEventPayload::Unknown(UnknownRuntimeEventPayload {
                raw_type: 999_002,
                bytes: vec![1, 2, 3, 4],
            })
        );
    }

    #[test]
    fn typed_payload_copies_nested_text() {
        let image_id = b"missing-image";
        let payload = sys::mln_runtime_event_style_image_missing {
            size: mem::size_of::<sys::mln_runtime_event_style_image_missing>() as u32,
            image_id: image_id.as_ptr().cast(),
            image_id_size: image_id.len(),
        };
        let raw = sys::mln_runtime_event {
            size: mem::size_of::<sys::mln_runtime_event>() as u32,
            type_: sys::MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_MAP,
            source: ptr::null_mut(),
            code: 0,
            payload_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING,
            payload: ptr::addr_of!(payload).cast(),
            payload_size: mem::size_of_val(&payload),
            message: image_id.as_ptr().cast(),
            message_size: image_id.len(),
        };

        let event = RuntimeEvent::from_native(&raw, RuntimeEventSource::UnknownMap).unwrap();

        assert_eq!(event.message.as_deref(), Some("missing-image"));
        assert_eq!(
            event.payload,
            RuntimeEventPayload::StyleImageMissing(StyleImageMissingEvent {
                image_id: "missing-image".to_owned(),
            })
        );
    }
}
