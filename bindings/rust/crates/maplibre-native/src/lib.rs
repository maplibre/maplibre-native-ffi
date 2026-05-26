//! Safe Rust binding for the MapLibre Native C API.
//!
//! This crate owns Rust-specific ergonomics and safety policy: thread-affine
//! public handles, parent retention, owner-thread `Drop`, Rust errors,
//! callback closure APIs, and lifetime-scoped render resources. Shared C ABI
//! adaptation lives in `maplibre-native-core`.

#![deny(unsafe_op_in_unsafe_fn)]

mod camera;
mod custom_geometry;
mod events;
mod geojson;
mod geometry;
mod handle;
mod json;
mod logging;
mod map;
mod options;
mod projection;
mod render;
mod resource;
mod runtime;
mod values;

use crate::values::NativeValue;
use maplibre_native_core as maplibre_core;
use maplibre_native_sys as sys;

pub use camera::{
    AnimationOptions, BoundOptions, CameraFitOptions, CameraOptions, FreeCameraOptions,
    ProjectionMode,
};
pub use custom_geometry::{CanonicalTileId, CustomGeometrySourceOptions};
pub use events::{
    MapId, OfflineOperationCompletedEvent, OfflineRegionResponseErrorEvent, OfflineRegionStatus,
    OfflineRegionStatusEvent, OfflineRegionTileCountLimitEvent, RenderFrameEvent, RenderMapEvent,
    RenderingStats, RuntimeEvent, RuntimeEventPayload, RuntimeEventSource, StyleImageMissingEvent,
    TileActionEvent, TileId, UnknownRuntimeEventPayload,
};
pub use geojson::{Feature, FeatureIdentifier, GeoJson};
pub use geometry::Geometry;
pub use json::{JsonMember, JsonValue};
pub use logging::{
    LogRecord, clear_log_callback, restore_default_async_log_severity_mask,
    set_async_log_severity_mask, set_log_callback,
};
pub use map::{
    LocationIndicatorImageKind, MapHandle, RasterDemEncoding, SourceInfo, SourceType, StyleImage,
    StyleImageInfo, StyleImageOptions, TileScheme, TileSourceOptions, VectorTileEncoding,
};
pub use maplibre_core::{
    AmbientCacheOperation, ConstrainMode, Error, ErrorKind, LogEvent, LogSeverity, LogSeverityMask,
    MapDebugOptions, MapMode, MapOptions, MapTileOptions, MapViewportOptions, NetworkStatus,
    NorthOrientation, OfflineOperationKind, OfflineOperationResultKind, OfflineRegionDownloadState,
    OpenGLContextProviderMask, RenderBackendMask, RenderMode, ResourceErrorReason, ResourceKind,
    ResourceLoadingMethod, ResourcePriority, ResourceResponseStatus, ResourceStoragePolicy,
    ResourceUsage, Result, RuntimeEventType, TileLodMode, TileOperation, ViewportMode,
};
pub use projection::MapProjectionHandle;
pub use render::{
    DetachedRenderSessionHandle, EglContextDescriptor, FeatureExtensionResult,
    FeatureStateSelector, FrameNativePointer, FrameOpenGLTextureName,
    MetalBorrowedTextureDescriptor, MetalContextDescriptor, MetalOwnedTextureDescriptor,
    MetalOwnedTextureFrame, MetalOwnedTextureFrameHandle, MetalSurfaceDescriptor, NativePointer,
    OpenGLBorrowedTextureDescriptor, OpenGLContextDescriptor, OpenGLOwnedTextureDescriptor,
    OpenGLOwnedTextureFrame, OpenGLOwnedTextureFrameHandle, OpenGLSurfaceDescriptor,
    PremultipliedRgba8Image, QueriedFeature, RenderSessionHandle, RenderTargetExtent,
    RenderedFeatureQueryOptions, RenderedQueryGeometry, SourceFeatureQueryOptions,
    TextureImageInfo, VulkanBorrowedTextureDescriptor, VulkanContextDescriptor,
    VulkanOwnedTextureDescriptor, VulkanOwnedTextureFrame, VulkanOwnedTextureFrameHandle,
    VulkanSurfaceDescriptor, WglContextDescriptor,
};
pub use resource::{
    ByteRange, ResourceProviderDecision, ResourceRequest, ResourceRequestHandle, ResourceResponse,
    ResourceTransformRequest,
};
pub use runtime::{
    OfflineOperationHandle, OfflineRegionDefinition, OfflineRegionInfo, RuntimeHandle,
    RuntimeOptions,
};
pub use values::{
    EdgeInsets, LatLng, LatLngBounds, ProjectedMeters, Quaternion, ScreenBox, ScreenPoint,
    UnitBezier, Vec3,
};

/// Error returned by consuming one-shot handle operations when the handle
/// remains live and the operation can be retried.
#[derive(Debug)]
pub struct HandleOperationError<T> {
    error: Error,
    handle: T,
}

impl<T> HandleOperationError<T> {
    pub(crate) fn new(error: Error, handle: T) -> Self {
        Self { error, handle }
    }

    /// Returns the operation error.
    pub fn error(&self) -> &Error {
        &self.error
    }

    /// Returns the stable category for the operation error.
    pub fn kind(&self) -> ErrorKind {
        self.error.kind()
    }

    /// Returns the raw C status for native operation errors, when available.
    pub fn raw_status(&self) -> Option<sys::mln_status> {
        self.error.raw_status()
    }

