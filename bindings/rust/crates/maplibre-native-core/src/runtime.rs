use std::ffi::CString;
use std::ptr::NonNull;

use maplibre_native_sys as sys;

use crate::{NetworkStatus, Result, check, string, validate_abi_version};

/// Reads MapLibre Native's process-global network status.
pub fn network_status() -> Result<NetworkStatus> {
    let mut raw_status = 0;
    // SAFETY: out_status points to valid writable storage for one u32.
    check(unsafe { sys::mln_network_status_get(&mut raw_status) })?;
    Ok(NetworkStatus::from_raw(raw_status))
}

/// Sets MapLibre Native's process-global network status.
pub fn set_network_status(status: NetworkStatus) -> Result<()> {
    set_network_status_raw(status.raw()?)
}

/// Sets MapLibre Native's process-global network status from a raw C value.
///
/// This helper is for bridge tests and native status conversion paths. Public
/// bindings should normally map their language enum first so unknown values can
/// fail before crossing the C boundary.
pub fn set_network_status_raw(raw_status: u32) -> Result<()> {
    // SAFETY: The raw value is passed by value. The C API validates the enum
    // domain and reports invalid values as MLN_STATUS_INVALID_ARGUMENT.
    check(unsafe { sys::mln_network_status_set(raw_status) })
}

/// Options used when creating a runtime.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
#[non_exhaustive]
pub struct RuntimeOptions {
    /// Filesystem root for `asset://` URLs.
    pub asset_path: Option<String>,
    /// Cache database path.
    pub cache_path: Option<String>,
    /// Maximum ambient cache size in bytes.
    pub maximum_cache_size: Option<u64>,
}

impl RuntimeOptions {}

/// Region descriptor used to create or inspect offline regions.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum OfflineRegionDefinition {
    TilePyramid {
        style_url: String,
        bounds: crate::LatLngBounds,
        min_zoom: f64,
        max_zoom: f64,
        pixel_ratio: f32,
        include_ideographs: bool,
    },
    GeometryRegion {
        style_url: String,
        geometry: crate::Geometry,
        min_zoom: f64,
        max_zoom: f64,
        pixel_ratio: f32,
        include_ideographs: bool,
    },
}

/// Offline region snapshot copied from native storage.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct OfflineRegionInfo {
    pub id: i64,
    pub definition: OfflineRegionDefinition,
    pub metadata: Vec<u8>,
}

pub enum NativeOfflineRegionDefinition {
    TilePyramid {
        style_url: CString,
        bounds: crate::LatLngBounds,
        min_zoom: f64,
        max_zoom: f64,
        pixel_ratio: f32,
        include_ideographs: bool,
    },
    GeometryRegion {
        style_url: CString,
        geometry: crate::geometry::NativeGeometry,
        min_zoom: f64,
        max_zoom: f64,
        pixel_ratio: f32,
        include_ideographs: bool,
    },
}

impl NativeOfflineRegionDefinition {
    fn new(definition: &OfflineRegionDefinition) -> Result<Self> {
        match definition {
            OfflineRegionDefinition::TilePyramid {
                style_url,
                bounds,
                min_zoom,
                max_zoom,
                pixel_ratio,
                include_ideographs,
            } => Ok(Self::TilePyramid {
                style_url: string::c_string(style_url)?,
                bounds: *bounds,
                min_zoom: *min_zoom,
                max_zoom: *max_zoom,
                pixel_ratio: *pixel_ratio,
                include_ideographs: *include_ideographs,
            }),
            OfflineRegionDefinition::GeometryRegion {
                style_url,
                geometry,
                min_zoom,
                max_zoom,
                pixel_ratio,
                include_ideographs,
            } => Ok(Self::GeometryRegion {
                style_url: string::c_string(style_url)?,
                geometry: crate::geometry::geometry_try_to_native(geometry)?,
                min_zoom: *min_zoom,
                max_zoom: *max_zoom,
                pixel_ratio: *pixel_ratio,
                include_ideographs: *include_ideographs,
            }),
        }
    }

