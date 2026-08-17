use std::marker::PhantomData;
use std::mem;
use std::slice;
use std::str;

use maplibre_native_ffi_sys as sys;

use crate::{Error, Result};
use crate::{
    OfflineRegionDownloadState, RenderMode, ResourceErrorReason, RuntimeEventType, TileOperation,
};

/// Byte offset of the payload union inside a native event record. Every ABI
/// version keeps this offset, so it converts a batch stride into the payload's
/// byte window.
const PAYLOAD_OFFSET: usize = mem::offset_of!(sys::mln_runtime_event, payload);

/// Raw source fields copied from a native runtime event.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct RawRuntimeEventSource {
    pub source_type: u32,
    /// The source handle id, which names one object for the process's life.
    pub source_id: u64,
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

/// Tile-action event payload. The event message carries the source ID.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct TileActionEvent {
    pub operation: TileOperation,
    pub tile_id: TileId,
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
    fn from_native(raw: sys::mln_offline_region_status) -> Self {
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

pub fn offline_region_status_from_native(
    raw: sys::mln_offline_region_status,
) -> OfflineRegionStatus {
    OfflineRegionStatus::from_native(raw)
}

pub fn empty_offline_region_status_native() -> sys::mln_offline_region_status {
    sys::mln_offline_region_status {
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

/// Camera transition-finished event payload.
///
/// A transition reports its end once for every terminal outcome, and the
/// payload names the transition rather than the outcome.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct CameraTransitionFinishedEvent {
    /// The `transition_id` the caller set on the `AnimationOptions` that
    /// started this transition.
    pub transition_id: u64,
}

/// Terminal disposition of an accepted command.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum CommandDisposition {
    Committed,
    Superseded,
    Failed,
    Cancelled,
    Unknown(u32),
}

impl CommandDisposition {
    fn from_native(raw: u32) -> Self {
        match raw {
            sys::MLN_COMMAND_DISPOSITION_COMMITTED => Self::Committed,
            sys::MLN_COMMAND_DISPOSITION_SUPERSEDED => Self::Superseded,
            sys::MLN_COMMAND_DISPOSITION_FAILED => Self::Failed,
            sys::MLN_COMMAND_DISPOSITION_CANCELLED => Self::Cancelled,
            value => Self::Unknown(value),
        }
    }
}

/// Completion payload for one accepted runtime or map command.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct CommandFinishedEvent {
    pub command_id: u64,
    pub disposition: CommandDisposition,
    /// The map snapshot generation the commit published, or zero when the
    /// command committed no generation. A later map snapshot that reports
    /// this generation or a newer one observes the commit.
    pub generation: u64,
}

/// Payload of a type this version does not define, preserved for forward
/// compatibility.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct UnknownRuntimeEventPayload {
    pub raw_type: u32,
    /// The payload union's byte window, copied from the batch. Its length is
    /// the batch's event stride minus the payload's offset in an event record.
    pub bytes: Vec<u8>,
}

/// Event payload decoded from the payload union that every event carries.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum RuntimeEventPayload {
    None,
    RenderFrame(RenderFrameEvent),
    RenderMap(RenderMapEvent),
    TileAction(TileActionEvent),
    OfflineRegionStatus(OfflineRegionStatusEvent),
    OfflineRegionResponseError(OfflineRegionResponseErrorEvent),
    OfflineRegionTileCountLimit(OfflineRegionTileCountLimitEvent),
    CameraTransitionFinished(CameraTransitionFinishedEvent),
    CommandFinished(CommandFinishedEvent),
    Unknown(UnknownRuntimeEventPayload),
}

/// Owned runtime event copied out of a drained batch, with raw source fields.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct CopiedRuntimeEvent {
    pub event_type: RuntimeEventType,
    pub source: RawRuntimeEventSource,
    /// Secondary event detail whose meaning `event_type` selects. Camera
    /// change events decode as
    /// `CameraChangeMode::from_raw(code as u32)`, and map loading-failure
    /// events carry a load error ordinal whose text is in `message`.
    pub code: i32,
    pub message: Option<String>,
    pub payload: RuntimeEventPayload,
}

/// One event located inside a drained batch: the event record plus the arena
/// slices it names.
#[derive(Clone, Copy)]
pub struct NativeEventView<'a> {
    /// The event record, read at the batch's own event stride.
    pub raw: sys::mln_runtime_event,
    /// The event's message bytes, without the arena's null terminator.
    pub message: &'a [u8],
    /// The payload union's byte window: the batch's event stride minus the
    /// payload's offset in an event record.
    pub payload_window: &'a [u8],
}