    /// Returns the copied diagnostic message for the operation error.
    pub fn diagnostic(&self) -> &str {
        self.error.diagnostic()
    }

    /// Returns the operation error, dropping the still-live handle.
    pub fn into_error(self) -> Error {
        self.error
    }

    /// Returns the still-live handle so the operation can be retried.
    pub fn into_handle(self) -> T {
        self.handle
    }

    /// Splits this error into the operation error and still-live handle.
    pub fn into_parts(self) -> (Error, T) {
        (self.error, self.handle)
    }
}

impl<T> std::fmt::Display for HandleOperationError<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.error.fmt(f)
    }
}

impl<T: std::fmt::Debug> std::error::Error for HandleOperationError<T> {}

/// Returns the native C ABI contract version.
pub fn c_version() -> u32 {
    // SAFETY: mln_c_version takes no arguments and returns the process-global C
    // ABI version for the linked native library.
    unsafe { sys::mln_c_version() }
}

/// Returns the render backends compiled into the linked native library.
pub fn supported_render_backends() -> RenderBackendMask {
    // SAFETY: mln_supported_render_backend_mask takes no arguments and returns a
    // value mask. Unknown future bits are preserved by from_bits_retain.
    let mask = unsafe { sys::mln_supported_render_backend_mask() };
    RenderBackendMask::from_bits_retain(mask)
}

/// Returns the OpenGL context providers compiled into the linked native library.
pub fn supported_opengl_context_providers() -> OpenGLContextProviderMask {
    // SAFETY: mln_opengl_supported_context_provider_mask takes no arguments and
    // returns a value mask. Unknown future bits are preserved by from_bits_retain.
    let mask = unsafe { sys::mln_opengl_supported_context_provider_mask() };
    OpenGLContextProviderMask::from_bits_retain(mask)
}

/// Converts a geographic coordinate to Spherical Mercator projected meters.
pub fn projected_meters_for_lat_lng(coordinate: LatLng) -> Result<ProjectedMeters> {
    let mut raw_meters = sys::mln_projected_meters {
        northing: 0.0,
        easting: 0.0,
    };
    // SAFETY: coordinate is passed by value. out_meters points to valid
    // writable storage for one projected-meter value.
    maplibre_core::check(unsafe {
        sys::mln_projected_meters_for_lat_lng(coordinate.to_native(), &mut raw_meters)
    })?;
    Ok(ProjectedMeters::from_native(raw_meters))
}

/// Converts Spherical Mercator projected meters to a geographic coordinate.
pub fn lat_lng_for_projected_meters(meters: ProjectedMeters) -> Result<LatLng> {
    let mut raw_coordinate = sys::mln_lat_lng {
        latitude: 0.0,
        longitude: 0.0,
    };
    // SAFETY: meters is passed by value. out_coordinate points to valid
    // writable storage for one coordinate value.
    maplibre_core::check(unsafe {
        sys::mln_lat_lng_for_projected_meters(meters.to_native(), &mut raw_coordinate)
    })?;
    Ok(LatLng::from_native(raw_coordinate))
}

/// Reads MapLibre Native's process-global network status.
pub fn network_status() -> Result<NetworkStatus> {
    let mut raw_status = 0;
    // SAFETY: out_status points to valid writable storage for one u32.
    maplibre_core::check(unsafe { sys::mln_network_status_get(&mut raw_status) })?;
    Ok(NetworkStatus::from_raw(raw_status))
}

/// Sets MapLibre Native's process-global network status.
pub fn set_network_status(status: NetworkStatus) -> Result<()> {
    set_network_status_raw(status.raw()?)
}

fn set_network_status_raw(raw_status: u32) -> Result<()> {
    // SAFETY: The raw value is passed by value. The C API validates the enum
    // domain and reports invalid values as MLN_STATUS_INVALID_ARGUMENT.
    maplibre_core::check(unsafe { sys::mln_network_status_set(raw_status) })
}

#[cfg(test)]
mod tests {
    use static_assertions::assert_not_impl_any;

    use super::*;

    assert_not_impl_any!(RuntimeHandle: Send, Sync);
    assert_not_impl_any!(MapHandle: Send, Sync);
    assert_not_impl_any!(MapProjectionHandle: Send, Sync);
    assert_not_impl_any!(NativePointer: Send, Sync);
    assert_not_impl_any!(FrameNativePointer<'static>: Send, Sync);
    assert_not_impl_any!(RenderSessionHandle: Send, Sync);

    #[test]
    fn projected_meter_helpers_round_trip() {
        let coordinate = LatLng::new(45.0, -122.0);
        let meters = projected_meters_for_lat_lng(coordinate).unwrap();
        let round_tripped = lat_lng_for_projected_meters(meters).unwrap();

        assert!((round_tripped.latitude - coordinate.latitude).abs() < 1e-9);
        assert!((round_tripped.longitude - coordinate.longitude).abs() < 1e-9);
    }

    #[test]
    fn invalid_network_status_reports_public_error() {
        let error = set_network_status_raw(999_999).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert!(error.diagnostic().contains("network status"));
    }

    #[test]
    fn unknown_network_status_is_rejected_before_calling_c() {
        let error = set_network_status(NetworkStatus::Unknown(999_999)).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);
        assert!(error.diagnostic().contains("cannot be set"));
    }
}