    pub fn to_raw(&self) -> sys::mln_offline_region_definition {
        match self {
            Self::TilePyramid {
                style_url,
                bounds,
                min_zoom,
                max_zoom,
                pixel_ratio,
                include_ideographs,
            } => sys::mln_offline_region_definition {
                size: std::mem::size_of::<sys::mln_offline_region_definition>() as u32,
                type_: sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
                data: sys::mln_offline_region_definition__bindgen_ty_1 {
                    tile_pyramid: sys::mln_offline_tile_pyramid_region_definition {
                        size: std::mem::size_of::<sys::mln_offline_tile_pyramid_region_definition>()
                            as u32,
                        style_url: style_url.as_ptr(),
                        bounds: crate::values::lat_lng_bounds_to_native(*bounds),
                        min_zoom: *min_zoom,
                        max_zoom: *max_zoom,
                        pixel_ratio: *pixel_ratio,
                        include_ideographs: *include_ideographs,
                    },
                },
            },
            Self::GeometryRegion {
                style_url,
                geometry,
                min_zoom,
                max_zoom,
                pixel_ratio,
                include_ideographs,
            } => sys::mln_offline_region_definition {
                size: std::mem::size_of::<sys::mln_offline_region_definition>() as u32,
                type_: sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY,
                data: sys::mln_offline_region_definition__bindgen_ty_1 {
                    geometry: sys::mln_offline_geometry_region_definition {
                        size: std::mem::size_of::<sys::mln_offline_geometry_region_definition>()
                            as u32,
                        style_url: style_url.as_ptr(),
                        geometry: geometry.as_ptr(),
                        min_zoom: *min_zoom,
                        max_zoom: *max_zoom,
                        pixel_ratio: *pixel_ratio,
                        include_ideographs: *include_ideographs,
                    },
                },
            },
        }
    }
}

pub fn offline_region_definition_to_native(
    definition: &OfflineRegionDefinition,
) -> Result<NativeOfflineRegionDefinition> {
    NativeOfflineRegionDefinition::new(definition)
}

pub fn empty_offline_region_info() -> sys::mln_offline_region_info {
    sys::mln_offline_region_info {
        size: std::mem::size_of::<sys::mln_offline_region_info>() as u32,
        id: 0,
        definition: sys::mln_offline_region_definition {
            size: std::mem::size_of::<sys::mln_offline_region_definition>() as u32,
            type_: 0,
            data: sys::mln_offline_region_definition__bindgen_ty_1 {
                tile_pyramid: sys::mln_offline_tile_pyramid_region_definition {
                    size: std::mem::size_of::<sys::mln_offline_tile_pyramid_region_definition>()
                        as u32,
                    style_url: std::ptr::null(),
                    bounds: sys::mln_lat_lng_bounds {
                        southwest: sys::mln_lat_lng {
                            latitude: 0.0,
                            longitude: 0.0,
                        },
                        northeast: sys::mln_lat_lng {
                            latitude: 0.0,
                            longitude: 0.0,
                        },
                    },
                    min_zoom: 0.0,
                    max_zoom: 0.0,
                    pixel_ratio: 0.0,
                    include_ideographs: false,
                },
            },
        },
        metadata: std::ptr::null(),
        metadata_size: 0,
    }
}

pub fn copy_offline_region_info(raw: &sys::mln_offline_region_info) -> Result<OfflineRegionInfo> {
    Ok(OfflineRegionInfo {
        id: raw.id,
        definition: copy_offline_region_definition(&raw.definition)?,
        metadata: copy_metadata(raw.metadata, raw.metadata_size)?,
    })
}

pub fn copy_offline_region_definition(
    raw: &sys::mln_offline_region_definition,
) -> Result<OfflineRegionDefinition> {
    match raw.type_ {
        sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID => {
            // SAFETY: Active union member is selected by raw.type_.
            let tile = unsafe { raw.data.tile_pyramid };
            Ok(OfflineRegionDefinition::TilePyramid {
                // SAFETY: Native snapshot/list storage owns a NUL-terminated style URL.
                style_url: unsafe { string::copy_c_string(tile.style_url) }?,
                bounds: crate::values::lat_lng_bounds_from_native(tile.bounds),
                min_zoom: tile.min_zoom,
                max_zoom: tile.max_zoom,
                pixel_ratio: tile.pixel_ratio,
                include_ideographs: tile.include_ideographs,
            })
        }
        sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY => {
            // SAFETY: Active union member is selected by raw.type_.
            let geometry = unsafe { raw.data.geometry };
            if geometry.geometry.is_null() {
                return Err(crate::Error::invalid_argument(
                    "offline region geometry must not be null",
                ));
            }
            Ok(OfflineRegionDefinition::GeometryRegion {
                // SAFETY: Native snapshot/list storage owns a NUL-terminated style URL.
                style_url: unsafe { string::copy_c_string(geometry.style_url) }?,
                // SAFETY: geometry.geometry is non-null and borrowed from live snapshot/list storage.
                geometry: unsafe {
                    crate::geometry::geometry_from_native_with_depth(&*geometry.geometry, 0)
                }?,
                min_zoom: geometry.min_zoom,
                max_zoom: geometry.max_zoom,
                pixel_ratio: geometry.pixel_ratio,
                include_ideographs: geometry.include_ideographs,
            })
        }
        type_ => Err(crate::Error::invalid_argument(format!(
            "unknown offline region definition type: {type_}"
        ))),
    }
}