/// Iterator over the events of one drained batch, in queue order.
pub struct NativeEventViews<'a> {
    batch: sys::mln_runtime_event_batch_view,
    index: usize,
    _storage: PhantomData<&'a [u8]>,
}

impl<'a> Iterator for NativeEventViews<'a> {
    type Item = NativeEventView<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.index >= self.batch.event_count {
            return None;
        }
        // SAFETY: event_views' caller promised the batch describes storage that
        // stays valid for 'a, and index is below event_count.
        let view = unsafe { event_view(&self.batch, self.index) };
        self.index += 1;
        Some(view)
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        let remaining = self.batch.event_count - self.index;
        (remaining, Some(remaining))
    }
}

impl ExactSizeIterator for NativeEventViews<'_> {}

/// Walks the events of one drained batch in queue order.
///
/// # Safety
///
/// `batch` must be a view that `mln_event_batch_get` filled, whose event and
/// message storage stays valid for `'a`.
pub unsafe fn event_views<'a>(batch: &sys::mln_runtime_event_batch_view) -> NativeEventViews<'a> {
    NativeEventViews {
        batch: *batch,
        index: 0,
        _storage: PhantomData,
    }
}

/// Locates one event of a drained batch by its index.
///
/// Events are indexed by the batch's `event_size`, so a stride a later ABI
/// version widens still reads every field this version defines.
///
/// # Safety
///
/// `batch` must be a view that `mln_event_batch_get` filled, whose event and
/// message storage stays valid for `'a`, and `index` must be below
/// `batch.event_count`.
pub unsafe fn event_view<'a>(
    batch: &sys::mln_runtime_event_batch_view,
    index: usize,
) -> NativeEventView<'a> {
    let stride = batch.event_size as usize;
    debug_assert!(
        stride >= mem::size_of::<sys::mln_runtime_event>(),
        "a drained batch reports an event stride below the compiled event size"
    );
    // SAFETY: The caller promised index names an event of this batch, so the
    // record and its payload window lie inside the event array.
    let (raw, payload_window) = unsafe {
        let record = batch.events.cast::<u8>().add(index * stride);
        (
            record.cast::<sys::mln_runtime_event>().read_unaligned(),
            slice::from_raw_parts(
                record.add(PAYLOAD_OFFSET),
                stride.saturating_sub(PAYLOAD_OFFSET),
            ),
        )
    };
    let message = if raw.message_size == 0 {
        &[][..]
    } else {
        // SAFETY: The C API writes message offsets and sizes that lie inside
        // the batch's message arena.
        unsafe {
            slice::from_raw_parts(
                batch.messages.cast::<u8>().add(raw.message_offset as usize),
                raw.message_size as usize,
            )
        }
    };
    NativeEventView {
        raw,
        message,
        payload_window,
    }
}

/// Copies every event of one drained batch into owned Rust data, in queue
/// order. An event whose message is not UTF-8 fails on its own, so the rest of
/// the batch still decodes.
///
/// # Safety
///
/// `batch` must be a view that `mln_event_batch_get` filled, whose event and
/// message storage stays valid for the returned iterator's lifetime.
pub unsafe fn drain_batch<'a>(
    batch: &sys::mln_runtime_event_batch_view,
) -> impl Iterator<Item = Result<CopiedRuntimeEvent>> + 'a {
    // SAFETY: The caller promised the batch describes live storage.
    unsafe { event_views(batch) }.map(|view|
        // SAFETY: The view came from this batch, so its payload window matches
        // the record it was read with.
        unsafe { copied_event_from_view(&view) })
}

/// Copies one event of a drained batch into owned Rust data.
///
/// # Safety
///
/// `view` must come from `event_view` or `event_views` for a batch that
/// `mln_runtime_drain_events` filled.
pub unsafe fn copied_event_from_view(view: &NativeEventView<'_>) -> Result<CopiedRuntimeEvent> {
    let message = message_text(view.message)?;
    // SAFETY: The caller promised the view came from a drained batch, so the
    // payload union holds initialized bytes for the member payload_type names.
    let payload = unsafe { payload_from_view(view) };
    Ok(CopiedRuntimeEvent {
        event_type: RuntimeEventType::from_raw(view.raw.type_),
        source: RawRuntimeEventSource {
            source_type: view.raw.source_type,
            source_id: view.raw.source,
        },
        code: view.raw.code,
        message,
        payload,
    })
}

