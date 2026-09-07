use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::Result;
pub use maplibre_core::events::{
    CameraTransitionFinishedEvent, CommandDisposition, OfflineRegionResponseErrorEvent,
    OfflineRegionStatus, OfflineRegionStatusEvent, OfflineRegionTileCountLimitEvent,
    RenderFrameEvent, RenderMapEvent, RenderingStats, RuntimeEventPayload, TileActionEvent, TileId,
    UnknownRuntimeEventPayload,
};
pub(crate) use maplibre_core::{OfflineRegionDownloadState, RuntimeEventType};

/// Identity for a map owned by a runtime. The value is the map's native handle,
/// which names one map for the life of the process. It carries no ownership;
/// map operations go through [`MapHandle`](crate::MapHandle).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct MapId(u64);

impl MapId {
    pub(crate) const fn new(value: u64) -> Self {
        Self(value)
    }

    /// Returns the numeric map identity.
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
    /// Source kind this version does not name, with the native source identity
    /// the event carried.
    Unknown {
        source_type: u32,
        source: u64,
    },
}

impl RuntimeEventSource {
    pub(crate) fn from_raw(source_type: u32, source: u64) -> Self {
        match source_type {
            sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME => Self::Runtime,
            // The copied id grants nothing, so it is reported whether or not
            // this runtime still holds a wrapper for that map.
            sys::MLN_RUNTIME_EVENT_SOURCE_MAP if source != 0 => Self::Map(MapId::new(source)),
            sys::MLN_RUNTIME_EVENT_SOURCE_MAP => Self::UnknownMap,
            source_type => Self::Unknown {
                source_type,
                source,
            },
        }
    }
}

/// Owned runtime event copied out of a drained batch.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct RuntimeEvent {
    pub event_type: RuntimeEventType,
    pub source: RuntimeEventSource,
    /// Secondary event detail whose meaning `event_type` selects. Camera
    /// change events decode as `CameraChangeMode::from_raw(code as u32)`, and
    /// map loading-failure events carry a load error ordinal whose text is in
    /// `message`.
    pub code: i32,
    pub message: Option<String>,
    pub payload: RuntimeEventPayload,
}

impl RuntimeEvent {
    /// Copies one event out of the storage a drained batch owns.
    ///
    /// # Safety
    ///
    /// `view` must come from a batch that `mln_runtime_drain_events` filled,
    /// and that batch must be live for this call.
    pub(crate) unsafe fn from_view(view: &maplibre_core::NativeEventView<'_>) -> Result<Self> {
        // SAFETY: The caller promised the view reads live batch storage.
        let copied = unsafe { maplibre_core::events::copied_event_from_view(view) }?;
        Ok(Self {
            event_type: copied.event_type,
            source: RuntimeEventSource::from_raw(
                copied.source.source_type,
                copied.source.source_id,
            ),
            code: copied.code,
            message: copied.message,
            payload: copied.payload,
        })
    }
}

/// Copies every event of an owned native batch and releases the batch.
///
/// # Safety
///
/// `handle` must be an owned batch returned by `mln_runtime_drain_events`.
pub(crate) unsafe fn copy_event_batch(handle: sys::mln_event_batch) -> Result<Vec<RuntimeEvent>> {
    // SAFETY: handle is live until the release below.
    let copied = unsafe { copy_events(handle) };
    // SAFETY: This function owns the handle on every path.
    unsafe { sys::mln_event_batch_release(handle) };
    copied
}

/// Copies the events a live batch holds.
///
/// # Safety
///
/// `handle` must be live for this call.
unsafe fn copy_events(handle: sys::mln_event_batch) -> Result<Vec<RuntimeEvent>> {
    let mut raw = sys::mln_runtime_event_batch_view {
        size: std::mem::size_of::<sys::mln_runtime_event_batch_view>() as u32,
        event_size: 0,
        events: std::ptr::null(),
        event_count: 0,
        messages: std::ptr::null(),
        messages_size: 0,
    };
    // SAFETY: handle is live and raw is writable for this ABI version.
    maplibre_core::check(unsafe { sys::mln_event_batch_get(handle, &mut raw) })?;
    let mut events = Vec::with_capacity(raw.event_count);
    for index in 0..raw.event_count {
        // SAFETY: the batch is live for this call and index names one of its
        // events.
        let view = unsafe { maplibre_core::events::event_view(&raw, index) };
        // SAFETY: the view reads the live batch's storage.
        events.push(unsafe { RuntimeEvent::from_view(&view) }?);
    }
    Ok(events)
}

