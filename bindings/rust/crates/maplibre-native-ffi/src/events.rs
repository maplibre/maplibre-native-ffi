use std::fmt;
use std::marker::PhantomData;
use std::str;

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::{Error, Result};
pub use maplibre_core::events::{
    CameraTransitionFinishedEvent, OfflineOperationCompletedEvent, OfflineRegionResponseErrorEvent,
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
    /// change events decode as
    /// `CameraChangeMode::from_raw(code as u32)`, offline operation-completion
    /// events carry the operation's native status, and map loading-failure
    /// events carry a load error ordinal whose text is in `message`.
    pub code: i32,
    pub message: Option<String>,
    pub payload: RuntimeEventPayload,
}

/// Batch of runtime events borrowed from runtime-owned storage.
///
/// A batch borrows the [`RuntimeHandle`](crate::RuntimeHandle) it came from, so
/// the next drain is a compile error while the batch lives, and an event read
/// out of a batch borrows the batch. Take [`RuntimeEventRef::to_owned`] for a
/// value that outlives either.
pub struct RuntimeEventBatch<'a> {
    raw: sys::mln_runtime_event_batch,
    _storage: PhantomData<&'a [u8]>,
}

impl<'a> RuntimeEventBatch<'a> {
    /// Reports one drained batch's runtime-owned storage.
    ///
    /// # Safety
    ///
    /// `raw` must be a batch that `mln_runtime_drain_events` filled, whose
    /// event and message storage stays readable for `'a`.
    pub(crate) unsafe fn new(raw: sys::mln_runtime_event_batch) -> Self {
        Self {
            raw,
            _storage: PhantomData,
        }
    }

    /// Returns how many events this batch reports.
    pub fn len(&self) -> usize {
        self.raw.event_count
    }

    /// Reports whether this batch has no events.
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// Returns how many events stayed queued after this batch. A nonzero count
    /// means another drain reports more events.
    pub fn remaining(&self) -> usize {
        self.raw.remaining_count
    }

    /// Walks this batch's events in queue order.
    ///
    /// Each event borrows this batch, so safe code cannot read one after the
    /// batch is gone:
    ///
    /// ```compile_fail,E0505
    /// # use maplibre_native_ffi::{RuntimeHandle, RuntimeOptions};
    /// let mut runtime = RuntimeHandle::with_options(&RuntimeOptions::default()).unwrap();
    /// let batch = runtime.drain_events(0).unwrap();
    /// let events = batch.iter().collect::<Vec<_>>();
    /// drop(batch);
    /// let _ = events.first().map(|event| event.message_bytes());
    /// ```
    pub fn iter(&self) -> impl Iterator<Item = RuntimeEventRef<'_>> {
        (0..self.len()).map(move |index| {
            // SAFETY: This batch's storage stays readable while it is borrowed,
            // and index names one of its events.
            RuntimeEventRef {
                view: unsafe { maplibre_core::events::event_view(&self.raw, index) },
            }
        })
    }
}

impl fmt::Debug for RuntimeEventBatch<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RuntimeEventBatch")
            .field("len", &self.len())
            .field("remaining", &self.remaining())
            .finish()
    }
}

/// One event of a [`RuntimeEventBatch`], read from runtime-owned storage.
#[derive(Clone, Copy)]
pub struct RuntimeEventRef<'a> {
    view: maplibre_core::NativeEventView<'a>,
}

impl<'a> RuntimeEventRef<'a> {
    /// Returns this event's type.
    pub fn event_type(&self) -> RuntimeEventType {
        RuntimeEventType::from_raw(self.view.raw.type_)
    }

    /// Returns the object that emitted this event.
    pub fn source(&self) -> RuntimeEventSource {
        RuntimeEventSource::from_raw(self.view.raw.source_type, self.view.raw.source)
    }

    /// Returns the secondary detail whose meaning the event type selects. See
    /// [`RuntimeEvent::code`].
    pub fn code(&self) -> i32 {
        self.view.raw.code
    }

    /// Decodes the payload union member that this event's payload type names.
    /// A payload type this version does not define keeps its raw value and the
    /// payload's copied byte window.
    pub fn payload(&self) -> RuntimeEventPayload {
        // SAFETY: The view came from a drained batch, so the payload union
        // holds initialized bytes for the member payload_type names.
        unsafe { maplibre_core::events::payload_from_view(&self.view) }
    }

