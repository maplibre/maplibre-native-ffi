//! Shared ABI adaptation for MapLibre Native bridge bindings.
//!
//! This crate sits above `maplibre-native-sys` and below public language
//! bindings. It owns reusable C ABI facts such as status mapping,
//! diagnostics, pointer checks, native string helpers, and short-lived native
//! result guards. Public bindings layer their handle, lifetime, callback,
//! threading, and host-runtime policies above these building blocks.
//!
//! This is an internal workspace crate, not the supported safe Rust binding.
//! Rust applications should use the `maplibre-native` crate.

#![deny(unsafe_op_in_unsafe_fn)]

pub mod abi;
pub mod camera;
pub mod enums;
pub mod error;
pub mod events;
pub mod geojson;
pub mod geometry;
pub mod handle;
pub mod json;
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
    AnimationOptions, BoundOptions, CameraFitOptions, CameraOptions, FreeCameraOptions,
    ProjectionMode,
};
pub use enums::{
    AmbientCacheOperation, ConstrainMode, LocationIndicatorImageKind, LogEvent, LogSeverity,
    LogSeverityMask, MapDebugOptions, MapMode, NetworkStatus, NorthOrientation,
    OfflineOperationKind, OfflineOperationResultKind, OfflineRegionDownloadState,
    OpenGLContextProviderMask, RasterDemEncoding, RenderBackendMask, RenderMode,
    ResourceErrorReason, ResourceKind, ResourceLoadingMethod, ResourcePriority,
    ResourceResponseStatus, ResourceStoragePolicy, ResourceUsage, RuntimeEventType, SourceType,
    TileLodMode, TileOperation, TileScheme, VectorTileEncoding, ViewportMode,
};
pub use error::{Error, ErrorKind, Result, check};
pub use events::{
    CopiedRuntimeEvent, OfflineOperationCompletedEvent, OfflineRegionResponseErrorEvent,
    OfflineRegionStatus, OfflineRegionStatusEvent, OfflineRegionTileCountLimitEvent,
    RawRuntimeEventSource, RenderFrameEvent, RenderMapEvent, RenderingStats, RuntimeEventPayload,
    StyleImageMissingEvent, TileActionEvent, TileId, UnknownRuntimeEventPayload,
};
pub use geojson::{Feature, FeatureIdentifier, GeoJson};
pub use geometry::Geometry;
pub use json::{JsonMember, JsonValue};
pub use logging::LogRecord;
pub use options::{MapOptions, MapTileOptions, MapViewportOptions};
pub use query::{
    FeatureStateSelector, RenderedFeatureQueryOptions, RenderedQueryGeometry,
    SourceFeatureQueryOptions,
};
pub use resource::{
    ByteRange, ResourceProviderDecision, ResourceRequest, ResourceResponse,
    ResourceTransformRequest,
};
pub use runtime::{OfflineRegionDefinition, OfflineRegionInfo, RuntimeOptions};
pub use style::{SourceInfo, StyleImage, StyleImageOptions, TileSourceOptions};
pub use values::{
    EdgeInsets, LatLng, LatLngBounds, PremultipliedRgba8Image, ProjectedMeters, Quaternion,
    ScreenBox, ScreenPoint, StyleImageInfo, TextureImageInfo, UnitBezier, Vec3,
};