/// Batch a test fills itself, so batch decoding is exercised without a live
/// runtime.
#[cfg(test)]
pub(crate) struct SynthesizedBatch {
    records: Vec<u8>,
    messages: Vec<u8>,
    stride: usize,
    count: usize,
}

#[cfg(test)]
impl SynthesizedBatch {
    pub(crate) fn new() -> Self {
        Self {
            records: Vec::new(),
            messages: Vec::new(),
            stride: std::mem::size_of::<sys::mln_runtime_event>(),
            count: 0,
        }
    }

    /// Returns a zeroed event record, which is what the C API queues before it
    /// fills the fields an event type uses.
    pub(crate) fn zeroed_event(event_type: u32) -> sys::mln_runtime_event {
        // SAFETY: Every member of an event record is plain data.
        let mut event = unsafe { std::mem::zeroed::<sys::mln_runtime_event>() };
        event.type_ = event_type;
        event
    }

    pub(crate) fn push(&mut self, mut event: sys::mln_runtime_event, message: &[u8]) {
        event.message_offset = u64::try_from(self.messages.len()).unwrap();
        event.message_size = u32::try_from(message.len()).unwrap();
        self.messages.extend_from_slice(message);
        self.messages.push(0);

        let record = self.records.len();
        self.records.resize(record + self.stride, 0);
        // SAFETY: event is a live local of exactly this many plain-data bytes.
        let bytes = unsafe {
            std::slice::from_raw_parts(
                std::ptr::addr_of!(event).cast::<u8>(),
                std::mem::size_of::<sys::mln_runtime_event>(),
            )
        };
        self.records[record..record + bytes.len()].copy_from_slice(bytes);
        self.count += 1;
    }

    pub(crate) fn raw(&self) -> sys::mln_runtime_event_batch_view {
        sys::mln_runtime_event_batch_view {
            size: std::mem::size_of::<sys::mln_runtime_event_batch_view>() as u32,
            event_size: u32::try_from(self.stride).unwrap(),
            events: self.records.as_ptr().cast(),
            event_count: self.count,
            messages: self.messages.as_ptr().cast(),
            messages_size: self.messages.len(),
        }
    }

    pub(crate) fn iter(&self) -> impl Iterator<Item = Result<RuntimeEvent>> + '_ {
        let raw = self.raw();
        (0..self.count).map(move |index| {
            // SAFETY: This fixture owns the event records and message arena for
            // the iterator's lifetime, and index names one of its records.
            let view = unsafe { maplibre_core::events::event_view(&raw, index) };
            // SAFETY: The view reads storage this fixture keeps live.
            unsafe { RuntimeEvent::from_view(&view) }
        })
    }
}

#[cfg(test)]
mod tests {
    use crate::ResourceErrorReason;

    use super::*;

