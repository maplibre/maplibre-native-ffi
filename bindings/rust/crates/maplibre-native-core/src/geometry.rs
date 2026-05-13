use std::ptr;

use maplibre_native_sys as sys;

use crate::values::{lat_lng_from_native, lat_lng_to_native};
use crate::{Error, LatLng, Result};

pub const MAX_GEOMETRY_COLLECTION_DEPTH: usize = 64;

/// Owned geometry descriptor.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum Geometry {
    Empty,
    Point(LatLng),
    LineString(Vec<LatLng>),
    Polygon(Vec<Vec<LatLng>>),
    MultiPoint(Vec<LatLng>),
    MultiLineString(Vec<Vec<LatLng>>),
    MultiPolygon(Vec<Vec<Vec<LatLng>>>),
    GeometryCollection(Vec<Geometry>),
}

impl Geometry {
    fn to_native(&self) -> NativeGeometry {
        NativeGeometry::new(self)
    }

    fn try_to_native(&self) -> Result<NativeGeometry> {
        self.try_to_native_with_depth(0)
    }

    fn try_to_native_with_depth(&self, depth: usize) -> Result<NativeGeometry> {
        self.validate_depth(depth)?;
        Ok(self.to_native())
    }

    fn validate_depth(&self, depth: usize) -> Result<()> {
        check_geometry_depth(depth)?;
        if let Self::GeometryCollection(children) = self {
            for child in children {
                child.validate_depth(depth + 1)?;
            }
        }
        Ok(())
    }

    /// Copies a borrowed native geometry descriptor into an owned Rust value.
    ///
    /// # Safety
    ///
    /// `raw` and all nested pointers must be valid for the duration of this
    /// call. The returned value owns all copied data.
    unsafe fn from_native(raw: &sys::mln_geometry) -> Result<Self> {
        // SAFETY: The caller promises raw and nested pointers are valid for the
        // duration of this call. The helper copies recursively before return.
        unsafe { Self::from_native_with_depth(raw, 0) }
    }

    unsafe fn from_native_with_depth(raw: &sys::mln_geometry, depth: usize) -> Result<Self> {
        check_geometry_depth(depth)?;
        match raw.type_ {
            sys::MLN_GEOMETRY_TYPE_EMPTY => Ok(Self::Empty),
            sys::MLN_GEOMETRY_TYPE_POINT => {
                // SAFETY: The active union member is selected by raw.type_.
                Ok(Self::Point(lat_lng_from_native(unsafe { raw.data.point })))
            }
            sys::MLN_GEOMETRY_TYPE_LINE_STRING => {
                // SAFETY: The active union member is selected by raw.type_.
                let span = unsafe { raw.data.line_string };
                Ok(Self::LineString(copy_coordinate_span(span)?))
            }
            sys::MLN_GEOMETRY_TYPE_POLYGON => {
                // SAFETY: The active union member is selected by raw.type_.
                let polygon = unsafe { raw.data.polygon };
                Ok(Self::Polygon(copy_polygon_geometry(polygon)?))
            }
            sys::MLN_GEOMETRY_TYPE_MULTI_POINT => {
                // SAFETY: The active union member is selected by raw.type_.
                let span = unsafe { raw.data.multi_point };
                Ok(Self::MultiPoint(copy_coordinate_span(span)?))
            }
            sys::MLN_GEOMETRY_TYPE_MULTI_LINE_STRING => {
                // SAFETY: The active union member is selected by raw.type_.
                let multi_line = unsafe { raw.data.multi_line_string };
                Ok(Self::MultiLineString(copy_coordinate_spans(
                    multi_line.lines,
                    multi_line.line_count,
                    "multi-line string lines",
                )?))
            }
            sys::MLN_GEOMETRY_TYPE_MULTI_POLYGON => {
                // SAFETY: The active union member is selected by raw.type_.
                let multi_polygon = unsafe { raw.data.multi_polygon };
                let polygons = polygon_slice(
                    multi_polygon.polygons,
                    multi_polygon.polygon_count,
                    "multi-polygon polygons",
                )?;
                let mut copied = Vec::with_capacity(polygons.len());
                for polygon in polygons {
                    copied.push(copy_polygon_geometry(*polygon)?);
                }
                Ok(Self::MultiPolygon(copied))
            }
            sys::MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION => {
                // SAFETY: The active union member is selected by raw.type_.
                let collection = unsafe { raw.data.geometry_collection };
                let geometries = geometry_slice(
                    collection.geometries,
                    collection.geometry_count,
                    "geometry collection",
                )?;
                let mut copied = Vec::with_capacity(geometries.len());
                for geometry in geometries {
                    // SAFETY: geometries came from validated native collection storage.
                    copied.push(unsafe { Self::from_native_with_depth(geometry, depth + 1) }?);
                }
                Ok(Self::GeometryCollection(copied))
            }
            type_ => Err(Error::invalid_argument(format!(
                "unknown native geometry type: {type_}"
            ))),
        }
    }
}

