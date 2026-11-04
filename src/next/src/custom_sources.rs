// Ports the custom source half of maplibre-native-ffi src/c_api/style.cpp and
// src/map/style.cpp (add_callback_source, validate_custom_geometry_source_
// options through map_invalidate_custom_vector_source_tile) and the C++
// core's CustomTileLoader (src/mln/style/custom_tile_loader.cpp).
//
// maplibre-native-next 0.0.6 has no custom sources: no CustomGeometrySource,
// CustomVectorSource, or render sources for them. A custom geometry source is
// built here on the core's GeoJSON source: its data (`CustomTileData`) is the
// tiles that the host delivers. The renderer asks it for a tile, which it
// answers from the delivered tiles, or asks the host for with fetch_tile. A
// delivery, failure or invalidation replaces the data, so the renderer asks
// again for every tile it shows. Deviations from C++:
// - cancel_tile runs for invalidated tiles only, not when the renderer
//   drops a tile;
// - a failed tile renders as an empty one, until an invalidation refetches
//   it;
// - its source info reports MLN_STYLE_SOURCE_TYPE_CUSTOM_GEOMETRY, but the
//   renderer treats it as a GeoJSON source.
// A custom vector source would need a vector render source fed by the host,
// which the core lacks: mln_map_add_custom_vector_source validates its input
// and returns MLN_STATUS_UNSUPPORTED, so no custom vector source exists.
//
// The host's user_data is released when the last holder of its
// `Registration` drops: the style's source, or an accepted command whose
// source the style didn't take. A rejected call never releases it.

use crate::delivery::{UserData, Value, deliver};
use crate::diagnostics::{Failure, boundary};
use crate::geojson_data;
use crate::handles;
use crate::map::command;
use crate::style_sources::{
    add_source, ensure_no_source, non_empty_bytes, non_empty_text, source_missing, utf8, validate_lat_lng, view_bytes,
    with_style,
};
use crate::types::*;
use maplibre_native_next::geojson::GeoJSON;
use maplibre_native_next::geojson_vt;
use maplibre_native_next::mln_renderer::util::tile_range::TileRange;
use maplibre_native_next::mln_storage::FileSource;
use maplibre_native_next::mln_style::Style;
use maplibre_native_next::mln_style::layer::LayerTypeInfo;
use maplibre_native_next::mln_style::source::{Source, SourceBase};
use maplibre_native_next::mln_style::source_impl;
use maplibre_native_next::mln_style::sources::geojson_source::{
    Features, GeoJSONData, GeoJSONOptions, GeoJSONSource, TileFeatures,
};
use maplibre_native_next::mln_tile_id::CanonicalTileID;
use maplibre_native_next::mln_util::constants;
use maplibre_native_next::mln_util::geo::LatLngBounds;
use maplibre_native_next::mln_util::logging::{Event, Log};
use std::any::Any;
use std::collections::HashMap;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};

// MARK: - Validation (C++ map/style.cpp)

fn validate_zoom(zoom: f64, name: &str) -> Result<(), Failure> {
    if zoom.is_finite() && (0.0..=32.0).contains(&zoom) && zoom.floor() == zoom {
        Ok(())
    } else {
        Err(Failure::invalid_argument(format!("{name} must be an integer within [0, 32]")))
    }
}

/// C++ `validate_canonical_tile_id`.
fn canonical_tile_id(tile_id: mln_canonical_tile_id) -> Result<CanonicalTileID, Failure> {
    if tile_id.z > 32 {
        return Err(Failure::invalid_argument("tile_id.z must be within [0, 32]"));
    }
    let limit = 1u64 << tile_id.z;
    if u64::from(tile_id.x) >= limit || u64::from(tile_id.y) >= limit {
        return Err(Failure::invalid_argument("tile_id x and y must be within zoom bounds"));
    }
    Ok(CanonicalTileID { z: tile_id.z as u8, x: tile_id.x, y: tile_id.y })
}

