use maplibre_native_core as maplibre_core;
use maplibre_native_sys as sys;

use crate::Result;
pub use maplibre_core::events::{
    OfflineOperationCompletedEvent, OfflineRegionResponseErrorEvent, OfflineRegionStatus,
    OfflineRegionStatusEvent, OfflineRegionTileCountLimitEvent, RenderFrameEvent, RenderMapEvent,
    RenderingStats, RuntimeEventPayload, StyleImageMissingEvent, TileActionEvent, TileId,
    UnknownRuntimeEventPayload,
};
pub(crate) use maplibre_core::{OfflineRegionDownloadState, RuntimeEventType};

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
        // SAFETY: raw is borrowed from the latest runtime poll result and is
        // copied before another poll can invalidate event-owned storage.
        let copied = unsafe { maplibre_core::events::runtime_event_from_native(raw) }?;
        Ok(Self {
            event_type: copied.event_type,
            source,
            code: copied.code,
            message: copied.message,
            payload: copied.payload,
        })
    }
}

pub(crate) fn empty_runtime_event() -> sys::mln_runtime_event {
    maplibre_core::events::empty_runtime_event()
}

#[cfg(test)]
mod tests {
    use std::mem;
    use std::ptr;

    use crate::ResourceErrorReason;

    use super::*;

    #[test]
    // Spec coverage: BND-086.
    fn public_runtime_event_applies_rust_source_policy_to_copied_event() {
        let bytes = [1u8, 2, 3, 4];
        let message = b"future payload";
        let source = RuntimeEventSource::Map(MapId::new(42));
        let raw = sys::mln_runtime_event {
            size: mem::size_of::<sys::mln_runtime_event>() as u32,
            type_: 999_001,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_MAP,
            source: ptr::null_mut(),
            code: -7,
            payload_type: 999_002,
            payload: bytes.as_ptr().cast(),
            payload_size: bytes.len(),
            message: message.as_ptr().cast(),
            message_size: message.len(),
        };

        let event = RuntimeEvent::from_native(&raw, source).unwrap();

        assert_eq!(event.event_type, RuntimeEventType::Unknown(999_001));
        assert_eq!(event.source, source);
        assert_eq!(event.code, -7);
        assert_eq!(event.message.as_deref(), Some("future payload"));
        let RuntimeEventPayload::Unknown(payload) = event.payload else {
            panic!("expected unknown payload");
        };
        assert_eq!(payload.raw_type, 999_002);
        assert_eq!(payload.bytes, bytes.to_vec());
    }

    #[test]
    // Spec coverage: BND-085.
    fn public_runtime_event_copies_offline_region_status_and_error_payloads() {
        let mut status = maplibre_core::events::empty_offline_region_status_native();
        status.download_state = sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE;
        status.completed_resource_count = 3;
        status.complete = true;
        let status_payload = sys::mln_runtime_event_offline_region_status {
            size: mem::size_of::<sys::mln_runtime_event_offline_region_status>() as u32,
            region_id: 7,
            status,
        };
        let raw_status = sys::mln_runtime_event {
            size: mem::size_of::<sys::mln_runtime_event>() as u32,
            type_: sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
            source: ptr::null_mut(),
            code: 0,
            payload_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS,
            payload: ptr::addr_of!(status_payload).cast(),
            payload_size: mem::size_of_val(&status_payload),
            message: ptr::null(),
            message_size: 0,
        };

        let event = RuntimeEvent::from_native(&raw_status, RuntimeEventSource::Runtime).unwrap();

        assert_eq!(
            event.event_type,
            RuntimeEventType::OfflineRegionStatusChanged
        );
        let RuntimeEventPayload::OfflineRegionStatus(status_event) = event.payload else {
            panic!("expected offline region status payload");
        };
        assert_eq!(status_event.region_id, 7);
        assert_eq!(
            status_event.status.download_state,
            OfflineRegionDownloadState::Active
        );
        assert_eq!(status_event.status.completed_resource_count, 3);
        assert!(status_event.status.complete);

        let mut message = b"offline failed".to_vec();
        let error_payload = sys::mln_runtime_event_offline_region_response_error {
            size: mem::size_of::<sys::mln_runtime_event_offline_region_response_error>() as u32,
            region_id: 7,
            reason: sys::MLN_RESOURCE_ERROR_REASON_OTHER,
        };
        let raw_error = sys::mln_runtime_event {
            size: mem::size_of::<sys::mln_runtime_event>() as u32,
            type_: sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR,
            source_type: sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
            source: ptr::null_mut(),
            code: -1,
            payload_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR,
            payload: ptr::addr_of!(error_payload).cast(),
            payload_size: mem::size_of_val(&error_payload),
            message: message.as_ptr().cast(),
            message_size: message.len(),
        };

        let event = RuntimeEvent::from_native(&raw_error, RuntimeEventSource::Runtime).unwrap();
        message.fill(b'x');

        assert_eq!(
            event.event_type,
            RuntimeEventType::OfflineRegionResponseError
        );
        assert_eq!(event.message.as_deref(), Some("offline failed"));
        let RuntimeEventPayload::OfflineRegionResponseError(error_event) = event.payload else {
            panic!("expected offline region response-error payload");
        };
        assert_eq!(error_event.region_id, 7);
        assert_eq!(error_event.reason, ResourceErrorReason::Other);
    }
}