pub fn geometry_try_to_native(value: &Geometry) -> Result<NativeGeometry> {
    value.try_to_native()
}

pub fn geometry_try_to_native_with_depth(value: &Geometry, depth: usize) -> Result<NativeGeometry> {
    value.try_to_native_with_depth(depth)
}

#[doc(hidden)]
pub trait GeometryNativeExt {
    fn try_to_native(&self) -> Result<NativeGeometry>;
}

impl GeometryNativeExt for Geometry {
    fn try_to_native(&self) -> Result<NativeGeometry> {
        geometry_try_to_native(self)
    }
}

/// Copies a borrowed native geometry descriptor into an owned Rust value.
///
/// # Safety
///
/// `raw` and all nested pointers must be valid for the duration of this call.
pub unsafe fn geometry_from_native(raw: &sys::mln_geometry) -> Result<Geometry> {
    // SAFETY: The caller promises raw and nested pointers are valid for this call.
    unsafe { Geometry::from_native(raw) }
}

/// Copies a borrowed native geometry descriptor into an owned Rust value.
///
/// # Safety
///
/// `raw` and all nested pointers must be valid for the duration of this call.
pub unsafe fn geometry_from_native_with_depth(
    raw: &sys::mln_geometry,
    depth: usize,
) -> Result<Geometry> {
    // SAFETY: The caller promises raw and nested pointers are valid for this call.
    unsafe { Geometry::from_native_with_depth(raw, depth) }
}

pub struct NativeGeometry {
    raw: sys::mln_geometry,
    coordinates: Vec<Box<[sys::mln_lat_lng]>>,
    spans: Vec<Box<[sys::mln_coordinate_span]>>,
    polygons: Vec<Box<[sys::mln_polygon_geometry]>>,
    geometries: Vec<Box<[sys::mln_geometry]>>,
    children: Vec<NativeGeometry>,
}

impl NativeGeometry {
    fn new(geometry: &Geometry) -> Self {
        let mut native = Self::empty(sys::MLN_GEOMETRY_TYPE_EMPTY);
        match geometry {
            Geometry::Empty => {}
            Geometry::Point(point) => {
                native.raw.type_ = sys::MLN_GEOMETRY_TYPE_POINT;
                native.raw.data = sys::mln_geometry__bindgen_ty_1 {
                    point: lat_lng_to_native(*point),
                };
            }
            Geometry::LineString(points) => {
                native.raw.type_ = sys::MLN_GEOMETRY_TYPE_LINE_STRING;
                let span = native.coordinate_span(points);
                native.raw.data = sys::mln_geometry__bindgen_ty_1 { line_string: span };
            }
            Geometry::Polygon(rings) => {
                native.raw.type_ = sys::MLN_GEOMETRY_TYPE_POLYGON;
                let polygon = native.polygon_geometry(rings);
                native.raw.data = sys::mln_geometry__bindgen_ty_1 { polygon };
            }
            Geometry::MultiPoint(points) => {
                native.raw.type_ = sys::MLN_GEOMETRY_TYPE_MULTI_POINT;
                let span = native.coordinate_span(points);
                native.raw.data = sys::mln_geometry__bindgen_ty_1 { multi_point: span };
            }
            Geometry::MultiLineString(lines) => {
                native.raw.type_ = sys::MLN_GEOMETRY_TYPE_MULTI_LINE_STRING;
                let line_spans = native.coordinate_spans(lines);
                native.raw.data = sys::mln_geometry__bindgen_ty_1 {
                    multi_line_string: sys::mln_multi_line_geometry {
                        lines: ptr_or_null(line_spans.as_ref()),
                        line_count: line_spans.len(),
                    },
                };
                native.spans.push(line_spans);
            }
            Geometry::MultiPolygon(polygons) => {
                native.raw.type_ = sys::MLN_GEOMETRY_TYPE_MULTI_POLYGON;
                let polygon_values = polygons
                    .iter()
                    .map(|rings| native.polygon_geometry(rings))
                    .collect::<Vec<_>>()
                    .into_boxed_slice();
                native.raw.data = sys::mln_geometry__bindgen_ty_1 {
                    multi_polygon: sys::mln_multi_polygon_geometry {
                        polygons: ptr_or_null(polygon_values.as_ref()),
                        polygon_count: polygon_values.len(),
                    },
                };
                native.polygons.push(polygon_values);
            }
            Geometry::GeometryCollection(children) => {
                native.raw.type_ = sys::MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION;
                native.children = children.iter().map(NativeGeometry::new).collect();
                let raw_children = native
                    .children
                    .iter()
                    .map(|child| child.raw)
                    .collect::<Vec<_>>()
                    .into_boxed_slice();
                native.raw.data = sys::mln_geometry__bindgen_ty_1 {
                    geometry_collection: sys::mln_geometry_collection {
                        geometries: ptr_or_null(raw_children.as_ref()),
                        geometry_count: raw_children.len(),
                    },
                };
                native.geometries.push(raw_children);
            }
        }
        native
    }