/// C++ `validate_lat_lng_bounds` and `to_native_lat_lng_bounds`.
fn lat_lng_bounds(bounds: mln_lat_lng_bounds) -> Result<LatLngBounds, Failure> {
    let southwest = validate_lat_lng(bounds.southwest)?;
    let northeast = validate_lat_lng(bounds.northeast)?;
    if bounds.southwest.latitude > bounds.northeast.latitude || bounds.southwest.longitude > bounds.northeast.longitude
    {
        return Err(Failure::invalid_argument("bounds southwest must be less than or equal to northeast"));
    }
    Ok(LatLngBounds::hull(southwest, northeast))
}

/// C++ `resolve_registration_queue("custom source", ...)`.
fn registration_queue(queue: mln_delivery_queue, has_callback: bool) -> Result<u64, Failure> {
    if !has_callback {
        return Err(Failure::invalid_argument("custom source requires a callback"));
    }
    if queue.0 != 0 {
        handles::queue(queue)?;
    }
    Ok(queue.0)
}

/// C++ `effective_custom_geometry_source_options`, after
/// `validate_custom_geometry_source_options`.
struct GeometryOptions {
    min_zoom: u8,
    max_zoom: u8,
    tolerance: f64,
    tile_size: u32,
    buffer: u32,
    clip: bool,
    wrap: bool,
}

const KNOWN_GEOMETRY_FIELDS: u32 = MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
    | MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
    | MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
    | MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
    | MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
    | MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
    | MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;

fn geometry_options(options: &mln_custom_geometry_source_options) -> Result<GeometryOptions, Failure> {
    if options.fields & !KNOWN_GEOMETRY_FIELDS != 0 {
        return Err(Failure::invalid_argument("mln_custom_geometry_source_options.fields contains unknown bits"));
    }
    let has = |field: u32| options.fields & field != 0;
    // C++ custom_geometry_source_options_default.
    let (mut min_zoom, mut max_zoom, mut tolerance) = (0.0, 18.0, 0.375);
    let (mut tile_size, mut buffer, mut clip, mut wrap) = (u32::from(constants::TILE_SIZE_I), 128, false, false);
    if has(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM) {
        validate_zoom(options.min_zoom, "min_zoom")?;
        min_zoom = options.min_zoom;
    }
    if has(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM) {
        validate_zoom(options.max_zoom, "max_zoom")?;
        max_zoom = options.max_zoom;
    }
    if has(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE) {
        tolerance = options.tolerance;
    }
    if has(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE) {
        tile_size = options.tile_size;
    }
    if has(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER) {
        buffer = options.buffer;
    }
    if has(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP) {
        clip = options.clip;
    }
    if has(MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP) {
        wrap = options.wrap;
    }
    if min_zoom > max_zoom {
        return Err(Failure::invalid_argument("min_zoom must be less than or equal to max_zoom"));
    }
    if !(tolerance.is_finite() && tolerance >= 0.0) {
        return Err(Failure::invalid_argument("tolerance must be finite and non-negative"));
    }
    if tile_size == 0 || tile_size > 65535 {
        return Err(Failure::invalid_argument("tile_size must be within [1, 65535]"));
    }
    if buffer > 65535 {
        return Err(Failure::invalid_argument("buffer must be at most 65535"));
    }
    Ok(GeometryOptions { min_zoom: min_zoom as u8, max_zoom: max_zoom as u8, tolerance, tile_size, buffer, clip, wrap })
}

// MARK: - The tile loader (C++ CustomTileLoader)

/// The host's tile callbacks.
#[derive(Clone, Copy)]
struct Callbacks {
    fetch: unsafe extern "C" fn(*mut std::ffi::c_void, mln_canonical_tile_id),
    cancel: mln_custom_source_cancel_tile_callback,
    user_data: UserData,
    queue: u64,
}

fn c_tile_id(id: CanonicalTileID) -> mln_canonical_tile_id {
    mln_canonical_tile_id { z: u32::from(id.z), x: id.x, y: id.y }
}

