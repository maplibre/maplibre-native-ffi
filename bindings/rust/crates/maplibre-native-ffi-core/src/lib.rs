//! Shared ABI adaptation for MapLibre Native bridge bindings: status mapping,
//! diagnostics, pointer checks, native string helpers, and native result
//! guards.
//!
//! This is an internal workspace crate, not the supported safe Rust binding.
//! Rust applications should use the `maplibre-native-ffi` crate.

#![deny(unsafe_op_in_unsafe_fn)]

pub mod abi;
pub mod camera;
pub mod enums;
pub mod error;
pub mod events;
pub mod handle;
pub mod logging;
pub mod options;
pub mod ptr;
pub mod query;
pub mod render;
pub mod resource;
pub mod runtime;
pub mod string;
pub mod style;
pub mod values;

pub use abi::{EXPECTED_C_ABI_VERSION, validate_abi_version, validate_abi_version_value};
pub use camera::{
    AnimationOptions, BoundOptions, BoundsConstraint, CameraFitOptions, CameraOptions,
    FreeCameraOptions, ProjectionMode,
};
pub use enums::{
    AmbientCacheOperation, CameraChangeMode, ConstrainMode, LocationIndicatorImageKind, LogEvent,
    LogSeverity, LogSeverityMask, MapDebugOptions, MapMode, NetworkStatus, NorthOrientation,
    OfflineOperationKind, OfflineOperationResultKind, OfflineRegionDownloadState,
    OpenGLContextProviderMask, RasterDemEncoding, RenderBackendMask, RenderMode, RenderResult,
    ResourceErrorReason, ResourceKind, ResourceLoadingMethod, ResourcePriority,
    ResourceResponseStatus, ResourceStoragePolicy, ResourceUsage, RuntimeEventType, SourceType,
    StyleImageTextFit, StyleLayerVisibility, TileLodMode, TileOperation, TileScheme,
    VectorTileEncoding, ViewportMode,
};
pub use error::{Error, ErrorKind, Result, check};
pub use events::{
    CameraTransitionFinishedEvent, CopiedRuntimeEvent, OfflineOperationCompletedEvent,
    OfflineRegionResponseErrorEvent, OfflineRegionStatus, OfflineRegionStatusEvent,
    OfflineRegionTileCountLimitEvent, RawRuntimeEventSource, RenderFrameEvent, RenderMapEvent,
    RenderingStats, RuntimeEventPayload, StyleImageMissingEvent, TileActionEvent, TileId,
    UnknownRuntimeEventPayload,
};
pub use logging::LogRecord;
pub use options::{MapOptions, MapTileOptions, MapViewportOptions};
pub use query::{
    FeatureStateSelector, RenderedFeatureQueryOptions, RenderedQueryGeometry,
    SourceFeatureQueryOptions,
};
pub use resource::{
    ByteRange, HttpHeader, HttpHeaderTransformRequest, ResourceProviderDecision, ResourceRequest,
    ResourceResponse, ResourceTransformRequest,
};
pub use runtime::{
    OfflineRegionDefinition, OfflineRegionInfo, RuntimeOptions, network_status, set_network_status,
    set_network_status_raw,
};
pub use style::{
    GeoJsonSourceOptions, ImageContent, ImageStretch, SourceInfo, StyleImage, StyleImageOptions,
    StyleTransitionOptions, TileJsonInfo, TileSourceOptions,
};
pub use values::{
    EdgeInsets, LatLng, LatLngBounds, PremultipliedRgba8Image, ProjectedMeters, Quaternion,
    ScreenBox, ScreenPoint, StyleImageInfo, TextureImageInfo, UnitBezier, Vec3,
};