    fn empty(type_: u32) -> Self {
        Self {
            raw: sys::mln_geometry {
                size: std::mem::size_of::<sys::mln_geometry>() as u32,
                type_,
                data: sys::mln_geometry__bindgen_ty_1 {
                    point: sys::mln_lat_lng {
                        latitude: 0.0,
                        longitude: 0.0,
                    },
                },
            },
            coordinates: Vec::new(),
            spans: Vec::new(),
            polygons: Vec::new(),
            geometries: Vec::new(),
            children: Vec::new(),
        }
    }

    pub fn as_ptr(&self) -> *const sys::mln_geometry {
        &self.raw
    }

    fn coordinate_span(&mut self, points: &[LatLng]) -> sys::mln_coordinate_span {
        let coordinates = points
            .iter()
            .copied()
            .map(lat_lng_to_native)
            .collect::<Vec<_>>()
            .into_boxed_slice();
        let span = sys::mln_coordinate_span {
            coordinates: ptr_or_null(coordinates.as_ref()),
            coordinate_count: coordinates.len(),
        };
        self.coordinates.push(coordinates);
        span
    }

    fn coordinate_spans(&mut self, lines: &[Vec<LatLng>]) -> Box<[sys::mln_coordinate_span]> {
        lines
            .iter()
            .map(|line| self.coordinate_span(line))
            .collect::<Vec<_>>()
            .into_boxed_slice()
    }

    fn polygon_geometry(&mut self, rings: &[Vec<LatLng>]) -> sys::mln_polygon_geometry {
        let ring_spans = self.coordinate_spans(rings);
        let polygon = sys::mln_polygon_geometry {
            rings: ptr_or_null(ring_spans.as_ref()),
            ring_count: ring_spans.len(),
        };
        self.spans.push(ring_spans);
        polygon
    }
}

impl AsRef<sys::mln_geometry> for NativeGeometry {
    fn as_ref(&self) -> &sys::mln_geometry {
        &self.raw
    }
}

fn ptr_or_null<T>(values: &[T]) -> *const T {
    if values.is_empty() {
        ptr::null()
    } else {
        values.as_ptr()
    }
}

fn check_geometry_depth(depth: usize) -> Result<()> {
    if depth > MAX_GEOMETRY_COLLECTION_DEPTH {
        Err(Error::invalid_argument(format!(
            "geometry collection depth exceeds {MAX_GEOMETRY_COLLECTION_DEPTH}"
        )))
    } else {
        Ok(())
    }
}

fn copy_coordinate_span(span: sys::mln_coordinate_span) -> Result<Vec<LatLng>> {
    let coordinates = coordinate_slice(span.coordinates, span.coordinate_count, "coordinates")?;
    Ok(coordinates
        .iter()
        .copied()
        .map(lat_lng_from_native)
        .collect())
}

fn copy_coordinate_spans(
    spans: *const sys::mln_coordinate_span,
    count: usize,
    name: &'static str,
) -> Result<Vec<Vec<LatLng>>> {
    let spans = coordinate_span_slice(spans, count, name)?;
    spans.iter().copied().map(copy_coordinate_span).collect()
}

fn copy_polygon_geometry(polygon: sys::mln_polygon_geometry) -> Result<Vec<Vec<LatLng>>> {
    copy_coordinate_spans(polygon.rings, polygon.ring_count, "polygon rings")
}

fn coordinate_slice<'a>(
    ptr: *const sys::mln_lat_lng,
    count: usize,
    name: &'static str,
) -> Result<&'a [sys::mln_lat_lng]> {
    slice_or_empty(ptr, count, name)
}

