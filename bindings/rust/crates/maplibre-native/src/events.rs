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

    use super::*;

    #[test]
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
}