    #[test]
    // Spec coverage: BND-086.
    fn a_batch_applies_the_rust_source_policy_to_every_source_kind() {
        let mut batch = SynthesizedBatch::new();
        let mut runtime_event = SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_IDLE);
        runtime_event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME;
        batch.push(runtime_event, b"");
        let mut map_event = SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED);
        map_event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
        map_event.source = 0x0200_0000_0000_002a;
        batch.push(map_event, b"");
        let mut unknown_map = SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_IDLE);
        unknown_map.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
        batch.push(unknown_map, b"");
        let mut unknown_source = SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_IDLE);
        unknown_source.source_type = 999_003;
        unknown_source.source = 0x0300_0000_0000_0063;
        batch.push(unknown_source, b"");
        let sources = batch
            .iter()
            .map(|event| event.unwrap().source)
            .collect::<Vec<_>>();

        assert_eq!(
            sources,
            vec![
                RuntimeEventSource::Runtime,
                RuntimeEventSource::Map(MapId::new(0x0200_0000_0000_002a)),
                RuntimeEventSource::UnknownMap,
                RuntimeEventSource::Unknown {
                    source_type: 999_003,
                    source: 0x0300_0000_0000_0063,
                },
            ]
        );
    }

    #[test]
    // Spec coverage: BND-085.
    fn offline_events_decode_their_union_payloads_and_messages() {
        let mut status = maplibre_core::events::empty_offline_region_status_native();
        status.download_state = sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE;
        status.completed_resource_count = 3;
        status.complete = true;
        let mut status_event =
            SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED);
        status_event.payload_type = sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS;
        status_event.payload.offline_region_status = sys::mln_runtime_event_offline_region_status {
            region_id: 7,
            status,
        };
        let mut error_event =
            SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR);
        error_event.code = -1;
        error_event.payload_type = sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR;
        error_event.payload.offline_region_response_error =
            sys::mln_runtime_event_offline_region_response_error {
                region_id: 7,
                reason: sys::MLN_RESOURCE_ERROR_REASON_OTHER,
            };
        let mut records = SynthesizedBatch::new();
        records.push(status_event, b"");
        records.push(error_event, b"offline failed");
        let events = records
            .iter()
            .collect::<Result<Vec<_>>>()
            .expect("every synthesized message is text");

        assert_eq!(
            events[0].event_type,
            RuntimeEventType::OfflineRegionStatusChanged
        );
        assert_eq!(events[0].source, RuntimeEventSource::Runtime);
        let RuntimeEventPayload::OfflineRegionStatus(status) = &events[0].payload else {
            panic!("the first event should carry an offline region status payload");
        };
        assert_eq!(status.region_id, 7);
        assert_eq!(
            status.status.download_state,
            OfflineRegionDownloadState::Active
        );
        assert_eq!(status.status.completed_resource_count, 3);
        assert!(status.status.complete);

        assert_eq!(events[1].message.as_deref(), Some("offline failed"));
        assert_eq!(events[1].code, -1);
        let RuntimeEventPayload::OfflineRegionResponseError(error) = &events[1].payload else {
            panic!("the second event should carry a response-error payload");
        };
        assert_eq!(error.region_id, 7);
        assert_eq!(error.reason, ResourceErrorReason::Other);
    }

    #[test]
    // Spec coverage: BND-092.
    fn an_owned_event_copy_survives_the_storage_it_came_from() {
        let mut records = SynthesizedBatch::new();
        let mut event = SynthesizedBatch::zeroed_event(999_001);
        event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
        event.source = 42;
        event.code = -7;
        event.payload_type = 999_002;
        event.payload.camera_transition_finished =
            sys::mln_runtime_event_camera_transition_finished {
                transition_id: 0x0102_0304_0506_0708,
            };
        records.push(event, b"future payload");

        let owned = records.iter().next().unwrap().unwrap();
        // The copy reads these records, so releasing the storage the event was
        // read from leaves only the copy.
        drop(records);

        assert_eq!(owned.event_type, RuntimeEventType::Unknown(999_001));
        assert_eq!(owned.source, RuntimeEventSource::Map(MapId::new(42)));
        assert_eq!(owned.code, -7);
        assert_eq!(owned.message.as_deref(), Some("future payload"));
        let RuntimeEventPayload::Unknown(payload) = &owned.payload else {
            panic!("an undefined payload type should stay opaque");
        };
        assert_eq!(payload.raw_type, 999_002);
        assert_eq!(
            &payload.bytes[..8],
            &0x0102_0304_0506_0708_u64.to_ne_bytes()
        );
    }

    #[test]
    fn a_message_that_is_not_utf8_fails_the_copy_it_belongs_to() {
        let mut records = SynthesizedBatch::new();
        records.push(
            SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_LOADING_FAILED),
            &[0xff, 0xfe],
        );
        records.push(
            SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED),
            b"loaded",
        );
        let events = records.iter().collect::<Vec<_>>();

        let error = events[0].as_ref().unwrap_err();
        assert_eq!(error.kind(), crate::ErrorKind::InvalidArgument);
        assert!(error.diagnostic().contains("not valid UTF-8"));
        assert_eq!(
            events[1].as_ref().unwrap().message.as_deref(),
            Some("loaded")
        );
    }
}
