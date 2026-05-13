//! Shared ABI adaptation for MapLibre Native bridge bindings.
//!
//! This crate sits above `maplibre-native-sys` and below public language
//! bindings. It owns reusable C ABI facts such as status mapping,
//! diagnostics, pointer checks, native string helpers, and short-lived native
//! result guards. Public bindings layer their handle, lifetime, callback,
//! threading, and host-runtime policies above these building blocks.

#![deny(unsafe_op_in_unsafe_fn)]

pub mod abi;
pub mod camera;
pub mod enums;
pub mod error;
pub mod geojson;
pub mod geometry;
pub mod handle;
pub mod json;
pub mod options;
pub mod ptr;
pub mod query;
pub mod render;
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
    OfflineRegionDownloadState, RasterDemEncoding, RenderBackendMask, RenderMode,
    ResourceErrorReason, ResourceKind, ResourceLoadingMethod, ResourcePriority,
    ResourceResponseStatus, ResourceStoragePolicy, ResourceUsage, RuntimeEventType, SourceType,
    TileLodMode, TileOperation, TileScheme, VectorTileEncoding, ViewportMode,
};
pub use error::{Error, ErrorKind, Result, check};
pub use geojson::{Feature, FeatureIdentifier, GeoJson};
pub use geometry::Geometry;
pub use json::{JsonMember, JsonValue};
pub use options::{MapOptions, MapTileOptions, MapViewportOptions};
pub use query::{
    FeatureStateSelector, RenderedFeatureQueryOptions, RenderedQueryGeometry,
    SourceFeatureQueryOptions,
};
pub use runtime::{OfflineRegionDefinition, OfflineRegionInfo, RuntimeOptions};
pub use style::{StyleImageOptions, TileSourceOptions};
pub use values::{
    EdgeInsets, LatLng, LatLngBounds, PremultipliedRgba8Image, ProjectedMeters, Quaternion,
    ScreenBox, ScreenPoint, StyleImageInfo, TextureImageInfo, UnitBezier, Vec3,
};