fn coordinate_span_slice<'a>(
    ptr: *const sys::mln_coordinate_span,
    count: usize,
    name: &'static str,
) -> Result<&'a [sys::mln_coordinate_span]> {
    slice_or_empty(ptr, count, name)
}

fn polygon_slice<'a>(
    ptr: *const sys::mln_polygon_geometry,
    count: usize,
    name: &'static str,
) -> Result<&'a [sys::mln_polygon_geometry]> {
    slice_or_empty(ptr, count, name)
}

fn geometry_slice<'a>(
    ptr: *const sys::mln_geometry,
    count: usize,
    name: &'static str,
) -> Result<&'a [sys::mln_geometry]> {
    slice_or_empty(ptr, count, name)
}

fn slice_or_empty<'a, T>(ptr: *const T, count: usize, name: &'static str) -> Result<&'a [T]> {
    if count == 0 {
        return Ok(&[]);
    }
    if ptr.is_null() {
        return Err(Error::invalid_argument(format!(
            "{name} pointer must not be null when count is nonzero"
        )));
    }
    // SAFETY: The caller promises ptr points to count valid elements.
    Ok(unsafe { std::slice::from_raw_parts(ptr, count) })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn line_string_materializes_borrowed_coordinate_span() {
        let native =
            Geometry::LineString(vec![LatLng::new(1.0, 2.0), LatLng::new(3.0, 4.0)]).to_native();
        let raw = native.as_ref();

        assert_eq!(raw.type_, sys::MLN_GEOMETRY_TYPE_LINE_STRING);
        // SAFETY: The active union field is line_string because type_ was set
        // by the materializer above, and native owns the backing storage.
        let span = unsafe { raw.data.line_string };
        assert_eq!(span.coordinate_count, 2);
        assert!(!span.coordinates.is_null());
        // SAFETY: The span points to coordinate_count valid entries retained
        // by native for the duration of this test.
        let coordinates =
            unsafe { std::slice::from_raw_parts(span.coordinates, span.coordinate_count) };
        assert_eq!(coordinates[0].latitude, 1.0);
        assert_eq!(coordinates[1].longitude, 4.0);
    }

    #[test]
    fn geometry_collection_retains_child_storage() {
        let native = Geometry::GeometryCollection(vec![
            Geometry::Point(LatLng::new(1.0, 2.0)),
            Geometry::LineString(vec![LatLng::new(3.0, 4.0)]),
        ])
        .to_native();
        let raw = native.as_ref();

        assert_eq!(raw.type_, sys::MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION);
        // SAFETY: The active union field is geometry_collection because type_
        // was set by the materializer above.
        let collection = unsafe { raw.data.geometry_collection };
        assert_eq!(collection.geometry_count, 2);
        assert!(!collection.geometries.is_null());
    }

    #[test]
    fn geometry_copy_survives_backing_storage_changes() {
        let mut coordinates = [
            sys::mln_lat_lng {
                latitude: 1.0,
                longitude: 2.0,
            },
            sys::mln_lat_lng {
                latitude: 3.0,
                longitude: 4.0,
            },
        ];
        let raw = sys::mln_geometry {
            size: std::mem::size_of::<sys::mln_geometry>() as u32,
            type_: sys::MLN_GEOMETRY_TYPE_LINE_STRING,
            data: sys::mln_geometry__bindgen_ty_1 {
                line_string: sys::mln_coordinate_span {
                    coordinates: coordinates.as_ptr(),
                    coordinate_count: coordinates.len(),
                },
            },
        };

        // SAFETY: raw points to valid coordinate storage for this call.
        let copied = unsafe { Geometry::from_native(&raw) }.unwrap();
        coordinates[0].latitude = 10.0;
        coordinates[1].longitude = 40.0;
        assert_eq!(coordinates[1].longitude, 40.0);

        assert_eq!(
            copied,
            Geometry::LineString(vec![LatLng::new(1.0, 2.0), LatLng::new(3.0, 4.0)])
        );
    }

    #[test]
    fn geometry_collection_copies_back_before_native_storage_drops() {
        let geometry = Geometry::GeometryCollection(vec![
            Geometry::MultiPoint(vec![LatLng::new(1.0, 2.0), LatLng::new(3.0, 4.0)]),
            Geometry::Polygon(vec![vec![LatLng::new(5.0, 6.0), LatLng::new(7.0, 8.0)]]),
        ]);

        let copied = {
            let native = geometry_try_to_native(&geometry).unwrap();
            // SAFETY: native owns a valid descriptor graph for this copy.
            unsafe { geometry_from_native(native.as_ref()) }.unwrap()
        };

        assert_eq!(copied, geometry);
    }
}
