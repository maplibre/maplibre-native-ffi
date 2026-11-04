// Ports the source half of maplibre-native-ffi src/c_api/style.cpp and
// src/map/style.cpp (map_add_style_source_json through
// map_get_image_source_coordinates, and the validation they share) onto
// mln_style's `Style`: sources added as style-spec JSON, GeoJSON sources and
// image sources. Custom sources are in custom_sources.rs.

use crate::custom_sources;
use crate::delivery::{LentValue, Value};
use crate::diagnostics::{Failure, boundary};
use crate::geojson_data::{self, GeoJsonOptions};
use crate::map::{MapEntry, command};
use crate::types::*;
use maplibre_native_next::mln_style::Style;
use maplibre_native_next::mln_style::conversion::{Convertible, Error, convert_source};
use maplibre_native_next::mln_style::source::{self, Source};
use maplibre_native_next::mln_style::sources::geojson_source::GeoJSONSource;
use maplibre_native_next::mln_style::sources::image_source::ImageSource;
use maplibre_native_next::mln_style::sources::tile_source::{
    RasterDEMSource, RasterSource, TileSource, UrlOrTileset, VectorSource,
};
use maplibre_native_next::mln_style_types::SourceType;
use maplibre_native_next::mln_util::geo::LatLng;
use maplibre_native_next::mln_util::image::PremultipliedImage;
use maplibre_native_next::mln_util::size::Size;
use maplibre_native_next::mln_util::tileset::{RasterEncoding, Scheme, VectorEncoding};

// MARK: - Completion values

/// C structs that a completion lends, with the storage their views point
/// into (C++ `deliver_style_result`).
pub struct LentStructs {
    ptr: *const std::ffi::c_void,
    size: usize,
    count: usize,
    _storage: Box<dyn std::any::Any>,
}

// SAFETY: `ptr` and the views in the structs point into `_storage`, which
// moves with them and which nothing else references.
unsafe impl Send for LentStructs {}

impl LentStructs {
    /// The completion value that lends `structs`, keeping `storage` alive
    /// with them.
    ///
    /// # Safety
    /// Every pointer in `structs` points at static data or into heap memory
    /// that `storage` owns, which moving `storage` doesn't relocate.
    pub unsafe fn value<T: 'static, S: 'static>(structs: Vec<T>, storage: S) -> Value {
        let ptr = structs.as_ptr().cast();
        let count = structs.len();
        Value::Lent(Box::new(LentStructs { ptr, size: size_of::<T>(), count, _storage: Box::new((structs, storage)) }))
    }
}

impl LentValue for LentStructs {
    fn view(&self) -> (*const std::ffi::c_void, usize, usize) {
        (self.ptr, self.size, self.count)
    }
}

// MARK: - Input validation (C++ c_api/style.cpp, map/style.cpp)

/// C++ `valid_view`: the bytes of a view, which may be empty.
pub fn view_bytes<'a>(view: mln_buffer_view, name: &str) -> Result<&'a [u8], Failure> {
    // SAFETY: C API contract for buffer views.
    unsafe { view.bytes() }.ok_or_else(|| Failure::invalid_argument(format!("{name} is invalid")))
}

/// C++ `valid_non_empty_view`, which every ID, URL and JSON input takes.
pub fn non_empty_bytes<'a>(view: mln_buffer_view, name: &str) -> Result<&'a [u8], Failure> {
    if view.size == 0 {
        return Err(Failure::invalid_argument(format!("{name} must not be empty")));
    }
    view_bytes(view, name)
}

pub fn utf8(bytes: &[u8], name: &str) -> Result<String, Failure> {
    String::from_utf8(bytes.to_vec()).map_err(|_| Failure::invalid_argument(format!("{name} must be UTF-8")))
}

/// A non-empty view's text, such as an ID.
pub fn non_empty_text(view: mln_buffer_view, name: &str) -> Result<String, Failure> {
    utf8(non_empty_bytes(view, name)?, name)
}

/// A non-empty view's text that C++ also checks with
/// `validate_c_string_view`, such as a URL.
fn c_string_text(view: mln_buffer_view, name: &str) -> Result<String, Failure> {
    let bytes = non_empty_bytes(view, name)?;
    if bytes.contains(&0) {
        return Err(Failure::invalid_argument(format!("{name} contains embedded NUL")));
    }
    utf8(bytes, name)
}

