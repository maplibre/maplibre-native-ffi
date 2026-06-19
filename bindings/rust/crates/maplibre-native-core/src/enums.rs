use maplibre_native_sys as sys;

use crate::{Error, Result};

bitflags::bitflags! {
    /// Render backends compiled into the linked native library.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct RenderBackendMask: u32 {
        const METAL = sys::MLN_RENDER_BACKEND_FLAG_METAL;
        const VULKAN = sys::MLN_RENDER_BACKEND_FLAG_VULKAN;
        const OPENGL = sys::MLN_RENDER_BACKEND_FLAG_OPENGL;
        const _ = !0;
    }
}

bitflags::bitflags! {
    /// OpenGL context providers compiled into the linked native library.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct OpenGLContextProviderMask: u32 {
        const WGL = sys::MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL;
        const EGL = sys::MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL;
        const _ = !0;
    }
}

bitflags::bitflags! {
    /// Mask of log severities that MapLibre Native may dispatch asynchronously.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct LogSeverityMask: u32 {
        const INFO = sys::MLN_LOG_SEVERITY_MASK_INFO;
        const WARNING = sys::MLN_LOG_SEVERITY_MASK_WARNING;
        const ERROR = sys::MLN_LOG_SEVERITY_MASK_ERROR;
        const DEFAULT = sys::MLN_LOG_SEVERITY_MASK_DEFAULT;
        const ALL = sys::MLN_LOG_SEVERITY_MASK_ALL;
    }
}

bitflags::bitflags! {
    /// MapLibre debug overlay mask bits.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct MapDebugOptions: u32 {
        const TILE_BORDERS = sys::MLN_MAP_DEBUG_TILE_BORDERS;
        const PARSE_STATUS = sys::MLN_MAP_DEBUG_PARSE_STATUS;
        const TIMESTAMPS = sys::MLN_MAP_DEBUG_TIMESTAMPS;
        const COLLISION = sys::MLN_MAP_DEBUG_COLLISION;
        const OVERDRAW = sys::MLN_MAP_DEBUG_OVERDRAW;
        const STENCIL_CLIP = sys::MLN_MAP_DEBUG_STENCIL_CLIP;
        const DEPTH_BUFFER = sys::MLN_MAP_DEBUG_DEPTH_BUFFER;
        const _ = !0;
    }
}

/// Ambient cache maintenance operation for a runtime.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum AmbientCacheOperation {
    /// Reset the ambient cache database.
    ResetDatabase,
    /// Pack the ambient cache database.
    PackDatabase,
    /// Mark ambient cache resources as invalid.
    Invalidate,
    /// Clear ambient cache resources.
    Clear,
}

impl AmbientCacheOperation {
    pub const fn raw_value(self) -> u32 {
        match self {
            Self::ResetDatabase => sys::MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE,
            Self::PackDatabase => sys::MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE,
            Self::Invalidate => sys::MLN_AMBIENT_CACHE_OPERATION_INVALIDATE,
            Self::Clear => sys::MLN_AMBIENT_CACHE_OPERATION_CLEAR,
        }
    }

    pub const fn to_native(self) -> u32 {
        self.raw_value()
    }
}

/// Offline database operation kind reported by completion events.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum OfflineOperationKind {
    AmbientCache,
    RegionCreate,
    RegionGet,
    RegionsList,
    RegionsMergeDatabase,
    RegionUpdateMetadata,
    RegionGetStatus,
    RegionSetObserved,
    RegionSetDownloadState,
    RegionInvalidate,
    RegionDelete,
    Unknown(u32),
}

impl OfflineOperationKind {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_OFFLINE_OPERATION_AMBIENT_CACHE => Self::AmbientCache,
            sys::MLN_OFFLINE_OPERATION_REGION_CREATE => Self::RegionCreate,
            sys::MLN_OFFLINE_OPERATION_REGION_GET => Self::RegionGet,
            sys::MLN_OFFLINE_OPERATION_REGIONS_LIST => Self::RegionsList,
            sys::MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE => Self::RegionsMergeDatabase,
            sys::MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA => Self::RegionUpdateMetadata,
            sys::MLN_OFFLINE_OPERATION_REGION_GET_STATUS => Self::RegionGetStatus,
            sys::MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED => Self::RegionSetObserved,
            sys::MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE => Self::RegionSetDownloadState,
            sys::MLN_OFFLINE_OPERATION_REGION_INVALIDATE => Self::RegionInvalidate,
            sys::MLN_OFFLINE_OPERATION_REGION_DELETE => Self::RegionDelete,
            _ => Self::Unknown(raw),
        }
    }

    pub fn raw_value(self) -> u32 {
        match self {
            Self::AmbientCache => sys::MLN_OFFLINE_OPERATION_AMBIENT_CACHE,
            Self::RegionCreate => sys::MLN_OFFLINE_OPERATION_REGION_CREATE,
            Self::RegionGet => sys::MLN_OFFLINE_OPERATION_REGION_GET,
            Self::RegionsList => sys::MLN_OFFLINE_OPERATION_REGIONS_LIST,
            Self::RegionsMergeDatabase => sys::MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE,
            Self::RegionUpdateMetadata => sys::MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA,
            Self::RegionGetStatus => sys::MLN_OFFLINE_OPERATION_REGION_GET_STATUS,
            Self::RegionSetObserved => sys::MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED,
            Self::RegionSetDownloadState => sys::MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE,
            Self::RegionInvalidate => sys::MLN_OFFLINE_OPERATION_REGION_INVALIDATE,
            Self::RegionDelete => sys::MLN_OFFLINE_OPERATION_REGION_DELETE,
            Self::Unknown(raw) => raw,
        }
    }
}