impl Callbacks {
    fn fetch(self, id: CanonicalTileID) {
        deliver(
            self.queue,
            Box::new(move || {
                // SAFETY: the host registered this callback and user_data, which
                // stay valid until their release, which no call follows.
                unsafe { (self.fetch)(self.user_data.get(), c_tile_id(id)) };
            }),
        );
    }

    fn cancel(self, id: CanonicalTileID) {
        if let Some(cancel) = self.cancel {
            let user_data = self.user_data;
            deliver(
                self.queue,
                Box::new(move || {
                    // SAFETY: as for `fetch`.
                    unsafe { cancel(user_data.get(), c_tile_id(id)) };
                }),
            );
        }
    }
}

/// What the loader knows of a tile that the renderer asked for (C++
/// `tileCallbackMap` and `dataCache`).
enum TileState {
    Requested,
    Loaded(Arc<TileFeatures>),
    Failed,
}

struct Loader {
    source_id: String,
    /// The host's callbacks, until the release takes them: a call starts
    /// under this lock, so none starts after the release.
    callbacks: Mutex<Option<Callbacks>>,
    tiles: Mutex<HashMap<CanonicalTileID, TileState>>,
    tile_options: geojson_vt::TileOptions,
    wrap: bool,
    clip: bool,
}

impl Loader {
    fn with_callbacks(&self, f: impl FnOnce(Callbacks)) {
        if let Some(callbacks) = *self.callbacks.lock().unwrap() {
            f(callbacks);
        }
    }

    /// C++ `fetchTile`: a delivered tile's features, or `None` while the
    /// host has yet to deliver it, which asks the host the first time.
    fn tile(&self, id: CanonicalTileID) -> Option<TileFeatures> {
        let mut tiles = self.tiles.lock().unwrap();
        match tiles.get(&id) {
            Some(TileState::Loaded(features)) => Some((**features).clone()),
            Some(TileState::Failed) => Some(TileFeatures::new()),
            Some(TileState::Requested) => None,
            None => {
                tiles.insert(id, TileState::Requested);
                drop(tiles);
                self.with_callbacks(|callbacks| callbacks.fetch(id));
                None
            }
        }
    }

    /// C++ `setTileData` and `CustomGeometryTile::setTileData`: whether the
    /// renderer asked for the tile, which then holds `geojson` cut to it.
    fn set_data(&self, id: CanonicalTileID, geojson: &GeoJSON) -> bool {
        let mut tiles = self.tiles.lock().unwrap();
        let Some(state) = tiles.get_mut(&id) else {
            return false;
        };
        let tile = geojson_vt::geojson_to_tile(geojson, id.z, id.x, id.y, &self.tile_options, self.wrap, self.clip);
        *state = TileState::Loaded(Arc::new(tile.features));
        true
    }

    /// C++ `setTileError`: the error goes to the log, as the renderer's
    /// report of a failed tile does.
    fn set_error(&self, id: CanonicalTileID, message: &str) -> bool {
        let mut tiles = self.tiles.lock().unwrap();
        let Some(state) = tiles.get_mut(&id) else {
            return false;
        };
        *state = TileState::Failed;
        drop(tiles);
        Log::error(
            Event::Style,
            format!("Failed to load tile {id}=>{} for source {}: {message}", id.z, self.source_id),
        );
        true
    }

    /// C++ `invalidateTile`: forgets the tile, so the renderer's next request
    /// fetches it again.
    fn invalidate(&self, id: CanonicalTileID) -> bool {
        let removed = self.tiles.lock().unwrap().remove(&id).is_some();
        if removed {
            self.with_callbacks(|callbacks| callbacks.cancel(id));
        }
        removed
    }