/// Decodes the payload union member that an event's payload type names.
///
/// # Safety
///
/// `view` must come from `event_view` or `event_views` for a batch that
/// `mln_runtime_drain_events` filled, so the payload union holds initialized
/// bytes and the payload window covers them.
pub unsafe fn payload_from_view(view: &NativeEventView<'_>) -> RuntimeEventPayload {
    let raw = &view.raw;
    // SAFETY: The C API zeroes the payload union of every event it queues and
    // fills the member payload_type names, so every read below is initialized.
    unsafe {
        match raw.payload_type {
            sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE => RuntimeEventPayload::None,
            sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => {
                let payload = raw.payload.render_frame;
                RuntimeEventPayload::RenderFrame(RenderFrameEvent {
                    mode: RenderMode::from_raw(payload.mode),
                    needs_repaint: payload.needs_repaint,
                    placement_changed: payload.placement_changed,
                    stats: RenderingStats::from_native(payload.stats),
                })
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => {
                RuntimeEventPayload::RenderMap(RenderMapEvent {
                    mode: RenderMode::from_raw(raw.payload.render_map.mode),
                })
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => {
                let payload = raw.payload.tile_action;
                RuntimeEventPayload::TileAction(TileActionEvent {
                    operation: TileOperation::from_raw(payload.operation),
                    tile_id: TileId::from_native(payload.tile_id),
                })
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS => {
                let payload = raw.payload.offline_region_status;
                RuntimeEventPayload::OfflineRegionStatus(OfflineRegionStatusEvent {
                    region_id: payload.region_id,
                    status: OfflineRegionStatus::from_native(payload.status),
                })
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR => {
                let payload = raw.payload.offline_region_response_error;
                RuntimeEventPayload::OfflineRegionResponseError(OfflineRegionResponseErrorEvent {
                    region_id: payload.region_id,
                    reason: ResourceErrorReason::from_raw(payload.reason),
                })
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT => {
                let payload = raw.payload.offline_region_tile_count_limit;
                RuntimeEventPayload::OfflineRegionTileCountLimit(OfflineRegionTileCountLimitEvent {
                    region_id: payload.region_id,
                    limit: payload.limit,
                })
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED => {
                RuntimeEventPayload::CameraTransitionFinished(CameraTransitionFinishedEvent {
                    transition_id: raw.payload.camera_transition_finished.transition_id,
                })
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_COMMAND_FINISHED => {
                let payload = raw.payload.command_finished;
                RuntimeEventPayload::CommandFinished(CommandFinishedEvent {
                    command_id: payload.command_id,
                    disposition: CommandDisposition::from_native(payload.disposition),
                    generation: payload.generation,
                })
            }
            raw_type => RuntimeEventPayload::Unknown(UnknownRuntimeEventPayload {
                raw_type,
                bytes: view.payload_window.to_vec(),
            }),
        }
    }
}

/// Validates one event's message bytes as text, so a message that is not UTF-8
/// fails on its own event.
fn message_text(bytes: &[u8]) -> Result<Option<String>> {
    if bytes.is_empty() {
        return Ok(None);
    }
    str::from_utf8(bytes)
        .map(|text| Some(text.to_owned()))
        .map_err(|error| {
            Error::invalid_argument(format!(
                "runtime event message was not valid UTF-8: {error}"
            ))
        })
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Batch a test fills itself, so the decoder's stride and arena arithmetic
    /// is exercised without a live runtime.
    struct SynthesizedBatch {
        records: Vec<u8>,
        messages: Vec<u8>,
        stride: usize,
        count: usize,
    }

    impl SynthesizedBatch {
        fn new(stride: usize) -> Self {
            assert!(stride >= mem::size_of::<sys::mln_runtime_event>());
            Self {
                records: Vec::new(),
                messages: Vec::new(),
                stride,
                count: 0,
            }
        }

        fn push(&mut self, mut event: sys::mln_runtime_event, message: &[u8]) {
            event.message_offset = u64::try_from(self.messages.len()).unwrap();
            event.message_size = u32::try_from(message.len()).unwrap();
            self.messages.extend_from_slice(message);
            self.messages.push(0);

            let record = self.records.len();
            self.records.resize(record + self.stride, 0);
            let bytes = unsafe {
                slice::from_raw_parts(
                    std::ptr::addr_of!(event).cast::<u8>(),
                    mem::size_of::<sys::mln_runtime_event>(),
                )
            };
            self.records[record..record + bytes.len()].copy_from_slice(bytes);
            self.count += 1;
        }

        fn raw(&self) -> sys::mln_runtime_event_batch_view {
            sys::mln_runtime_event_batch_view {
                size: mem::size_of::<sys::mln_runtime_event_batch_view>() as u32,
                event_size: u32::try_from(self.stride).unwrap(),
                events: self.records.as_ptr().cast(),
                event_count: self.count,
                messages: self.messages.as_ptr().cast(),
                messages_size: self.messages.len(),
            }
        }
    }

    fn zeroed_event(event_type: u32) -> sys::mln_runtime_event {
        // SAFETY: Every member of an event record is plain data, and the C API
        // queues events whose payload union is zeroed.
        let mut event = unsafe { mem::zeroed::<sys::mln_runtime_event>() };
        event.type_ = event_type;
        event
    }

    fn render_map_event() -> sys::mln_runtime_event {
        let mut event = zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED);
        event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
        event.source = 0x0200_0000_0000_002a;
        event.payload_type = sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP;
        event.payload.render_map = sys::mln_runtime_event_render_map {
            mode: sys::MLN_RENDER_MODE_FULL,
        };
        event
    }

    #[test]
    // Spec coverage: BND-087.
    fn a_batch_steps_events_by_the_reported_stride() {
        // A stride wider than this header's event size models the next ABI
        // version adding a payload member.
        let stride = mem::size_of::<sys::mln_runtime_event>() + 16;
        let mut batch = SynthesizedBatch::new(stride);
        batch.push(zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED), b"");
        batch.push(render_map_event(), b"");
        let mut tile_action = zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_TILE_ACTION);
        tile_action.payload_type = sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION;
        tile_action.payload.tile_action = sys::mln_runtime_event_tile_action {
            operation: sys::MLN_TILE_OPERATION_END_PARSE,
            tile_id: sys::mln_tile_id {
                overscaled_z: 4,
                wrap: -1,
                canonical_z: 3,
                canonical_x: 2,
                canonical_y: 1,
            },
        };
        batch.push(tile_action, b"tiles");
        let raw = batch.raw();

        let events = unsafe { drain_batch(&raw) }
            .collect::<Result<Vec<_>>>()
            .unwrap();

        assert_eq!(events.len(), 3);
        assert_eq!(events[0].event_type, RuntimeEventType::MapStyleLoaded);
        assert_eq!(
            events[1].payload,
            RuntimeEventPayload::RenderMap(RenderMapEvent {
                mode: RenderMode::Full,
            })
        );
        assert_eq!(
            events[1].source,
            RawRuntimeEventSource {
                source_type: sys::MLN_RUNTIME_EVENT_SOURCE_MAP,
                source_id: 0x0200_0000_0000_002a,
            }
        );
        assert_eq!(events[2].message.as_deref(), Some("tiles"));
        let RuntimeEventPayload::TileAction(tile_action) = &events[2].payload else {
            panic!("the third event should carry a tile-action payload");
        };
        assert_eq!(tile_action.operation, TileOperation::EndParse);
        assert_eq!(tile_action.tile_id.canonical_x, 2);
    }

    #[test]
    // Spec coverage: BND-083.
    fn unknown_domains_preserve_raw_values_and_the_payload_window() {
        let stride = mem::size_of::<sys::mln_runtime_event>() + 8;
        let mut event = zeroed_event(999_001);
        event.source_type = 999_003;
        event.source = 7;
        event.code = -7;
        event.payload_type = 999_002;
        event.payload.camera_transition_finished =
            sys::mln_runtime_event_camera_transition_finished {
                transition_id: 0x0102_0304_0506_0708,
            };
        let mut batch = SynthesizedBatch::new(stride);
        batch.push(event, b"future payload");
        let raw = batch.raw();

        let mut events = unsafe { drain_batch(&raw) };
        let event = events.next().unwrap().unwrap();
        assert!(events.next().is_none());

        assert_eq!(event.event_type, RuntimeEventType::Unknown(999_001));
        assert_eq!(
            event.source,
            RawRuntimeEventSource {
                source_type: 999_003,
                source_id: 7,
            }
        );
        assert_eq!(event.code, -7);
        assert_eq!(event.message.as_deref(), Some("future payload"));
        let RuntimeEventPayload::Unknown(payload) = &event.payload else {
            panic!("an undefined payload type should stay opaque");
        };
        assert_eq!(payload.raw_type, 999_002);
        // The window is the batch stride minus the payload's own offset, so it
        // covers members a later version adds.
        assert_eq!(payload.bytes.len(), stride - PAYLOAD_OFFSET);
        assert_eq!(
            &payload.bytes[..8],
            &0x0102_0304_0506_0708_u64.to_ne_bytes()
        );

        // Spec coverage: BND-092. A copy is independent of the batch storage.
        drop(batch);
        assert_eq!(event.message.as_deref(), Some("future payload"));
        assert_eq!(
            &payload.bytes[..8],
            &0x0102_0304_0506_0708_u64.to_ne_bytes()
        );
    }
}