    /// Returns this event's message as text, or `None` when it carries no
    /// message. A message that is not UTF-8 fails on its own event.
    pub fn message(&self) -> Result<Option<&'a str>> {
        if self.view.message.is_empty() {
            return Ok(None);
        }
        str::from_utf8(self.view.message)
            .map(Some)
            .map_err(|error| {
                Error::invalid_argument(format!(
                    "runtime event message was not valid UTF-8: {error}"
                ))
            })
    }

    /// Returns this event's message bytes, which are empty when it carries no
    /// message.
    pub fn message_bytes(&self) -> &'a [u8] {
        self.view.message
    }

    /// Copies this event into a value that outlives the batch.
    pub fn to_owned(&self) -> Result<RuntimeEvent> {
        // SAFETY: The view came from a drained batch whose storage is readable
        // for 'a, so every field copied here is live.
        let copied = unsafe { maplibre_core::events::copied_event_from_view(&self.view) }?;
        Ok(RuntimeEvent {
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

impl fmt::Debug for RuntimeEventRef<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RuntimeEventRef")
            .field("event_type", &self.event_type())
            .field("source", &self.source())
            .field("code", &self.code())
            .field("message_bytes", &self.message_bytes())
            .finish()
    }
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
        event.message_offset = u32::try_from(self.messages.len()).unwrap();
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

    pub(crate) fn raw(&self) -> sys::mln_runtime_event_batch {
        sys::mln_runtime_event_batch {
            size: std::mem::size_of::<sys::mln_runtime_event_batch>() as u32,
            event_size: u32::try_from(self.stride).unwrap(),
            events: self.records.as_ptr().cast(),
            event_count: self.count,
            messages: self.messages.as_ptr().cast(),
            messages_size: self.messages.len(),
            remaining_count: 0,
        }
    }

    pub(crate) fn batch(&self) -> RuntimeEventBatch<'_> {
        // SAFETY: This fixture's records and arena are laid out the way a drain
        // fills them, and they outlive the borrow the batch takes.
        unsafe { RuntimeEventBatch::new(self.raw()) }
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
        let mut runtime_event =
            SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED);
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
        let batch = batch.batch();

        let sources = batch.iter().map(|event| event.source()).collect::<Vec<_>>();

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
        let batch = records.batch();

        let events = batch.iter().collect::<Vec<_>>();

        assert_eq!(
            events[0].event_type(),
            RuntimeEventType::OfflineRegionStatusChanged
        );
        assert_eq!(events[0].source(), RuntimeEventSource::Runtime);
        let RuntimeEventPayload::OfflineRegionStatus(status) = events[0].payload() else {
            panic!("the first event should carry an offline region status payload");
        };
        assert_eq!(status.region_id, 7);
        assert_eq!(
            status.status.download_state,
            OfflineRegionDownloadState::Active
        );
        assert_eq!(status.status.completed_resource_count, 3);
        assert!(status.status.complete);

        assert_eq!(events[1].message().unwrap(), Some("offline failed"));
        assert_eq!(events[1].code(), -1);
        let RuntimeEventPayload::OfflineRegionResponseError(error) = events[1].payload() else {
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
        let batch = records.batch();
        let borrowed = batch.iter().next().unwrap();

        let owned = borrowed.to_owned().unwrap();
        assert_eq!(borrowed.message_bytes(), b"future payload");
        // The batch borrows these records, so releasing the storage the events
        // were read from leaves only the copy.
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
    fn a_message_that_is_not_utf8_fails_only_its_own_event() {
        let mut records = SynthesizedBatch::new();
        records.push(
            SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_LOADING_FAILED),
            &[0xff, 0xfe],
        );
        records.push(
            SynthesizedBatch::zeroed_event(sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED),
            b"loaded",
        );
        let batch = records.batch();
        let events = batch.iter().collect::<Vec<_>>();

        let error = events[0].message().unwrap_err();
        assert_eq!(error.kind(), crate::ErrorKind::InvalidArgument);
        assert!(error.diagnostic().contains("not valid UTF-8"));
        assert_eq!(events[0].message_bytes(), &[0xff, 0xfe]);
        assert!(events[0].to_owned().is_err());
        assert_eq!(events[1].message().unwrap(), Some("loaded"));
        assert!(events[1].to_owned().is_ok());
    }
}