/// Offline database operation result kind reported by completion events.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum OfflineOperationResultKind {
    None,
    Region,
    OptionalRegion,
    RegionList,
    RegionStatus,
    Unknown(u32),
}

impl OfflineOperationResultKind {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_OFFLINE_OPERATION_RESULT_NONE => Self::None,
            sys::MLN_OFFLINE_OPERATION_RESULT_REGION => Self::Region,
            sys::MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION => Self::OptionalRegion,
            sys::MLN_OFFLINE_OPERATION_RESULT_REGION_LIST => Self::RegionList,
            sys::MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS => Self::RegionStatus,
            _ => Self::Unknown(raw),
        }
    }

    pub fn raw_value(self) -> u32 {
        match self {
            Self::None => sys::MLN_OFFLINE_OPERATION_RESULT_NONE,
            Self::Region => sys::MLN_OFFLINE_OPERATION_RESULT_REGION,
            Self::OptionalRegion => sys::MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION,
            Self::RegionList => sys::MLN_OFFLINE_OPERATION_RESULT_REGION_LIST,
            Self::RegionStatus => sys::MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS,
            Self::Unknown(raw) => raw,
        }
    }
}

/// Style source type values returned by native style source metadata.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum SourceType {
    Unknown,
    Vector,
    Raster,
    RasterDem,
    GeoJson,
    Image,
    Video,
    Annotations,
    CustomVector,
    Other(u32),
}

impl SourceType {
    /// Converts a raw C ABI source type value into a Rust value, preserving
    /// future values.
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_STYLE_SOURCE_TYPE_UNKNOWN => Self::Unknown,
            sys::MLN_STYLE_SOURCE_TYPE_VECTOR => Self::Vector,
            sys::MLN_STYLE_SOURCE_TYPE_RASTER => Self::Raster,
            sys::MLN_STYLE_SOURCE_TYPE_RASTER_DEM => Self::RasterDem,
            sys::MLN_STYLE_SOURCE_TYPE_GEOJSON => Self::GeoJson,
            sys::MLN_STYLE_SOURCE_TYPE_IMAGE => Self::Image,
            sys::MLN_STYLE_SOURCE_TYPE_VIDEO => Self::Video,
            sys::MLN_STYLE_SOURCE_TYPE_ANNOTATIONS => Self::Annotations,
            sys::MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR => Self::CustomVector,
            _ => Self::Other(raw),
        }
    }

    /// Returns the raw C ABI source type value.
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Unknown => sys::MLN_STYLE_SOURCE_TYPE_UNKNOWN,
            Self::Vector => sys::MLN_STYLE_SOURCE_TYPE_VECTOR,
            Self::Raster => sys::MLN_STYLE_SOURCE_TYPE_RASTER,
            Self::RasterDem => sys::MLN_STYLE_SOURCE_TYPE_RASTER_DEM,
            Self::GeoJson => sys::MLN_STYLE_SOURCE_TYPE_GEOJSON,
            Self::Image => sys::MLN_STYLE_SOURCE_TYPE_IMAGE,
            Self::Video => sys::MLN_STYLE_SOURCE_TYPE_VIDEO,
            Self::Annotations => sys::MLN_STYLE_SOURCE_TYPE_ANNOTATIONS,
            Self::CustomVector => sys::MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR,
            Self::Other(raw) => raw,
        }
    }
}

/// Tile URL coordinate scheme for vector, raster, and raster DEM sources.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum TileScheme {
    Xyz,
    Tms,
}

impl TileScheme {
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Xyz => sys::MLN_STYLE_TILE_SCHEME_XYZ,
            Self::Tms => sys::MLN_STYLE_TILE_SCHEME_TMS,
        }
    }
}

/// Vector tile encoding for vector style sources.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum VectorTileEncoding {
    Mvt,
    Mlt,
}

impl VectorTileEncoding {
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Mvt => sys::MLN_STYLE_VECTOR_TILE_ENCODING_MVT,
            Self::Mlt => sys::MLN_STYLE_VECTOR_TILE_ENCODING_MLT,
        }
    }
}

/// DEM raster encoding for raster DEM style sources.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum RasterDemEncoding {
    Mapbox,
    Terrarium,
}

impl RasterDemEncoding {
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Mapbox => sys::MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX,
            Self::Terrarium => sys::MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM,
        }
    }
}

/// Image-name property slots for location indicator layers.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum LocationIndicatorImageKind {
    Top,
    Bearing,
    Shadow,
}

