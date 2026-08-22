//! Safe Rust binding for the MapLibre Native C API.
//!
//! Runtime and map control handles are any-thread; graphics sessions retain
//! their backend thread affinity. This crate also owns parent retention, Rust
//! errors, callback closures, and render-resource lifetimes. Shared C ABI
//! adaptation lives in `maplibre-native-ffi-core`.

#![deny(unsafe_op_in_unsafe_fn)]

mod camera;
mod completion;
mod custom_geometry;
mod custom_mvt_vector;
mod events;
mod geojson;
mod handle;
mod logging;
mod map;
mod options;
mod projection;
mod render;
mod resource;
mod runtime;
mod values;

use crate::values::NativeValue;
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

pub use camera::{
    AnimationOptions, BoundOptions, BoundsConstraint, CameraDelta, CameraDeltaKind,
    CameraFitOptions, CameraOptions, CameraSnapshot, CameraUpdate, CameraUpdateMode,
    FreeCameraOptions, GesturePhase, ProjectionMode,
};
pub use completion::{CommandCompletion, NativeFuture};
pub use custom_geometry::{CanonicalTileId, CustomGeometrySourceOptions};
pub use custom_mvt_vector::CustomMvtVectorSourceOptions;
pub use events::{
    CameraTransitionFinishedEvent, CommandDisposition, MapId, OfflineRegionResponseErrorEvent,
    OfflineRegionStatus, OfflineRegionStatusEvent, OfflineRegionTileCountLimitEvent,
    RenderFrameEvent, RenderMapEvent, RenderingStats, RuntimeEvent, RuntimeEventBatch,
    RuntimeEventPayload, RuntimeEventRef, RuntimeEventSource, TileActionEvent, TileId,
    UnknownRuntimeEventPayload,
};
pub use geojson::GeoJsonSourceDataHandle;
pub use logging::{LogRecord, clear_log_callback, set_async_log_severity_mask, set_log_callback};
pub use map::{
    GeoJsonSourceOptions, ImageContent, ImageStretch, LocationIndicatorImageKind, LogicalExtent,
    MapHandle, MapSnapshot, SourceInfo, SourceType, StyleImage, StyleImageInfo, StyleImageOptions,
    StyleImageStretches, StyleImageTextFit, StyleLayerInfo, StyleLayerVisibility,
    StyleTransitionOptions, TileJsonInfo, TileScheme, TileSourceOptions, VectorTileEncoding,
};
pub use maplibre_core::{
    AmbientCacheOperation, CameraChangeMode, ConstrainMode, Error, ErrorKind, LogEvent,
    LogSeverity, LogSeverityMask, MapDebugOptions, MapMode, MapOptions, MapTileOptions,
    MapViewportOptions, NetworkStatus, NorthOrientation, OfflineRegionDownloadState,
    OpenGLClientApi, OpenGLContextOwnership, OpenGLContextProviderMask, RasterDemEncoding,
    RenderBackendMask, RenderMode, ResourceErrorReason, ResourceKind, ResourceLoadingMethod,
    ResourcePriority, ResourceResponseStatus, ResourceStoragePolicy, ResourceUsage, Result,
    RuntimeEventMask, RuntimeEventType, TileLodMode, TileOperation, ViewportMode,
};
pub use maplibre_native_ffi_core::handle::{NativeHandleLeak, set_leak_reporter};
pub use projection::MapProjectionHandle;
pub use render::{
    AcquiredFrameHandle, EglContextDescriptor, FeatureStateSelector, FrameDemand, FrameDisposition,
    FrameGpuSync, FrameNativePointer, FrameOpenGLTextureName, GpuSync, GpuSyncKind,
    MetalBorrowedTextureDescriptor, MetalContextDescriptor, MetalOwnedTextureDescriptor,
    MetalOwnedTextureFrame, MetalSurfaceDescriptor, NativePointer, OpenGLBorrowedTextureDescriptor,
    OpenGLContextDescriptor, OpenGLOwnedTextureDescriptor, OpenGLOwnedTextureFrame,
    OpenGLSurfaceDescriptor, PremultipliedRgba8Image, QueriedFeature, RenderAbandonResult,
    RenderDriverKind, RenderFrameBatch, RenderFrameResult, RenderSessionAttachOptions,
    RenderSessionAttachment, RenderSessionCapabilities, RenderSessionHandle,
    RenderSessionLifecycle, RenderSessionSnapshot, RenderTargetExtent, RenderedFeatureQueryOptions,
    RenderedQueryGeometry, SourceFeatureQueryOptions, TextureImageInfo,
    VulkanBorrowedTextureDescriptor, VulkanContextDescriptor, VulkanOwnedTextureDescriptor,
    VulkanOwnedTextureFrame, VulkanSurfaceDescriptor, WebGlContextDescriptor,
    WebGpuBorrowedTextureDescriptor, WebGpuContextDescriptor, WebGpuOwnedTextureDescriptor,
    WebGpuOwnedTextureFrame, WebGpuSurfaceDescriptor, WglContextDescriptor,
};
pub use resource::{
    ByteRange, HttpHeader, HttpHeaderTransformRequest, ResourceProviderDecision, ResourceRequest,
    ResourceRequestHandle, ResourceResponse, ResourceTransformRequest,
};
pub use runtime::{OfflineRegionDefinition, OfflineRegionInfo, RuntimeHandle, RuntimeOptions};
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
    pub fn raw_status(&self) -> Option<i32> {
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
    // SAFETY: mln_supported_render_backend_mask takes no arguments and returns
    // a value mask.
    let mask = unsafe { sys::mln_supported_render_backend_mask() };
    RenderBackendMask::from_bits_retain(mask)
}