    /// C++ `invalidateRegion`.
    fn invalidate_region(&self, bounds: &LatLngBounds) -> bool {
        let mut ranges = HashMap::new();
        let mut removed = Vec::new();
        self.tiles.lock().unwrap().retain(|id, _| {
            let range = ranges.entry(id.z).or_insert_with(|| TileRange::from_lat_lng_bounds(bounds, id.z));
            let inside = range.contains(id);
            if inside {
                removed.push(*id);
            }
            !inside
        });
        for id in &removed {
            self.with_callbacks(|callbacks| callbacks.cancel(*id));
        }
        !removed.is_empty()
    }
}

/// The data of a custom geometry source's GeoJSON source. Each change to the
/// loader's tiles installs a new one, which makes the renderer ask for every
/// tile again.
struct CustomTileData {
    loader: Arc<Loader>,
}

impl std::fmt::Debug for CustomTileData {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("CustomTileData")
    }
}

impl GeoJSONData for CustomTileData {
    fn get_tile(&self, id: &CanonicalTileID, callback: Box<dyn FnOnce(TileFeatures)>, _run_synchronously: bool) {
        // A tile the host hasn't delivered stays loading: the next data asks
        // again.
        if let Some(features) = self.loader.tile(*id) {
            callback(features);
        }
    }

    fn get_children(&self, _cluster_id: u32) -> Features {
        Features::new()
    }

    fn get_leaves(&self, _cluster_id: u32, _limit: u32, _offset: u32) -> Features {
        Features::new()
    }

    fn get_cluster_expansion_zoom(&self, _cluster_id: u32) -> u8 {
        0
    }
}

// MARK: - The source

/// A custom source's hold on the host's user_data (C++
/// `OwnedCallbackSourceOptions`).
struct Registration {
    loader: Arc<Loader>,
    release_user_data: mln_user_data_release,
    user_data: UserData,
    queue: u64,
    /// Set once the add command is submitted; a rejected call leaves
    /// user_data with the host.
    accepted: AtomicBool,
}

impl Drop for Registration {
    fn drop(&mut self) {
        // No host callback starts after this.
        self.loader.callbacks.lock().unwrap().take();
        if !self.accepted.load(Ordering::SeqCst) {
            return;
        }
        if let Some(release) = self.release_user_data {
            let user_data = self.user_data;
            deliver(
                self.queue,
                Box::new(move || {
                    // SAFETY: the registration's release, called once, after its
                    // last callback.
                    unsafe { release(user_data.get()) };
                }),
            );
        }
    }
}

/// `style::CustomGeometrySource`, on a GeoJSON source.
pub struct CustomGeometrySource {
    geojson: GeoJSONSource,
    registration: Arc<Registration>,
}

impl std::fmt::Debug for CustomGeometrySource {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("CustomGeometrySource").field("geojson", &self.geojson).finish_non_exhaustive()
    }
}

impl CustomGeometrySource {
    /// Makes the renderer ask for every tile again.
    fn refresh(&self) {
        self.geojson.set_geojson_data(Some(Arc::new(CustomTileData { loader: self.registration.loader.clone() })));
    }
}

impl Source for CustomGeometrySource {
    fn base(&self) -> &SourceBase {
        self.geojson.base()
    }

    fn as_any(&self) -> &dyn Any {
        self
    }

    fn load_description(&self, file_source: &dyn FileSource) {
        // A GeoJSON source without a URL, which only marks itself loaded.
        self.geojson.load_description(file_source);
    }

    fn supports_layer_type(&self, type_info: &LayerTypeInfo) -> bool {
        self.geojson.supports_layer_type(type_info)
    }

    fn create_mutable(&self) -> Box<dyn source_impl::Impl> {
        self.geojson.create_mutable()
    }
}

/// Whether `source` is a custom geometry source.
pub fn is_custom_geometry_source(source: &dyn Source) -> bool {
    source.as_any().is::<CustomGeometrySource>()
}

/// C++ `lookup_custom_geometry_source`.
fn custom_geometry_source<'a>(style: &'a mut Style, id: &str) -> Result<&'a CustomGeometrySource, Failure> {
    style.impl_.mutated = true;
    let source = style.get_source(id).ok_or_else(source_missing)?;
    source
        .as_any()
        .downcast_ref::<CustomGeometrySource>()
        .ok_or_else(|| Failure::invalid_argument("source is not a custom geometry source"))
}