/// C++ `validate_lat_lng`.
pub fn validate_lat_lng(coordinate: mln_lat_lng) -> Result<LatLng, Failure> {
    let mln_lat_lng { latitude, longitude } = coordinate;
    if !(latitude.is_finite() && (-90.0..=90.0).contains(&latitude) && longitude.is_finite()) {
        return Err(Failure::invalid_argument(
            "latitude must be finite and within [-90, 90], and longitude must be finite",
        ));
    }
    LatLng::new(latitude, longitude).map_err(Failure::invalid_argument)
}

fn c_lat_lng(coordinate: &LatLng) -> mln_lat_lng {
    mln_lat_lng { latitude: coordinate.latitude(), longitude: coordinate.longitude() }
}

/// C++ `validate_image_source_coordinates`: an image's four corners.
///
/// # Safety
/// `coordinates` is null or points at `count` readable `mln_lat_lng`s.
unsafe fn image_source_coordinates(coordinates: *const mln_lat_lng, count: usize) -> Result<[LatLng; 4], Failure> {
    if count != 4 {
        return Err(Failure::invalid_argument("image source coordinate_count must be 4"));
    }
    if coordinates.is_null() {
        return Err(Failure::invalid_argument("coordinates must not be null"));
    }
    // SAFETY: the caller's contract.
    let corners = unsafe { std::slice::from_raw_parts(coordinates, 4) };
    let mut result = [LatLng::default(); 4];
    for (out, corner) in result.iter_mut().zip(corners) {
        *out = validate_lat_lng(*corner)?;
    }
    let first = corners[0];
    if corners.iter().all(|corner| corner.latitude == first.latitude && corner.longitude == first.longitude) {
        return Err(Failure::invalid_argument("image source coordinates must not all match"));
    }
    Ok(result)
}

/// C++ `validate_premultiplied_rgba8_image` and
/// `to_native_premultiplied_rgba8_image`: a copy of a caller's image, with
/// its rows packed.
///
/// # Safety
/// `image` is null or points at a readable `mln_premultiplied_rgba8_image`
/// whose `pixels` holds `byte_length` readable bytes.
pub unsafe fn premultiplied_image(image: *const mln_premultiplied_rgba8_image) -> Result<PremultipliedImage, Failure> {
    if image.is_null() {
        return Err(Failure::invalid_argument("image must not be null"));
    }
    // SAFETY: the caller's contract.
    let Some(image) = (unsafe { read_sized(image) }) else {
        return Err(Failure::invalid_argument("mln_premultiplied_rgba8_image.size is too small"));
    };
    if image.width == 0 || image.height == 0 {
        return Err(Failure::invalid_argument("image dimensions must be positive"));
    }
    let Some(row_bytes) = image.width.checked_mul(4) else {
        return Err(Failure::invalid_argument("image row byte length overflows"));
    };
    if image.stride < row_bytes {
        return Err(Failure::invalid_argument("image stride must be at least width * 4"));
    }
    if image.pixels.is_null() {
        return Err(Failure::invalid_argument("image pixels must not be null"));
    }
    let (row_bytes, stride) = (row_bytes as usize, image.stride as usize);
    let required = (image.height as usize - 1).checked_mul(stride).and_then(|rows| rows.checked_add(row_bytes));
    if required.is_none_or(|required| image.byte_length < required) {
        return Err(Failure::invalid_argument("image byte_length is too small"));
    }
    // SAFETY: the caller's contract; byte_length covers every row.
    let input = unsafe { std::slice::from_raw_parts(image.pixels, image.byte_length) };
    let mut pixels = Vec::with_capacity(row_bytes * image.height as usize);
    for row in 0..image.height as usize {
        pixels.extend_from_slice(&input[row * stride..row * stride + row_bytes]);
    }
    Ok(PremultipliedImage::from_vec(Size::new(image.width, image.height), pixels))
}

// MARK: - Style access

/// Runs `f` on the map's style, for a change: the style's observer calls
/// reach the map when it returns.
pub fn with_style<R>(entry: &MapEntry, f: impl FnOnce(&mut Style) -> R) -> R {
    let mut map = entry.map.borrow_mut();
    let mut style = map.get_style_mut();
    f(&mut style)
}

pub fn source_missing() -> Failure {
    Failure::new(MLN_STATUS_NOT_FOUND, "source does not exist")
}

/// C++ `validate_source_can_be_added`.
pub fn ensure_no_source(style: &Style, id: &str) -> Result<(), Failure> {
    match style.get_source(id) {
        Some(_) => Err(Failure::invalid_argument("source already exists")),
        None => Ok(()),
    }
}