fn copy_metadata(ptr: *const u8, len: usize) -> Result<Vec<u8>> {
    if len == 0 {
        return Ok(Vec::new());
    }
    if ptr.is_null() {
        return Err(crate::Error::invalid_argument(
            "offline region metadata must not be null when size is nonzero",
        ));
    }
    // SAFETY: Metadata is borrowed from live snapshot/list storage and copied immediately.
    Ok(unsafe { std::slice::from_raw_parts(ptr, len) }.to_vec())
}

pub fn metadata_ptr(metadata: &[u8]) -> *const u8 {
    if metadata.is_empty() {
        std::ptr::null()
    } else {
        metadata.as_ptr()
    }
}

/// Copies an owned native offline-region snapshot into owned Rust data.
///
/// # Safety
///
/// `ptr` must point to a live `mln_offline_region_snapshot` handle owned by
/// the caller and returned by the matching C API. This function takes ownership
/// of that handle and releases it before returning, including on copy errors.
pub unsafe fn copy_offline_region_snapshot(
    ptr: NonNull<sys::mln_offline_region_snapshot>,
) -> Result<OfflineRegionInfo> {
    // SAFETY: ptr is an owned snapshot returned by the C API and released by the guard.
    let snapshot = unsafe { crate::handle::offline_region_snapshot(ptr.as_ptr()) }?;
    let mut raw = empty_offline_region_info();
    // SAFETY: snapshot is live and raw points to writable storage with size initialized.
    crate::check(unsafe { sys::mln_offline_region_snapshot_get(snapshot.as_ptr(), &mut raw) })?;
    copy_offline_region_info(&raw)
}

/// Copies an owned native offline-region list into owned Rust data.
///
/// # Safety
///
/// `ptr` must point to a live `mln_offline_region_list` handle owned by the
/// caller and returned by the matching C API. This function takes ownership of
/// that handle and releases it before returning, including on copy errors.
pub unsafe fn copy_offline_region_list(
    ptr: NonNull<sys::mln_offline_region_list>,
) -> Result<Vec<OfflineRegionInfo>> {
    // SAFETY: ptr is an owned list returned by the C API and released by the guard.
    let list = unsafe { crate::handle::offline_region_list(ptr.as_ptr()) }?;
    let mut count = 0;
    // SAFETY: list is live and count points to writable storage.
    crate::check(unsafe { sys::mln_offline_region_list_count(list.as_ptr(), &mut count) })?;
    let mut regions = Vec::with_capacity(count);
    for index in 0..count {
        let mut raw = empty_offline_region_info();
        // SAFETY: list is live, index is in range, and raw points to writable storage.
        crate::check(unsafe { sys::mln_offline_region_list_get(list.as_ptr(), index, &mut raw) })?;
        regions.push(copy_offline_region_info(&raw)?);
    }
    Ok(regions)
}

#[derive(Debug)]
pub struct NativeRuntimeOptions {
    asset_path: Option<CString>,
    cache_path: Option<CString>,
    maximum_cache_size: Option<u64>,
}

impl NativeRuntimeOptions {
    fn new(options: &RuntimeOptions) -> Result<Self> {
        validate_abi_version()?;
        Ok(Self {
            asset_path: string::optional_c_string(options.asset_path.as_deref())?,
            cache_path: string::optional_c_string(options.cache_path.as_deref())?,
            maximum_cache_size: options.maximum_cache_size,
        })
    }

    pub fn to_raw(&self) -> sys::mln_runtime_options {
        // SAFETY: Default constructor takes no arguments and initializes size,
        // flags, and default values for this C ABI version.
        let mut raw = unsafe { sys::mln_runtime_options_default() };
        if let Some(asset_path) = &self.asset_path {
            raw.asset_path = asset_path.as_ptr();
        }
        if let Some(cache_path) = &self.cache_path {
            raw.cache_path = cache_path.as_ptr();
        }
        if let Some(maximum_cache_size) = self.maximum_cache_size {
            raw.flags |= sys::MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE;
            raw.maximum_cache_size = maximum_cache_size;
        }
        raw
    }
}

pub fn runtime_options_to_native(options: &RuntimeOptions) -> Result<NativeRuntimeOptions> {
    NativeRuntimeOptions::new(options)
}