/// C++ `lookup_custom_vector_source`. No custom vector source exists (see
/// the top of this file).
fn custom_vector_source(style: &Style, id: &str) -> Result<(), Failure> {
    style.get_source(id).ok_or_else(source_missing)?;
    Err(Failure::invalid_argument("source is not a custom vector source"))
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_add_custom_geometry_source(
    map: mln_map,
    source_id: mln_buffer_view,
    options: *const mln_custom_geometry_source_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            if options.is_null() {
                return Err(Failure::invalid_argument("options must not be null"));
            }
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument("mln_custom_geometry_source_options.size is too small"));
            };
            let effective = geometry_options(&options)?;
            let queue = registration_queue(options.queue, options.fetch_tile.is_some())?;
            let Some(fetch) = options.fetch_tile else { unreachable!("registration_queue checks fetch_tile") };
            // C++ CustomGeometryTile::setTileData.
            let scale = f64::from(constants::EXTENT) / f64::from(effective.tile_size);
            let loader = Arc::new(Loader {
                source_id: id.clone(),
                callbacks: Mutex::new(Some(Callbacks {
                    fetch,
                    cancel: options.cancel_tile,
                    user_data: UserData(options.user_data),
                    queue,
                })),
                tiles: Mutex::new(HashMap::new()),
                tile_options: geojson_vt::TileOptions {
                    tolerance: scale * effective.tolerance,
                    extent: constants::EXTENT as u16,
                    buffer: (scale * f64::from(effective.buffer)).round() as u16,
                    line_metrics: false,
                },
                wrap: effective.wrap,
                clip: effective.clip,
            });
            let registration = Arc::new(Registration {
                loader,
                release_user_data: options.release_user_data,
                user_data: UserData(options.user_data),
                queue,
                accepted: AtomicBool::new(false),
            });
            let held = registration.clone();
            let (min_zoom, max_zoom) = (effective.min_zoom, effective.max_zoom);
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    ensure_no_source(style, &id)?;
                    let geojson = GeoJSONSource::new(
                        id,
                        Arc::new(GeoJSONOptions { minzoom: min_zoom, maxzoom: max_zoom, ..GeoJSONOptions::default() }),
                    );
                    let source = CustomGeometrySource { geojson, registration: held };
                    source.refresh();
                    add_source(style, Box::new(source))?;
                    Ok(Value::None)
                })
            })?;
            // The command may already have run; whichever of it and this call
            // drops the registration last releases user_data.
            registration.accepted.store(true, Ordering::SeqCst);
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_custom_geometry_source_tile_data(
    map: mln_map,
    source_id: mln_buffer_view,
    tile_id: mln_canonical_tile_id,
    data: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let data = non_empty_bytes(data, "data")?.to_vec();
            command(map, completion, true, move |entry| {
                let tile_id = canonical_tile_id(tile_id)?;
                let geojson = geojson_data::parse_geojson(Some(&data))?;
                with_style(entry, |style| {
                    let source = custom_geometry_source(style, &id)?;
                    if source.registration.loader.set_data(tile_id, &geojson) {
                        source.refresh();
                    }
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_custom_geometry_source_tile_error(
    map: mln_map,
    source_id: mln_buffer_view,
    tile_id: mln_canonical_tile_id,
    message: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            let message = utf8(view_bytes(message, "message")?, "message")?;
            command(map, completion, true, move |entry| {
                let tile_id = canonical_tile_id(tile_id)?;
                with_style(entry, |style| {
                    let source = custom_geometry_source(style, &id)?;
                    if source.registration.loader.set_error(tile_id, &message) {
                        source.refresh();
                    }
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_invalidate_custom_geometry_source_tile(
    map: mln_map,
    source_id: mln_buffer_view,
    tile_id: mln_canonical_tile_id,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            command(map, completion, true, move |entry| {
                let tile_id = canonical_tile_id(tile_id)?;
                with_style(entry, |style| {
                    let source = custom_geometry_source(style, &id)?;
                    if source.registration.loader.invalidate(tile_id) {
                        source.refresh();
                    }
                    Ok(Value::None)
                })
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_invalidate_custom_geometry_source_region(
    map: mln_map,
    source_id: mln_buffer_view,
    bounds: mln_lat_lng_bounds,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            command(map, completion, true, move |entry| {
                let bounds = lat_lng_bounds(bounds)?;
                with_style(entry, |style| {
                    let source = custom_geometry_source(style, &id)?;
                    if source.registration.loader.invalidate_region(&bounds) {
                        source.refresh();
                    }
                    Ok(Value::None)
                })
            })
        })
    }
}

const KNOWN_VECTOR_FIELDS: u32 = MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM | MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM;

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_add_custom_vector_source(
    map: mln_map,
    source_id: mln_buffer_view,
    options: *const mln_custom_vector_source_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            non_empty_text(source_id, "source_id")?;
            if options.is_null() {
                return Err(Failure::invalid_argument("options must not be null"));
            }
            let Some(options) = read_sized(options) else {
                return Err(Failure::invalid_argument("mln_custom_vector_source_options.size is too small"));
            };
            if options.fields & !KNOWN_VECTOR_FIELDS != 0 {
                return Err(Failure::invalid_argument("mln_custom_vector_source_options.fields contains unknown bits"));
            }
            let (mut min_zoom, mut max_zoom) = (0.0, 18.0);
            if options.fields & MLN_CUSTOM_VECTOR_SOURCE_OPTION_MIN_ZOOM != 0 {
                validate_zoom(options.min_zoom, "min_zoom")?;
                min_zoom = options.min_zoom;
            }
            if options.fields & MLN_CUSTOM_VECTOR_SOURCE_OPTION_MAX_ZOOM != 0 {
                validate_zoom(options.max_zoom, "max_zoom")?;
                max_zoom = options.max_zoom;
            }
            if min_zoom > max_zoom {
                return Err(Failure::invalid_argument("min_zoom must be less than or equal to max_zoom"));
            }
            registration_queue(options.queue, options.fetch_tile.is_some())?;
            handles::map(map)?;
            let _ = completion;
            // A failed call, so the caller keeps user_data.
            Err(Failure::new(
                MLN_STATUS_UNSUPPORTED,
                "custom vector sources are not implemented by maplibre-native-next yet: version 0.0.6 has no \
                 render source for vector tiles that the host delivers",
            ))
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_custom_vector_source_tile_data(
    map: mln_map,
    source_id: mln_buffer_view,
    tile_id: mln_canonical_tile_id,
    encoding: mln_style_vector_tile_encoding,
    data: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            if !matches!(encoding, MLN_STYLE_VECTOR_TILE_ENCODING_MVT | MLN_STYLE_VECTOR_TILE_ENCODING_MLT) {
                return Err(Failure::invalid_argument("encoding is invalid"));
            }
            view_bytes(data, "data")?;
            command(map, completion, true, move |entry| {
                canonical_tile_id(tile_id)?;
                custom_vector_source(&entry.map.borrow().get_style(), &id)?;
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_custom_vector_source_tile_error(
    map: mln_map,
    source_id: mln_buffer_view,
    tile_id: mln_canonical_tile_id,
    message: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            view_bytes(message, "message")?;
            command(map, completion, true, move |entry| {
                canonical_tile_id(tile_id)?;
                custom_vector_source(&entry.map.borrow().get_style(), &id)?;
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_invalidate_custom_vector_source_tile(
    map: mln_map,
    source_id: mln_buffer_view,
    tile_id: mln_canonical_tile_id,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(source_id, "source_id")?;
            command(map, completion, true, move |entry| {
                canonical_tile_id(tile_id)?;
                custom_vector_source(&entry.map.borrow().get_style(), &id)?;
                Ok(Value::None)
            })
        })
    }
}