impl LocationIndicatorImageKind {
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Top => sys::MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP,
            Self::Bearing => sys::MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING,
            Self::Shadow => sys::MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW,
        }
    }
}

/// Map rendering mode used when creating a map.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum MapMode {
    Continuous,
    Static,
    Tile,
}

impl MapMode {
    pub fn from_raw(raw: u32) -> Option<Self> {
        match raw {
            sys::MLN_MAP_MODE_CONTINUOUS => Some(Self::Continuous),
            sys::MLN_MAP_MODE_STATIC => Some(Self::Static),
            sys::MLN_MAP_MODE_TILE => Some(Self::Tile),
            _ => None,
        }
    }

    pub fn as_raw(self) -> u32 {
        match self {
            Self::Continuous => sys::MLN_MAP_MODE_CONTINUOUS,
            Self::Static => sys::MLN_MAP_MODE_STATIC,
            Self::Tile => sys::MLN_MAP_MODE_TILE,
        }
    }
}

/// Map north orientation values used by viewport options.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum NorthOrientation {
    Up,
    Right,
    Down,
    Left,
    Unknown(u32),
}

impl NorthOrientation {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_NORTH_ORIENTATION_UP => Self::Up,
            sys::MLN_NORTH_ORIENTATION_RIGHT => Self::Right,
            sys::MLN_NORTH_ORIENTATION_DOWN => Self::Down,
            sys::MLN_NORTH_ORIENTATION_LEFT => Self::Left,
            _ => Self::Unknown(raw),
        }
    }

    pub fn as_raw(self) -> u32 {
        match self {
            Self::Up => sys::MLN_NORTH_ORIENTATION_UP,
            Self::Right => sys::MLN_NORTH_ORIENTATION_RIGHT,
            Self::Down => sys::MLN_NORTH_ORIENTATION_DOWN,
            Self::Left => sys::MLN_NORTH_ORIENTATION_LEFT,
            Self::Unknown(raw) => raw,
        }
    }
}

/// Map constraint modes used by viewport options.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ConstrainMode {
    None,
    HeightOnly,
    WidthAndHeight,
    Screen,
    Unknown(u32),
}

impl ConstrainMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_CONSTRAIN_MODE_NONE => Self::None,
            sys::MLN_CONSTRAIN_MODE_HEIGHT_ONLY => Self::HeightOnly,
            sys::MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT => Self::WidthAndHeight,
            sys::MLN_CONSTRAIN_MODE_SCREEN => Self::Screen,
            _ => Self::Unknown(raw),
        }
    }

    pub fn as_raw(self) -> u32 {
        match self {
            Self::None => sys::MLN_CONSTRAIN_MODE_NONE,
            Self::HeightOnly => sys::MLN_CONSTRAIN_MODE_HEIGHT_ONLY,
            Self::WidthAndHeight => sys::MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT,
            Self::Screen => sys::MLN_CONSTRAIN_MODE_SCREEN,
            Self::Unknown(raw) => raw,
        }
    }
}

/// Viewport orientation modes used by viewport options.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ViewportMode {
    Default,
    FlippedY,
    Unknown(u32),
}

impl ViewportMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_VIEWPORT_MODE_DEFAULT => Self::Default,
            sys::MLN_VIEWPORT_MODE_FLIPPED_Y => Self::FlippedY,
            _ => Self::Unknown(raw),
        }
    }

    pub fn as_raw(self) -> u32 {
        match self {
            Self::Default => sys::MLN_VIEWPORT_MODE_DEFAULT,
            Self::FlippedY => sys::MLN_VIEWPORT_MODE_FLIPPED_Y,
            Self::Unknown(raw) => raw,
        }
    }
}

/// Tile LOD algorithms used by map tile options.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum TileLodMode {
    Default,
    Distance,
    Unknown(u32),
}

impl TileLodMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_TILE_LOD_MODE_DEFAULT => Self::Default,
            sys::MLN_TILE_LOD_MODE_DISTANCE => Self::Distance,
            _ => Self::Unknown(raw),
        }
    }

    pub fn as_raw(self) -> u32 {
        match self {
            Self::Default => sys::MLN_TILE_LOD_MODE_DEFAULT,
            Self::Distance => sys::MLN_TILE_LOD_MODE_DISTANCE,
            Self::Unknown(raw) => raw,
        }
    }
}

/// Runtime event type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum RuntimeEventType {
    MapCameraWillChange,
    MapCameraIsChanging,
    MapCameraDidChange,
    MapStyleLoaded,
    MapLoadingStarted,
    MapLoadingFinished,
    MapLoadingFailed,
    MapIdle,
    MapRenderUpdateAvailable,
    MapRenderError,
    MapStillImageFinished,
    MapStillImageFailed,
    MapRenderFrameStarted,
    MapRenderFrameFinished,
    MapRenderMapStarted,
    MapRenderMapFinished,
    MapStyleImageMissing,
    MapTileAction,
    OfflineRegionStatusChanged,
    OfflineRegionResponseError,
    OfflineRegionTileCountLimitExceeded,
    OfflineOperationCompleted,
    Unknown(u32),
}