#[doc(hidden)]
pub trait RuntimeOptionsNativeExt {
    fn to_native(&self) -> Result<NativeRuntimeOptions>;
}

impl RuntimeOptionsNativeExt for RuntimeOptions {
    fn to_native(&self) -> Result<NativeRuntimeOptions> {
        runtime_options_to_native(self)
    }
}

#[doc(hidden)]
pub trait OfflineRegionDefinitionNativeExt {
    fn to_native(&self) -> Result<NativeOfflineRegionDefinition>;
}

impl OfflineRegionDefinitionNativeExt for OfflineRegionDefinition {
    fn to_native(&self) -> Result<NativeOfflineRegionDefinition> {
        offline_region_definition_to_native(self)
    }
}

#[cfg(test)]
mod tests {
    use std::ffi::CStr;
    use std::ptr;

    use super::*;

    #[test]
    // Spec coverage: BND-060 and BND-061.
    fn runtime_options_materializes_flags_strings_and_defaults() {
        let native = runtime_options_to_native(&RuntimeOptions {
            asset_path: Some("assets".into()),
            cache_path: Some("cache.db".into()),
            maximum_cache_size: Some(42),
        })
        .unwrap();

        let raw = native.to_raw();

        assert_eq!(
            raw.size,
            std::mem::size_of::<sys::mln_runtime_options>() as u32
        );
        assert_eq!(raw.flags, sys::MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE);
        assert_eq!(raw.maximum_cache_size, 42);
        assert!(!raw.asset_path.is_null());
        assert!(!raw.cache_path.is_null());
        // SAFETY: native owns the C strings referenced by raw for this scope.
        assert_eq!(
            unsafe { CStr::from_ptr(raw.asset_path) }.to_str().unwrap(),
            "assets"
        );
        // SAFETY: native owns the C strings referenced by raw for this scope.
        assert_eq!(
            unsafe { CStr::from_ptr(raw.cache_path) }.to_str().unwrap(),
            "cache.db"
        );
    }

    #[test]
    fn runtime_options_leave_absent_fields_null_and_unflagged() {
        let native = runtime_options_to_native(&RuntimeOptions::default()).unwrap();
        let raw = native.to_raw();

        assert_eq!(raw.flags & sys::MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE, 0);
        assert_eq!(raw.asset_path, ptr::null());
        assert_eq!(raw.cache_path, ptr::null());
    }

    #[test]
    fn offline_tile_pyramid_definition_materializes_size_string_and_bounds() {
        let definition = OfflineRegionDefinition::TilePyramid {
            style_url: "maplibre://style".to_owned(),
            bounds: crate::LatLngBounds::new(
                crate::LatLng::new(1.0, 2.0),
                crate::LatLng::new(3.0, 4.0),
            ),
            min_zoom: 5.0,
            max_zoom: 6.0,
            pixel_ratio: 2.0,
            include_ideographs: true,
        };
        let native = offline_region_definition_to_native(&definition).unwrap();
        let raw = native.to_raw();

        assert_eq!(
            raw.size,
            std::mem::size_of::<sys::mln_offline_region_definition>() as u32
        );
        assert_eq!(raw.type_, sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID);
        // SAFETY: Active union field is selected by raw.type_, and native owns the style URL.
        let tile = unsafe { raw.data.tile_pyramid };
        assert_eq!(
            tile.size,
            std::mem::size_of::<sys::mln_offline_tile_pyramid_region_definition>() as u32
        );
        // SAFETY: native owns the C string referenced by tile for this scope.
        assert_eq!(
            unsafe { CStr::from_ptr(tile.style_url) }.to_str().unwrap(),
            "maplibre://style"
        );
        assert_eq!(tile.bounds.southwest.latitude, 1.0);
        assert_eq!(tile.bounds.northeast.longitude, 4.0);
    }

    #[test]
    fn offline_geometry_definition_materializes_nested_geometry_pointer() {
        let definition = OfflineRegionDefinition::GeometryRegion {
            style_url: "maplibre://geometry".to_owned(),
            geometry: crate::Geometry::Point(crate::LatLng::new(1.0, 2.0)),
            min_zoom: 3.0,
            max_zoom: 4.0,
            pixel_ratio: 1.5,
            include_ideographs: false,
        };
        let native = offline_region_definition_to_native(&definition).unwrap();
        let raw = native.to_raw();

        assert_eq!(raw.type_, sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY);
        // SAFETY: Active union field is selected by raw.type_.
        let geometry = unsafe { raw.data.geometry };
        assert_eq!(
            geometry.size,
            std::mem::size_of::<sys::mln_offline_geometry_region_definition>() as u32
        );
        assert!(!geometry.geometry.is_null());
    }