/// Returns the OpenGL context providers compiled into the linked native library.
pub fn supported_opengl_context_providers() -> OpenGLContextProviderMask {
    // SAFETY: mln_opengl_supported_context_provider_mask takes no arguments and
    // returns a value mask.
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
    maplibre_core::network_status()
}

/// Sets MapLibre Native's process-global network status.
pub fn set_network_status(status: NetworkStatus) -> Result<()> {
    maplibre_core::set_network_status(status)
}

#[cfg(test)]
fn set_network_status_raw(raw_status: u32) -> Result<()> {
    maplibre_core::set_network_status_raw(raw_status)
}

#[cfg(test)]
mod tests {
    use static_assertions::{assert_impl_all, assert_not_impl_any};

    use super::*;

    assert_impl_all!(RuntimeHandle: Send, Sync);
    assert_impl_all!(MapHandle: Send, Sync);
    assert_impl_all!(MapProjectionHandle: Send, Sync);
    assert_impl_all!(RenderSessionHandle: Send, Sync);
    assert_impl_all!(AcquiredFrameHandle: Send, Sync);
    assert_not_impl_any!(NativePointer: Send, Sync);
    assert_not_impl_any!(FrameNativePointer<'static>: Send, Sync);
    assert_not_impl_any!(FrameOpenGLTextureName<'static>: Send, Sync);
    assert_not_impl_any!(GpuSync: Send, Sync);
    assert_not_impl_any!(FrameGpuSync<'static>: Send, Sync);

    #[test]
    // Spec coverage: BND-103.
    fn projected_meter_helpers_round_trip() {
        let coordinate = LatLng::new(45.0, -122.0);
        let meters = projected_meters_for_lat_lng(coordinate).unwrap();
        let round_tripped = lat_lng_for_projected_meters(meters).unwrap();

        assert!((round_tripped.latitude - coordinate.latitude).abs() < 1e-9);
        assert!((round_tripped.longitude - coordinate.longitude).abs() < 1e-9);
    }

    #[test]
    // Spec coverage: BND-020.
    fn invalid_network_status_reports_public_error() {
        let error = set_network_status_raw(999_999).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert!(error.diagnostic().contains("network status"));
    }

    #[test]
    // Spec coverage: BND-025 and BND-068.
    fn unknown_network_status_is_rejected_before_calling_c() {
        let error = set_network_status(NetworkStatus::Unknown(999_999)).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);
        assert!(error.diagnostic().contains("cannot be set"));
    }
}