/// The source named `id` as a `T` (C++ `source->as<T>()`), or the C++
/// failure for a missing source or one of another kind.
///
/// The non-const C++ `Style::getSource` marks the style mutated, which a
/// source change does here too.
fn source_as<'a, T: 'static>(style: &'a mut Style, id: &str, kind: &str) -> Result<&'a T, Failure> {
    style.impl_.mutated = true;
    let source = style.get_source(id).ok_or_else(source_missing)?;
    source.as_any().downcast_ref::<T>().ok_or_else(|| Failure::invalid_argument(format!("source is not {kind}")))
}

pub fn add_source(style: &mut Style, source: Box<dyn Source>) -> Result<(), Failure> {
    style.add_source(source).map_err(|error| Failure::invalid_argument(error.to_string()))
}

// MARK: - Source info (C++ style_source_record)

/// A source's `mln_style_source_info` and the text its views lend.
struct SourceRecord {
    info: mln_style_source_info,
    _id: LentText,
    _attribution: LentText,
    _url: LentText,
    _tile_urls: Vec<LentText>,
    _tile_url_views: Vec<mln_buffer_view>,
}

fn c_source_type(source: &dyn Source) -> mln_style_source_type {
    if custom_sources::is_custom_geometry_source(source) {
        return MLN_STYLE_SOURCE_TYPE_CUSTOM_GEOMETRY;
    }
    match source.get_type() {
        SourceType::Vector => MLN_STYLE_SOURCE_TYPE_VECTOR,
        SourceType::Raster => MLN_STYLE_SOURCE_TYPE_RASTER,
        SourceType::RasterDEM => MLN_STYLE_SOURCE_TYPE_RASTER_DEM,
        SourceType::GeoJSON => MLN_STYLE_SOURCE_TYPE_GEO_JSON,
        SourceType::Video => MLN_STYLE_SOURCE_TYPE_VIDEO,
        SourceType::Annotations => MLN_STYLE_SOURCE_TYPE_ANNOTATIONS,
        SourceType::Image => MLN_STYLE_SOURCE_TYPE_IMAGE,
        SourceType::CustomVector => MLN_STYLE_SOURCE_TYPE_CUSTOM_GEOMETRY,
        SourceType::CustomMVTVector => MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR,
    }
}

fn c_vector_encoding(encoding: VectorEncoding) -> mln_style_vector_tile_encoding {
    match encoding {
        VectorEncoding::Mapbox => MLN_STYLE_VECTOR_TILE_ENCODING_MVT,
        VectorEncoding::MLT => MLN_STYLE_VECTOR_TILE_ENCODING_MLT,
    }
}

/// C++ `tile_source_from_source`, with a vector source's encoding.
fn tile_source(source: &dyn Source) -> Option<(&TileSource, Option<VectorEncoding>)> {
    let any = source.as_any();
    if let Some(vector) = any.downcast_ref::<VectorSource>() {
        return Some((vector.tile_source(), Some(vector.get_encoding())));
    }
    if let Some(raster) = any.downcast_ref::<RasterSource>() {
        return Some((raster.tile_source(), None));
    }
    any.downcast_ref::<RasterDEMSource>().map(|dem| (dem.tile_source(), None))
}

/// C++ `source_url`.
fn source_url(source: &dyn Source) -> Option<String> {
    if let Some((tile_source, _)) = tile_source(source) {
        return tile_source.get_url();
    }
    if let Some(geojson) = source.as_any().downcast_ref::<GeoJSONSource>() {
        return geojson.get_url();
    }
    source.as_any().downcast_ref::<ImageSource>().and_then(ImageSource::get_url)
}

/// C++ `style_source_record`.
fn source_record(source: &dyn Source) -> SourceRecord {
    // SAFETY: plain C data, for which all zeros is a valid value.
    let mut info: mln_style_source_info = unsafe { std::mem::zeroed() };
    info.type_ = c_source_type(source);
    info.is_volatile = source.is_volatile();
    let mut tile_urls = Vec::new();
    if let Some((tile_source, encoding)) = tile_source(source) {
        info.fields |= MLN_STYLE_SOURCE_INFO_TILE_SIZE;
        info.tile_size = u32::from(tile_source.get_tile_size());
        if let Some(encoding) = encoding {
            info.fields |= MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING;
            info.vector_encoding = c_vector_encoding(encoding);
        }
        if let UrlOrTileset::Tileset(tileset) = tile_source.get_url_or_tileset() {
            info.fields |= MLN_STYLE_SOURCE_INFO_TILE_JSON;
            info.tile_json.min_zoom = f64::from(tileset.zoom_range.min);
            info.tile_json.max_zoom = f64::from(tileset.zoom_range.max);
            info.tile_json.scheme = match tileset.scheme {
                Scheme::XYZ => MLN_STYLE_TILE_SCHEME_XYZ,
                Scheme::TMS => MLN_STYLE_TILE_SCHEME_TMS,
            };
            tile_urls = tileset.tiles.iter().map(|url| LentText::new(url.as_str())).collect();
            if let Some(bounds) = &tileset.bounds {
                info.fields |= MLN_STYLE_SOURCE_INFO_BOUNDS;
                info.bounds = mln_lat_lng_bounds {
                    southwest: mln_lat_lng { latitude: bounds.south(), longitude: bounds.west() },
                    northeast: mln_lat_lng { latitude: bounds.north(), longitude: bounds.east() },
                };
            }
            if let Some(encoding) = tileset.vector_encoding {
                info.fields |= MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING;
                info.vector_encoding = c_vector_encoding(encoding);
            }
            if let Some(encoding) = tileset.raster_encoding {
                info.fields |= MLN_STYLE_SOURCE_INFO_RASTER_ENCODING;
                info.raster_encoding = match encoding {
                    RasterEncoding::Mapbox => MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX,
                    RasterEncoding::Terrarium => MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM,
                };
            }
        }
    }
    let id = LentText::new(source.get_id());
    let attribution = LentText::new(source.get_attribution().unwrap_or_default());
    let url = LentText::new(source_url(source).unwrap_or_default());
    let tile_url_views: Vec<mln_buffer_view> = tile_urls.iter().map(mln_buffer_view::of_text).collect();
    info.id = mln_buffer_view::of_text(&id);
    info.attribution = mln_buffer_view::of_text(&attribution);
    info.url = mln_buffer_view::of_text(&url);
    if info.fields & MLN_STYLE_SOURCE_INFO_TILE_JSON != 0 {
        info.tile_json.tile_urls = if tile_url_views.is_empty() { std::ptr::null() } else { tile_url_views.as_ptr() };
        info.tile_json.tile_url_count = tile_url_views.len();
    }
    // The views point into the heap buffers of the texts and of
    // `tile_url_views`, which the record moves without relocating.
    SourceRecord {
        info,
        _id: id,
        _attribution: attribution,
        _url: url,
        _tile_urls: tile_urls,
        _tile_url_views: tile_url_views,
    }
}

fn lend_sources(records: Vec<SourceRecord>) -> Value {
    let infos: Vec<mln_style_source_info> = records.iter().map(|record| record.info).collect();
    // SAFETY: each info's views point into its record's heap buffers.
    unsafe { LentStructs::value(infos, records) }
}

// MARK: - Style sources

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_add_style_source_json(
    map: mln_map,
    source_id: mln_buffer_view,
    source_json: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let json = non_empty_bytes(source_json, "source_json")?.to_vec();
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    ensure_no_source(style, &id)?;
                    // C++ `convertJSON<std::unique_ptr<Source>>`; a JSON
                    // syntax error carries serde_json's message, as in the
                    // core.
                    let mut error = Error::default();
                    let source = match serde_json::from_slice::<serde_json::Value>(&json) {
                        Ok(document) => convert_source(&Convertible::from(&document), &mut error, &id),
                        Err(e) => {
                            error.message = e.to_string();
                            None
                        }
                    };
                    let Some(source) = source else {
                        return Err(Failure::invalid_argument(format!("style source: {}", error.message)));
                    };
                    add_source(style, source)?;
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_remove_style_source(
    map: mln_map,
    source_id: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            command(map, completion, true, move |entry| {
                let removed = with_style(entry, |style| {
                    if style.get_source(&id).is_none() {
                        return Err(source_missing());
                    }
                    style.remove_source(&id).ok_or_else(|| Failure::invalid_state("source is used by a layer"))
                })?;
                // Dropped once the style no longer holds it: a custom source's
                // release runs here.
                drop(removed);
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_style_source(
    map: mln_map,
    source_id: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            command(map, completion, false, move |entry| {
                let map = entry.map.borrow();
                let style = map.get_style();
                Ok(match style.get_source(&id) {
                    Some(source) => lend_sources(vec![source_record(source)]),
                    None => Value::None,
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_style_source_volatile(
    map: mln_map,
    source_id: mln_buffer_view,
    is_volatile: bool,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    style.impl_.mutated = true;
                    let source = style.get_source(&id).ok_or_else(source_missing)?;
                    source::set_volatile(source, is_volatile);
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_list_style_sources(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, false, |entry| {
                let map = entry.map.borrow();
                let style = map.get_style();
                Ok(lend_sources(style.get_sources().into_iter().map(source_record).collect()))
            })
        })
    }
}

// MARK: - GeoJSON sources

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_add_geo_json_source_url(
    map: mln_map,
    source_id: mln_buffer_view,
    url: mln_buffer_view,
    options: *const mln_geo_json_source_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let url = c_string_text(url, "url")?;
            let options = GeoJsonOptions::resolve(options)?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    ensure_no_source(style, &id)?;
                    let source = GeoJSONSource::new(id, options.to_native()?);
                    source.set_url(&url);
                    add_source(style, Box::new(source))?;
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_add_geo_json_source_data(
    map: mln_map,
    source_id: mln_buffer_view,
    data: mln_geo_json_source_data,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            // The command holds the prepared data, so the caller may release
            // its handle once this returns.
            let prepared = geojson_data::lookup(data)?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    ensure_no_source(style, &id)?;
                    let source = GeoJSONSource::new(id, prepared.options.clone());
                    source.set_geojson_data(Some(prepared.data.clone()));
                    add_source(style, Box::new(source))?;
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_geo_json_source_url(
    map: mln_map,
    source_id: mln_buffer_view,
    url: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let url = c_string_text(url, "url")?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    source_as::<GeoJSONSource>(style, &id, "a GeoJSON source")?.set_url(&url);
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_geo_json_source_data(
    map: mln_map,
    source_id: mln_buffer_view,
    data: mln_geo_json_source_data,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let prepared = geojson_data::lookup(data)?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    let source = source_as::<GeoJSONSource>(style, &id, "a GeoJSON source")?;
                    if !geojson_data::options_equal(&source.get_options(), &prepared.options) {
                        return Err(Failure::invalid_argument(format!(
                            "prepared GeoJSON data options do not match the options that GeoJSON source \"{id}\" \
                             was added with"
                        )));
                    }
                    source.set_geojson_data(Some(prepared.data.clone()));
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_geo_json_source_synchronous_tiling(
    map: mln_map,
    source_id: mln_buffer_view,
    enabled: bool,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    source_as::<GeoJSONSource>(style, &id, "a GeoJSON source")?
                        .set_override_synchronous_update(enabled);
                    Ok(Value::None)
                })
            })
        })
    }
}

// MARK: - Image sources

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_add_image_source_url(
    map: mln_map,
    source_id: mln_buffer_view,
    coordinates: *const mln_lat_lng,
    coordinate_count: usize,
    url: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let url = c_string_text(url, "url")?;
            let corners = image_source_coordinates(coordinates, coordinate_count)?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    ensure_no_source(style, &id)?;
                    let source = ImageSource::new(id, corners);
                    source.set_url(&url);
                    add_source(style, Box::new(source))?;
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_add_image_source_image(
    map: mln_map,
    source_id: mln_buffer_view,
    coordinates: *const mln_lat_lng,
    coordinate_count: usize,
    image: *const mln_premultiplied_rgba8_image,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let corners = image_source_coordinates(coordinates, coordinate_count)?;
            let image = premultiplied_image(image)?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    ensure_no_source(style, &id)?;
                    let source = ImageSource::new(id, corners);
                    source.set_image(image);
                    add_source(style, Box::new(source))?;
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_image_source_url(
    map: mln_map,
    source_id: mln_buffer_view,
    url: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let url = c_string_text(url, "url")?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    source_as::<ImageSource>(style, &id, "an image source")?.set_url(&url);
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_image_source_image(
    map: mln_map,
    source_id: mln_buffer_view,
    image: *const mln_premultiplied_rgba8_image,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let image = premultiplied_image(image)?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    source_as::<ImageSource>(style, &id, "an image source")?.set_image(image);
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_image_source_coordinates(
    map: mln_map,
    source_id: mln_buffer_view,
    coordinates: *const mln_lat_lng,
    coordinate_count: usize,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let corners = image_source_coordinates(coordinates, coordinate_count)?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    source_as::<ImageSource>(style, &id, "an image source")?.set_coordinates(corners);
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_image_source_coordinates(
    map: mln_map,
    source_id: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            command(map, completion, false, move |entry| {
                let map = entry.map.borrow();
                let style = map.get_style();
                let source = style.get_source(&id).ok_or_else(source_missing)?;
                let Some(image_source) = source.as_any().downcast_ref::<ImageSource>() else {
                    return Err(Failure::invalid_argument("source is not an image source"));
                };
                let corners: Vec<mln_lat_lng> = image_source.get_coordinates().iter().map(c_lat_lng).collect();
                // SAFETY: plain values without views.
                Ok(LentStructs::value(corners, ()))
            })
        })
    }
}