    #[test]
    fn offline_region_info_copy_survives_backing_storage_changes() {
        let mut style_url = b"maplibre://a\0".to_vec();
        let mut metadata = [1_u8, 2, 3];
        let raw = sys::mln_offline_region_info {
            size: std::mem::size_of::<sys::mln_offline_region_info>() as u32,
            id: 7,
            definition: sys::mln_offline_region_definition {
                size: std::mem::size_of::<sys::mln_offline_region_definition>() as u32,
                type_: sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
                data: sys::mln_offline_region_definition__bindgen_ty_1 {
                    tile_pyramid: sys::mln_offline_tile_pyramid_region_definition {
                        size: std::mem::size_of::<sys::mln_offline_tile_pyramid_region_definition>()
                            as u32,
                        style_url: style_url.as_ptr().cast(),
                        bounds: crate::values::lat_lng_bounds_to_native(crate::LatLngBounds::new(
                            crate::LatLng::new(1.0, 2.0),
                            crate::LatLng::new(3.0, 4.0),
                        )),
                        min_zoom: 5.0,
                        max_zoom: 6.0,
                        pixel_ratio: 2.0,
                        include_ideographs: true,
                    },
                },
            },
            metadata: metadata.as_ptr(),
            metadata_size: metadata.len(),
        };

        let copied = copy_offline_region_info(&raw).unwrap();
        style_url[12] = b'b';
        metadata[0] = 9;
        assert_eq!(metadata[0], 9);

        assert_eq!(copied.id, 7);
        assert_eq!(copied.metadata, vec![1, 2, 3]);
        assert_eq!(
            copied.definition,
            OfflineRegionDefinition::TilePyramid {
                style_url: "maplibre://a".to_owned(),
                bounds: crate::LatLngBounds::new(
                    crate::LatLng::new(1.0, 2.0),
                    crate::LatLng::new(3.0, 4.0),
                ),
                min_zoom: 5.0,
                max_zoom: 6.0,
                pixel_ratio: 2.0,
                include_ideographs: true,
            }
        );
    }

    #[test]
    fn offline_region_info_rejects_nonempty_null_metadata() {
        let style_url = b"maplibre://a\0";
        let raw = sys::mln_offline_region_info {
            size: std::mem::size_of::<sys::mln_offline_region_info>() as u32,
            id: 7,
            definition: sys::mln_offline_region_definition {
                size: std::mem::size_of::<sys::mln_offline_region_definition>() as u32,
                type_: sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
                data: sys::mln_offline_region_definition__bindgen_ty_1 {
                    tile_pyramid: sys::mln_offline_tile_pyramid_region_definition {
                        size: std::mem::size_of::<sys::mln_offline_tile_pyramid_region_definition>()
                            as u32,
                        style_url: style_url.as_ptr().cast(),
                        bounds: crate::values::lat_lng_bounds_to_native(crate::LatLngBounds::new(
                            crate::LatLng::new(1.0, 2.0),
                            crate::LatLng::new(3.0, 4.0),
                        )),
                        min_zoom: 5.0,
                        max_zoom: 6.0,
                        pixel_ratio: 2.0,
                        include_ideographs: true,
                    },
                },
            },
            metadata: std::ptr::null(),
            metadata_size: 1,
        };

        let Err(err) = copy_offline_region_info(&raw) else {
            panic!("nonempty null metadata should fail");
        };
        assert!(
            err.to_string()
                .contains("offline region metadata must not be null")
        );
    }

    #[test]
    fn offline_region_definition_rejects_null_geometry_pointer() {
        let style_url = b"maplibre://geometry\0";
        let raw = sys::mln_offline_region_definition {
            size: std::mem::size_of::<sys::mln_offline_region_definition>() as u32,
            type_: sys::MLN_OFFLINE_REGION_DEFINITION_GEOMETRY,
            data: sys::mln_offline_region_definition__bindgen_ty_1 {
                geometry: sys::mln_offline_geometry_region_definition {
                    size: std::mem::size_of::<sys::mln_offline_geometry_region_definition>() as u32,
                    style_url: style_url.as_ptr().cast(),
                    geometry: std::ptr::null(),
                    min_zoom: 1.0,
                    max_zoom: 4.0,
                    pixel_ratio: 1.0,
                    include_ideographs: false,
                },
            },
        };

        let Err(err) = copy_offline_region_definition(&raw) else {
            panic!("null offline geometry pointer should fail");
        };
        assert!(
            err.to_string()
                .contains("offline region geometry must not be null")
        );
    }
}