impl RuntimeEventType {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE => Self::MapCameraWillChange,
            sys::MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING => Self::MapCameraIsChanging,
            sys::MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE => Self::MapCameraDidChange,
            sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED => Self::MapStyleLoaded,
            sys::MLN_RUNTIME_EVENT_MAP_LOADING_STARTED => Self::MapLoadingStarted,
            sys::MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED => Self::MapLoadingFinished,
            sys::MLN_RUNTIME_EVENT_MAP_LOADING_FAILED => Self::MapLoadingFailed,
            sys::MLN_RUNTIME_EVENT_MAP_IDLE => Self::MapIdle,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE => Self::MapRenderUpdateAvailable,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_ERROR => Self::MapRenderError,
            sys::MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED => Self::MapStillImageFinished,
            sys::MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED => Self::MapStillImageFailed,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED => Self::MapRenderFrameStarted,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED => Self::MapRenderFrameFinished,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED => Self::MapRenderMapStarted,
            sys::MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED => Self::MapRenderMapFinished,
            sys::MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING => Self::MapStyleImageMissing,
            sys::MLN_RUNTIME_EVENT_MAP_TILE_ACTION => Self::MapTileAction,
            sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED => {
                Self::OfflineRegionStatusChanged
            }
            sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR => {
                Self::OfflineRegionResponseError
            }
            sys::MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED => {
                Self::OfflineRegionTileCountLimitExceeded
            }
            sys::MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED => Self::OfflineOperationCompleted,
            _ => Self::Unknown(raw),
        }
    }
}

/// Render mode reported by render observer events.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum RenderMode {
    Partial,
    Full,
    Unknown(u32),
}

impl RenderMode {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RENDER_MODE_PARTIAL => Self::Partial,
            sys::MLN_RENDER_MODE_FULL => Self::Full,
            _ => Self::Unknown(raw),
        }
    }
}

/// Tile operation reported by tile observer events.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum TileOperation {
    RequestedFromCache,
    RequestedFromNetwork,
    LoadFromNetwork,
    LoadFromCache,
    StartParse,
    EndParse,
    Error,
    Cancelled,
    Null,
    Unknown(u32),
}

impl TileOperation {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_TILE_OPERATION_REQUESTED_FROM_CACHE => Self::RequestedFromCache,
            sys::MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK => Self::RequestedFromNetwork,
            sys::MLN_TILE_OPERATION_LOAD_FROM_NETWORK => Self::LoadFromNetwork,
            sys::MLN_TILE_OPERATION_LOAD_FROM_CACHE => Self::LoadFromCache,
            sys::MLN_TILE_OPERATION_START_PARSE => Self::StartParse,
            sys::MLN_TILE_OPERATION_END_PARSE => Self::EndParse,
            sys::MLN_TILE_OPERATION_ERROR => Self::Error,
            sys::MLN_TILE_OPERATION_CANCELLED => Self::Cancelled,
            sys::MLN_TILE_OPERATION_NULL => Self::Null,
            _ => Self::Unknown(raw),
        }
    }
}

/// Offline region download state copied from event payloads.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum OfflineRegionDownloadState {
    Inactive,
    Active,
    Unknown(u32),
}

impl OfflineRegionDownloadState {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE => Self::Inactive,
            sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE => Self::Active,
            _ => Self::Unknown(raw),
        }
    }

    pub fn raw_for_set(self) -> Result<u32> {
        match self {
            Self::Inactive => Ok(sys::MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE),
            Self::Active => Ok(sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE),
            Self::Unknown(raw) => Err(Error::invalid_argument(format!(
                "unknown offline region download state cannot be set: {raw}"
            ))),
        }
    }
}

/// Resource error reason copied from event payloads and used in resource responses.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceErrorReason {
    None,
    NotFound,
    Server,
    Connection,
    RateLimit,
    Other,
    Unknown(u32),
}

impl ResourceErrorReason {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RESOURCE_ERROR_REASON_NONE => Self::None,
            sys::MLN_RESOURCE_ERROR_REASON_NOT_FOUND => Self::NotFound,
            sys::MLN_RESOURCE_ERROR_REASON_SERVER => Self::Server,
            sys::MLN_RESOURCE_ERROR_REASON_CONNECTION => Self::Connection,
            sys::MLN_RESOURCE_ERROR_REASON_RATE_LIMIT => Self::RateLimit,
            sys::MLN_RESOURCE_ERROR_REASON_OTHER => Self::Other,
            _ => Self::Unknown(raw),
        }
    }

    pub fn raw_value(self) -> u32 {
        match self {
            Self::None => sys::MLN_RESOURCE_ERROR_REASON_NONE,
            Self::NotFound => sys::MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
            Self::Server => sys::MLN_RESOURCE_ERROR_REASON_SERVER,
            Self::Connection => sys::MLN_RESOURCE_ERROR_REASON_CONNECTION,
            Self::RateLimit => sys::MLN_RESOURCE_ERROR_REASON_RATE_LIMIT,
            Self::Other => sys::MLN_RESOURCE_ERROR_REASON_OTHER,
            Self::Unknown(raw) => raw,
        }
    }
}

/// Severity for a MapLibre Native log record.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum LogSeverity {
    Info,
    Warning,
    Error,
    Unknown(u32),
}

impl LogSeverity {
    /// Returns the raw C ABI value for this severity.
    pub fn raw_value(self) -> u32 {
        match self {
            Self::Info => sys::MLN_LOG_SEVERITY_INFO,
            Self::Warning => sys::MLN_LOG_SEVERITY_WARNING,
            Self::Error => sys::MLN_LOG_SEVERITY_ERROR,
            Self::Unknown(raw) => raw,
        }
    }

    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_LOG_SEVERITY_INFO => Self::Info,
            sys::MLN_LOG_SEVERITY_WARNING => Self::Warning,
            sys::MLN_LOG_SEVERITY_ERROR => Self::Error,
            _ => Self::Unknown(raw),
        }
    }
}

/// Category for a MapLibre Native log record.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum LogEvent {
    General,
    Setup,
    Shader,
    ParseStyle,
    ParseTile,
    Render,
    Style,
    Database,
    HttpRequest,
    Sprite,
    Image,
    OpenGl,
    Jni,
    Android,
    Crash,
    Glyph,
    Timing,
    Unknown(u32),
}

impl LogEvent {
    /// Returns the raw C ABI value for this event category.
    pub fn raw_value(self) -> u32 {
        match self {
            Self::General => sys::MLN_LOG_EVENT_GENERAL,
            Self::Setup => sys::MLN_LOG_EVENT_SETUP,
            Self::Shader => sys::MLN_LOG_EVENT_SHADER,
            Self::ParseStyle => sys::MLN_LOG_EVENT_PARSE_STYLE,
            Self::ParseTile => sys::MLN_LOG_EVENT_PARSE_TILE,
            Self::Render => sys::MLN_LOG_EVENT_RENDER,
            Self::Style => sys::MLN_LOG_EVENT_STYLE,
            Self::Database => sys::MLN_LOG_EVENT_DATABASE,
            Self::HttpRequest => sys::MLN_LOG_EVENT_HTTP_REQUEST,
            Self::Sprite => sys::MLN_LOG_EVENT_SPRITE,
            Self::Image => sys::MLN_LOG_EVENT_IMAGE,
            Self::OpenGl => sys::MLN_LOG_EVENT_OPENGL,
            Self::Jni => sys::MLN_LOG_EVENT_JNI,
            Self::Android => sys::MLN_LOG_EVENT_ANDROID,
            Self::Crash => sys::MLN_LOG_EVENT_CRASH,
            Self::Glyph => sys::MLN_LOG_EVENT_GLYPH,
            Self::Timing => sys::MLN_LOG_EVENT_TIMING,
            Self::Unknown(raw) => raw,
        }
    }

    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_LOG_EVENT_GENERAL => Self::General,
            sys::MLN_LOG_EVENT_SETUP => Self::Setup,
            sys::MLN_LOG_EVENT_SHADER => Self::Shader,
            sys::MLN_LOG_EVENT_PARSE_STYLE => Self::ParseStyle,
            sys::MLN_LOG_EVENT_PARSE_TILE => Self::ParseTile,
            sys::MLN_LOG_EVENT_RENDER => Self::Render,
            sys::MLN_LOG_EVENT_STYLE => Self::Style,
            sys::MLN_LOG_EVENT_DATABASE => Self::Database,
            sys::MLN_LOG_EVENT_HTTP_REQUEST => Self::HttpRequest,
            sys::MLN_LOG_EVENT_SPRITE => Self::Sprite,
            sys::MLN_LOG_EVENT_IMAGE => Self::Image,
            sys::MLN_LOG_EVENT_OPENGL => Self::OpenGl,
            sys::MLN_LOG_EVENT_JNI => Self::Jni,
            sys::MLN_LOG_EVENT_ANDROID => Self::Android,
            sys::MLN_LOG_EVENT_CRASH => Self::Crash,
            sys::MLN_LOG_EVENT_GLYPH => Self::Glyph,
            sys::MLN_LOG_EVENT_TIMING => Self::Timing,
            _ => Self::Unknown(raw),
        }
    }
}

/// Network resource kind passed to resource callbacks.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceKind {
    Unknown,
    Style,
    Source,
    Tile,
    Glyphs,
    SpriteImage,
    SpriteJson,
    Image,
    UnknownRaw(u32),
}

pub fn resource_kind_from_raw(raw: u32) -> ResourceKind {
    match raw {
        sys::MLN_RESOURCE_KIND_UNKNOWN => ResourceKind::Unknown,
        sys::MLN_RESOURCE_KIND_STYLE => ResourceKind::Style,
        sys::MLN_RESOURCE_KIND_SOURCE => ResourceKind::Source,
        sys::MLN_RESOURCE_KIND_TILE => ResourceKind::Tile,
        sys::MLN_RESOURCE_KIND_GLYPHS => ResourceKind::Glyphs,
        sys::MLN_RESOURCE_KIND_SPRITE_IMAGE => ResourceKind::SpriteImage,
        sys::MLN_RESOURCE_KIND_SPRITE_JSON => ResourceKind::SpriteJson,
        sys::MLN_RESOURCE_KIND_IMAGE => ResourceKind::Image,
        _ => ResourceKind::UnknownRaw(raw),
    }
}

/// Status for a resource provider response.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceResponseStatus {
    Ok,
    Error,
    NoContent,
    NotModified,
}

impl ResourceResponseStatus {
    pub fn as_raw(self) -> u32 {
        match self {
            Self::Ok => sys::MLN_RESOURCE_RESPONSE_STATUS_OK,
            Self::Error => sys::MLN_RESOURCE_RESPONSE_STATUS_ERROR,
            Self::NoContent => sys::MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT,
            Self::NotModified => sys::MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED,
        }
    }
}

/// Resource loading method requested by MapLibre Native.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceLoadingMethod {
    All,
    CacheOnly,
    NetworkOnly,
    Unknown(u32),
}

pub fn resource_loading_method_from_raw(raw: u32) -> ResourceLoadingMethod {
    match raw {
        sys::MLN_RESOURCE_LOADING_METHOD_ALL => ResourceLoadingMethod::All,
        sys::MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY => ResourceLoadingMethod::CacheOnly,
        sys::MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY => ResourceLoadingMethod::NetworkOnly,
        _ => ResourceLoadingMethod::Unknown(raw),
    }
}

/// Resource request priority.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourcePriority {
    Regular,
    Low,
    Unknown(u32),
}

pub fn resource_priority_from_raw(raw: u32) -> ResourcePriority {
    match raw {
        sys::MLN_RESOURCE_PRIORITY_REGULAR => ResourcePriority::Regular,
        sys::MLN_RESOURCE_PRIORITY_LOW => ResourcePriority::Low,
        _ => ResourcePriority::Unknown(raw),
    }
}

/// Resource request usage.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceUsage {
    Online,
    Offline,
    Unknown(u32),
}

pub fn resource_usage_from_raw(raw: u32) -> ResourceUsage {
    match raw {
        sys::MLN_RESOURCE_USAGE_ONLINE => ResourceUsage::Online,
        sys::MLN_RESOURCE_USAGE_OFFLINE => ResourceUsage::Offline,
        _ => ResourceUsage::Unknown(raw),
    }
}

/// Resource cache storage policy.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceStoragePolicy {
    Permanent,
    Volatile,
    Unknown(u32),
}

pub fn resource_storage_policy_from_raw(raw: u32) -> ResourceStoragePolicy {
    match raw {
        sys::MLN_RESOURCE_STORAGE_POLICY_PERMANENT => ResourceStoragePolicy::Permanent,
        sys::MLN_RESOURCE_STORAGE_POLICY_VOLATILE => ResourceStoragePolicy::Volatile,
        _ => ResourceStoragePolicy::Unknown(raw),
    }
}

/// Process-global network reachability state used by MapLibre Native.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum NetworkStatus {
    Online,
    Offline,
    Unknown(u32),
}

impl NetworkStatus {
    pub fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_NETWORK_STATUS_ONLINE => Self::Online,
            sys::MLN_NETWORK_STATUS_OFFLINE => Self::Offline,
            _ => Self::Unknown(raw),
        }
    }

    pub fn raw(self) -> Result<u32> {
        match self {
            Self::Online => Ok(sys::MLN_NETWORK_STATUS_ONLINE),
            Self::Offline => Ok(sys::MLN_NETWORK_STATUS_OFFLINE),
            Self::Unknown(raw) => Err(Error::invalid_argument(format!(
                "unknown network status values cannot be set: {raw}"
            ))),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ErrorKind;

    #[test]
    // Spec coverage: BND-062.
    fn log_severity_maps_raw_values_and_preserves_unknowns() {
        assert_eq!(
            LogSeverity::from_raw(sys::MLN_LOG_SEVERITY_INFO),
            LogSeverity::Info
        );
        assert_eq!(LogSeverity::Info.raw_value(), sys::MLN_LOG_SEVERITY_INFO);
        assert_eq!(
            LogSeverity::from_raw(999_001),
            LogSeverity::Unknown(999_001)
        );
        assert_eq!(LogSeverity::Unknown(999_001).raw_value(), 999_001);
    }

    #[test]
    fn log_event_maps_raw_values_and_preserves_unknowns() {
        assert_eq!(
            LogEvent::from_raw(sys::MLN_LOG_EVENT_PARSE_STYLE),
            LogEvent::ParseStyle
        );
        assert_eq!(
            LogEvent::ParseStyle.raw_value(),
            sys::MLN_LOG_EVENT_PARSE_STYLE
        );
        assert_eq!(LogEvent::from_raw(999_002), LogEvent::Unknown(999_002));
        assert_eq!(LogEvent::Unknown(999_002).raw_value(), 999_002);
    }

    #[test]
    fn bitmask_domains_map_raw_bits_and_preserve_unknowns() {
        assert_eq!(
            RenderBackendMask::METAL.bits(),
            sys::MLN_RENDER_BACKEND_FLAG_METAL
        );
        assert_eq!(
            RenderBackendMask::VULKAN.bits(),
            sys::MLN_RENDER_BACKEND_FLAG_VULKAN
        );
        assert_eq!(
            RenderBackendMask::OPENGL.bits(),
            sys::MLN_RENDER_BACKEND_FLAG_OPENGL
        );
        assert_eq!(RenderBackendMask::from_bits_retain(1 << 31).bits(), 1 << 31);

        assert_eq!(
            OpenGLContextProviderMask::WGL.bits(),
            sys::MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL
        );
        assert_eq!(
            OpenGLContextProviderMask::EGL.bits(),
            sys::MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL
        );
        assert_eq!(
            OpenGLContextProviderMask::from_bits_retain(1 << 31).bits(),
            1 << 31
        );

        assert_eq!(
            LogSeverityMask::INFO.bits(),
            sys::MLN_LOG_SEVERITY_MASK_INFO
        );
        assert_eq!(
            LogSeverityMask::DEFAULT.bits(),
            sys::MLN_LOG_SEVERITY_MASK_DEFAULT
        );
        assert_eq!(LogSeverityMask::from_bits_retain(1 << 31).bits(), 1 << 31);

        assert_eq!(
            MapDebugOptions::TILE_BORDERS.bits(),
            sys::MLN_MAP_DEBUG_TILE_BORDERS
        );
        assert_eq!(
            MapDebugOptions::DEPTH_BUFFER.bits(),
            sys::MLN_MAP_DEBUG_DEPTH_BUFFER
        );
        assert_eq!(MapDebugOptions::from_bits_retain(1 << 31).bits(), 1 << 31);
    }

    #[test]
    fn ambient_cache_operation_maps_raw_values() {
        assert_eq!(
            AmbientCacheOperation::ResetDatabase.raw_value(),
            sys::MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE
        );
        assert_eq!(
            AmbientCacheOperation::PackDatabase.to_native(),
            sys::MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE
        );
        assert_eq!(
            AmbientCacheOperation::Invalidate.raw_value(),
            sys::MLN_AMBIENT_CACHE_OPERATION_INVALIDATE
        );
        assert_eq!(
            AmbientCacheOperation::Clear.raw_value(),
            sys::MLN_AMBIENT_CACHE_OPERATION_CLEAR
        );
    }

    #[test]
    fn style_enums_map_raw_values_and_preserve_unknowns() {
        assert_eq!(
            SourceType::from_raw(sys::MLN_STYLE_SOURCE_TYPE_GEOJSON),
            SourceType::GeoJson
        );
        assert_eq!(
            SourceType::GeoJson.raw_value(),
            sys::MLN_STYLE_SOURCE_TYPE_GEOJSON
        );
        assert_eq!(SourceType::from_raw(999_040), SourceType::Other(999_040));
        assert_eq!(SourceType::Other(999_040).raw_value(), 999_040);

        assert_eq!(TileScheme::Tms.raw_value(), sys::MLN_STYLE_TILE_SCHEME_TMS);
        assert_eq!(
            VectorTileEncoding::Mlt.raw_value(),
            sys::MLN_STYLE_VECTOR_TILE_ENCODING_MLT
        );
        assert_eq!(
            RasterDemEncoding::Terrarium.raw_value(),
            sys::MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM
        );
        assert_eq!(
            LocationIndicatorImageKind::Bearing.raw_value(),
            sys::MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING
        );
    }

    #[test]
    fn map_option_enums_map_raw_values_and_preserve_unknowns() {
        assert_eq!(
            MapMode::from_raw(sys::MLN_MAP_MODE_TILE),
            Some(MapMode::Tile)
        );
        assert_eq!(MapMode::Tile.as_raw(), sys::MLN_MAP_MODE_TILE);
        assert_eq!(MapMode::from_raw(999_030), None);

        assert_eq!(
            NorthOrientation::from_raw(sys::MLN_NORTH_ORIENTATION_LEFT),
            NorthOrientation::Left
        );
        assert_eq!(
            NorthOrientation::Left.as_raw(),
            sys::MLN_NORTH_ORIENTATION_LEFT
        );
        assert_eq!(
            NorthOrientation::from_raw(999_031),
            NorthOrientation::Unknown(999_031)
        );
        assert_eq!(NorthOrientation::Unknown(999_031).as_raw(), 999_031);

        assert_eq!(
            ConstrainMode::from_raw(sys::MLN_CONSTRAIN_MODE_SCREEN),
            ConstrainMode::Screen
        );
        assert_eq!(
            ConstrainMode::Screen.as_raw(),
            sys::MLN_CONSTRAIN_MODE_SCREEN
        );
        assert_eq!(
            ConstrainMode::from_raw(999_032),
            ConstrainMode::Unknown(999_032)
        );

        assert_eq!(
            ViewportMode::from_raw(sys::MLN_VIEWPORT_MODE_FLIPPED_Y),
            ViewportMode::FlippedY
        );
        assert_eq!(
            ViewportMode::FlippedY.as_raw(),
            sys::MLN_VIEWPORT_MODE_FLIPPED_Y
        );
        assert_eq!(
            ViewportMode::from_raw(999_033),
            ViewportMode::Unknown(999_033)
        );

        assert_eq!(
            TileLodMode::from_raw(sys::MLN_TILE_LOD_MODE_DISTANCE),
            TileLodMode::Distance
        );
        assert_eq!(
            TileLodMode::Distance.as_raw(),
            sys::MLN_TILE_LOD_MODE_DISTANCE
        );
        assert_eq!(
            TileLodMode::from_raw(999_034),
            TileLodMode::Unknown(999_034)
        );
    }

    #[test]
    fn runtime_event_enums_map_raw_values_and_preserve_unknowns() {
        assert_eq!(
            RuntimeEventType::from_raw(sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED),
            RuntimeEventType::MapStyleLoaded
        );
        assert_eq!(
            RuntimeEventType::from_raw(999_020),
            RuntimeEventType::Unknown(999_020)
        );

        assert_eq!(
            RenderMode::from_raw(sys::MLN_RENDER_MODE_FULL),
            RenderMode::Full
        );
        assert_eq!(RenderMode::from_raw(999_021), RenderMode::Unknown(999_021));

        assert_eq!(
            TileOperation::from_raw(sys::MLN_TILE_OPERATION_CANCELLED),
            TileOperation::Cancelled
        );
        assert_eq!(
            TileOperation::from_raw(999_022),
            TileOperation::Unknown(999_022)
        );
    }

    #[test]
    // Spec coverage: BND-068.
    fn offline_download_state_maps_raw_values_and_rejects_unknown_for_set() {
        assert_eq!(
            OfflineRegionDownloadState::from_raw(sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE),
            OfflineRegionDownloadState::Active
        );
        assert_eq!(
            OfflineRegionDownloadState::Active.raw_for_set().unwrap(),
            sys::MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE
        );
        assert_eq!(
            OfflineRegionDownloadState::from_raw(999_023),
            OfflineRegionDownloadState::Unknown(999_023)
        );
        let error = OfflineRegionDownloadState::Unknown(999_023)
            .raw_for_set()
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    }

    #[test]
    fn resource_error_reason_maps_raw_values_and_preserves_unknowns() {
        assert_eq!(
            ResourceErrorReason::from_raw(sys::MLN_RESOURCE_ERROR_REASON_RATE_LIMIT),
            ResourceErrorReason::RateLimit
        );
        assert_eq!(
            ResourceErrorReason::RateLimit.raw_value(),
            sys::MLN_RESOURCE_ERROR_REASON_RATE_LIMIT
        );
        assert_eq!(
            ResourceErrorReason::from_raw(999_024),
            ResourceErrorReason::Unknown(999_024)
        );
        assert_eq!(ResourceErrorReason::Unknown(999_024).raw_value(), 999_024);
    }

    #[test]
    fn resource_request_enums_map_raw_values_and_preserve_unknowns() {
        assert_eq!(
            resource_kind_from_raw(sys::MLN_RESOURCE_KIND_TILE),
            ResourceKind::Tile
        );
        assert_eq!(
            resource_kind_from_raw(999_010),
            ResourceKind::UnknownRaw(999_010)
        );

        assert_eq!(
            ResourceResponseStatus::NotModified.as_raw(),
            sys::MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED
        );

        assert_eq!(
            resource_loading_method_from_raw(sys::MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY),
            ResourceLoadingMethod::CacheOnly
        );
        assert_eq!(
            resource_loading_method_from_raw(999_011),
            ResourceLoadingMethod::Unknown(999_011)
        );

        assert_eq!(
            resource_priority_from_raw(sys::MLN_RESOURCE_PRIORITY_LOW),
            ResourcePriority::Low
        );
        assert_eq!(
            resource_priority_from_raw(999_012),
            ResourcePriority::Unknown(999_012)
        );

        assert_eq!(
            resource_usage_from_raw(sys::MLN_RESOURCE_USAGE_OFFLINE),
            ResourceUsage::Offline
        );
        assert_eq!(
            resource_usage_from_raw(999_013),
            ResourceUsage::Unknown(999_013)
        );

        assert_eq!(
            resource_storage_policy_from_raw(sys::MLN_RESOURCE_STORAGE_POLICY_VOLATILE),
            ResourceStoragePolicy::Volatile
        );
        assert_eq!(
            resource_storage_policy_from_raw(999_014),
            ResourceStoragePolicy::Unknown(999_014)
        );
    }

    #[test]
    // Spec coverage: BND-068.
    fn network_status_preserves_unknown_raw_values() {
        assert_eq!(
            NetworkStatus::from_raw(999_999),
            NetworkStatus::Unknown(999_999)
        );

        let error = NetworkStatus::Unknown(999_999).raw().unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);
        assert!(error.diagnostic().contains("cannot be set"));
    }
}
