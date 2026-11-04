// Ports the offline region and ambient cache half of maplibre-native-ffi
// src/runtime/runtime.cpp (the *_start functions, their validation, the
// conversions to and from the C structs, and OfflineRegionRuntimeObserver)
// onto the runtime's database file source (database_file_source.rs).
//
// An operation is accepted on the caller's thread, finds the runtime's
// database file source on the runtime thread (which keeps it open until the
// runtime closes), and runs on the database thread, which completes it. No
// caller waits for either thread.

use crate::database_file_source::{DatabaseFileSource, DatabaseThread};
use crate::delivery::{Completion, LentValue, Value};
use crate::diagnostics::{Failure, boundary};
use crate::handles;
use crate::resource_loader::ResourceState;
use crate::runtime::{OwnedEvent, Payload, RuntimeShared, STATE};
use crate::types::*;
use crate::url_rules::validate_c_string_view;
use maplibre_native_next::geojson::{self, GeoJSON, Geometry};
use maplibre_native_next::mln_storage::offline::{
    OfflineGeometryRegionDefinition, OfflineRegion, OfflineRegionDefinition, OfflineRegionDownloadState,
    OfflineRegionObserver, OfflineRegionStatus, OfflineTilePyramidRegionDefinition,
};
use maplibre_native_next::mln_storage::offline_database::DatabaseError;
use maplibre_native_next::mln_storage::response::{Error as ResponseError, Reason};
use maplibre_native_next::mln_storage::sqlite3::{Database, OpenFlag};
use maplibre_native_next::mln_storage::{ClientOptions, FileSource, FileSourceManager, FileSourceType};
use maplibre_native_next::mln_util::geo::{LatLng, LatLngBounds};
use std::ffi::c_void;
use std::rc::Rc;
use std::sync::Arc;

/// `OfflineRegionData`: a region, as the C structs lend it.
struct RegionData {
    id: i64,
    definition_type: u32,
    style_url: LentText,
    bounds: mln_lat_lng_bounds,
    min_zoom: f64,
    max_zoom: f64,
    pixel_ratio: f32,
    include_ideographs: bool,
    geometry: LentText,
    metadata: Vec<u8>,
}

/// `to_c_region_data`.
fn region_data(region: &OfflineRegion) -> RegionData {
    let definition = region.get_definition();
    let mut data = RegionData {
        id: region.get_id(),
        definition_type: MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
        style_url: LentText::new(definition.style_url()),
        bounds: mln_lat_lng_bounds {
            southwest: mln_lat_lng { latitude: 0.0, longitude: 0.0 },
            northeast: mln_lat_lng { latitude: 0.0, longitude: 0.0 },
        },
        min_zoom: definition.min_zoom(),
        max_zoom: definition.max_zoom(),
        pixel_ratio: definition.pixel_ratio(),
        include_ideographs: definition.include_ideographs(),
        geometry: LentText::new(""),
        metadata: region.get_metadata().clone(),
    };
    match definition {
        OfflineRegionDefinition::TilePyramid(tile) => {
            data.bounds = mln_lat_lng_bounds {
                southwest: mln_lat_lng { latitude: tile.bounds.south(), longitude: tile.bounds.west() },
                northeast: mln_lat_lng { latitude: tile.bounds.north(), longitude: tile.bounds.east() },
            };
        }
        OfflineRegionDefinition::Geometry(geometry) => {
            data.definition_type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
            data.geometry = LentText::new(geojson::stringify_geometry(&geometry.geometry));
        }
    }
    data
}

/// `fill_region_info`.
fn region_info(data: &RegionData) -> mln_offline_region_info {
    let style_url = mln_buffer_view::of_text(&data.style_url);
    let definition_data = if data.definition_type == MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID {
        mln_offline_region_definition_data {
            tile_pyramid: mln_offline_tile_pyramid_region_definition {
                style_url,
                bounds: data.bounds,
                min_zoom: data.min_zoom,
                max_zoom: data.max_zoom,
                pixel_ratio: data.pixel_ratio,
                include_ideographs: data.include_ideographs,
            },
        }
    } else {
        mln_offline_region_definition_data {
            geometry: mln_offline_geometry_region_definition {
                style_url,
                geometry: mln_buffer_view::of_text(&data.geometry),
                min_zoom: data.min_zoom,
                max_zoom: data.max_zoom,
                pixel_ratio: data.pixel_ratio,
                include_ideographs: data.include_ideographs,
            },
        }
    };
    mln_offline_region_info {
        id: data.id,
        definition: mln_offline_region_definition {
            size: size_of::<mln_offline_region_definition>() as u32,
            type_: data.definition_type,
            data: definition_data,
        },
        metadata: if data.metadata.is_empty() { std::ptr::null() } else { data.metadata.as_ptr() },
        metadata_size: data.metadata.len(),
    }
}

/// Regions that a completion lends: one, none (an absent region) or a list.
struct RegionInfos {
    _regions: Vec<RegionData>,
    infos: Vec<mln_offline_region_info>,
    absent: bool,
}

// SAFETY: the infos point only into `_regions`, whose heap storage moves
// with it and which nothing else reaches.
unsafe impl Send for RegionInfos {}

impl RegionInfos {
    fn value(regions: &[OfflineRegion]) -> Value {
        let regions: Vec<RegionData> = regions.iter().map(region_data).collect();
        let infos = regions.iter().map(region_info).collect();
        Value::Lent(Box::new(RegionInfos { _regions: regions, infos, absent: false }))
    }

    fn absent() -> Value {
        Value::Lent(Box::new(RegionInfos { _regions: Vec::new(), infos: Vec::new(), absent: true }))
    }
}

impl LentValue for RegionInfos {
    fn view(&self) -> (*const c_void, usize, usize) {
        if self.absent {
            return (std::ptr::null(), 0, 0);
        }
        let address = if self.infos.is_empty() { std::ptr::null() } else { self.infos.as_ptr().cast() };
        (address, size_of::<mln_offline_region_info>(), self.infos.len())
    }
}

struct StatusValue(mln_offline_region_status);

impl LentValue for StatusValue {
    fn view(&self) -> (*const c_void, usize, usize) {
        ((&raw const self.0).cast(), size_of::<mln_offline_region_status>(), 1)
    }
}

/// `to_c_status`.
fn status_of(status: &OfflineRegionStatus) -> mln_offline_region_status {
    mln_offline_region_status {
        download_state: match status.download_state {
            OfflineRegionDownloadState::Inactive => MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE,
            OfflineRegionDownloadState::Active => MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
        },
        completed_resource_count: status.completed_resource_count,
        completed_resource_size: status.completed_resource_size,
        completed_tile_count: status.completed_tile_count,
        required_tile_count: status.required_tile_count,
        completed_tile_size: status.completed_tile_size,
        required_resource_count: status.required_resource_count,
        required_resource_count_is_precise: status.required_resource_count_is_precise,
        complete: status.complete(),
    }
}

/// `to_c_resource_error_reason`.
fn reason_of(reason: Reason) -> u32 {
    match reason {
        Reason::Success => MLN_RESOURCE_ERROR_REASON_NONE,
        Reason::NotFound => MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
        Reason::Server => MLN_RESOURCE_ERROR_REASON_SERVER,
        Reason::Connection => MLN_RESOURCE_ERROR_REASON_CONNECTION,
        Reason::RateLimit => MLN_RESOURCE_ERROR_REASON_RATE_LIMIT,
        Reason::Other => MLN_RESOURCE_ERROR_REASON_OTHER,
    }
}

/// `OfflineRegionRuntimeObserver`: turns a download's progress into runtime
/// events, for a region that the runtime observes.
struct RegionObserver {
    state: Arc<ResourceState>,
    region_id: i64,
}

impl RegionObserver {
    /// `region_event_selected` + `push_offline_region_event`.
    fn emit(&self, type_: u32, message: String, payload: Payload) {
        let observed = self
            .state
            .observed_regions
            .lock()
            .unwrap()
            .as_ref()
            .is_some_and(|regions| regions.contains(&self.region_id));
        if !observed {
            return;
        }
        let Ok(runtime) = handles::runtime(self.state.runtime) else {
            return;
        };
        runtime.emit(OwnedEvent {
            type_,
            source_type: MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
            source: runtime.handle,
            generation: 0,
            code: 0,
            message,
            payload,
        });
    }
}

impl OfflineRegionObserver for RegionObserver {
    fn status_changed(&self, status: OfflineRegionStatus) {
        self.emit(
            MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED,
            String::new(),
            Payload::OfflineRegionStatus(mln_runtime_event_offline_region_status {
                region_id: self.region_id,
                status: status_of(&status),
            }),
        );
    }

    fn response_error(&self, error: ResponseError) {
        self.emit(
            MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR,
            error.message,
            Payload::OfflineRegionResponseError(mln_runtime_event_offline_region_response_error {
                region_id: self.region_id,
                reason: reason_of(error.reason),
            }),
        );
    }

    fn mapbox_tile_count_limit_exceeded(&self, limit: u64) {
        self.emit(
            MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED,
            String::new(),
            Payload::OfflineRegionTileCountLimit(mln_runtime_event_offline_region_tile_count_limit {
                region_id: self.region_id,
                limit,
            }),
        );
    }
}

/// `database_source_for_runtime`: the runtime's database file source, which
/// the runtime thread keeps from the first offline operation on.
fn database_for_runtime(runtime: &RuntimeShared) -> Option<Arc<dyn FileSource>> {
    if let Some(database) = STATE.with(|state| state.borrow().database.clone()) {
        return Some(database);
    }
    let database = FileSourceManager::get().get_file_source(
        FileSourceType::Database,
        &runtime.resource_options,
        &ClientOptions::default(),
    )?;
    database.as_any()?.downcast_ref::<DatabaseFileSource>()?;
    STATE.with(|state| state.borrow_mut().database = Some(database.clone()));
    Some(database)
}

/// `submit_offline_operation`: validates the completion, then runs
/// `schedule` on the runtime's database thread.
fn submit(
    runtime: &Arc<RuntimeShared>,
    completion: *const mln_completion,
    schedule: impl FnOnce(&DatabaseThread, Completion) + Send + 'static,
) -> Result<(), Failure> {
    // SAFETY: C API contract for completions.
    let completion = unsafe { Completion::required(completion) }?;
    let shared = runtime.clone();
    runtime.post(move || {
        let database = database_for_runtime(&shared);
        match database.as_ref().and_then(|database| database.as_any()?.downcast_ref::<DatabaseFileSource>()) {
            Some(database) => database.run(move |thread| schedule(thread, completion)),
            None => completion.fail(MLN_STATUS_NATIVE_ERROR, "database file source is unavailable"),
        }
    });
    Ok(())
}

/// `complete_from_exception`.
fn complete_from(completion: Completion, error: Option<DatabaseError>) {
    match error {
        None => completion.ok(),
        Some(error) => completion.fail(MLN_STATUS_NATIVE_ERROR, error.to_string()),
    }
}

/// The region `region_id`, or the completion's failure: NOT_FOUND for a
/// missing region.
fn find_region(thread: &DatabaseThread, region_id: i64) -> Result<OfflineRegion, Failure> {
    match thread.db.borrow_mut().get_region(region_id) {
        Ok(Some(region)) => Ok(region),
        Ok(None) => Err(Failure::new(MLN_STATUS_NOT_FOUND, "offline region not found")),
        Err(error) => Err(Failure::new(MLN_STATUS_NATIVE_ERROR, error.to_string())),
    }
}

fn fail(completion: Completion, failure: Failure) {
    completion.fail(failure.status, failure.message);
}

/// The region operations' common start: the runtime, then the operation on
/// the region `region_id` once it exists.
fn with_region(
    runtime: mln_runtime,
    region_id: i64,
    completion: *const mln_completion,
    operation: impl FnOnce(&DatabaseThread, OfflineRegion, Completion) + Send + 'static,
) -> Result<(), Failure> {
    let shared = handles::runtime(runtime)?;
    submit(&shared, completion, move |thread, completion| match find_region(thread, region_id) {
        Ok(region) => operation(thread, region, completion),
        Err(failure) => fail(completion, failure),
    })
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_run_ambient_cache_operation(
    runtime: mln_runtime,
    operation: mln_ambient_cache_operation,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            if !matches!(
                operation,
                MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE
                    | MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE
                    | MLN_AMBIENT_CACHE_OPERATION_INVALIDATE
                    | MLN_AMBIENT_CACHE_OPERATION_CLEAR
            ) {
                return Err(Failure::invalid_argument("ambient cache operation is invalid"));
            }
            submit(&shared, completion, move |thread, completion| {
                let mut db = thread.db.borrow_mut();
                let error = match operation {
                    MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE => db.reset_database(),
                    MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE => db.pack(),
                    MLN_AMBIENT_CACHE_OPERATION_INVALIDATE => db.invalidate_ambient_cache(),
                    _ => db.clear_ambient_cache(),
                };
                drop(db);
                complete_from(completion, error);
            })
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_set_maximum_ambient_cache_size(
    runtime: mln_runtime,
    size: u64,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            submit(&shared, completion, move |thread, completion| {
                let error = thread.db.borrow_mut().set_maximum_ambient_cache_size(size);
                complete_from(completion, error);
            })
        })
    }
}

fn valid_coordinate(coordinate: &mln_lat_lng) -> bool {
    coordinate.latitude.is_finite() && (-90.0..=90.0).contains(&coordinate.latitude) && coordinate.longitude.is_finite()
}

fn check_zoom_and_ratio(min_zoom: f64, max_zoom: f64, pixel_ratio: f32) -> Result<(), Failure> {
    if !min_zoom.is_finite() || min_zoom < 0.0 || max_zoom.is_nan() || max_zoom < min_zoom {
        return Err(Failure::invalid_argument("offline region zoom range is invalid"));
    }
    if !pixel_ratio.is_finite() || pixel_ratio < 0.0 {
        return Err(Failure::invalid_argument("offline region pixel_ratio is invalid"));
    }
    Ok(())
}

/// `to_native_geometry`: one GeoJSON geometry.
fn geometry_of(view: mln_buffer_view) -> Result<Geometry<f64>, Failure> {
    // SAFETY: C API contract for buffer views.
    let bytes = unsafe { view.bytes() };
    if view.size == 0 {
        return Err(Failure::invalid_argument("geometry must not be empty"));
    }
    let Some(bytes) = bytes else {
        return Err(Failure::invalid_argument("geometry data must not be null"));
    };
    if bytes.contains(&0) {
        return Err(Failure::invalid_argument("geometry must not contain embedded NUL"));
    }
    let text = std::str::from_utf8(bytes)
        .map_err(|_| Failure::invalid_argument("geometry is not valid JSON: invalid UTF-8"))?;
    match geojson::parse(text) {
        Ok(GeoJSON::Geometry(geometry)) => Ok(geometry),
        Ok(_) => Err(Failure::invalid_argument("geometry must contain one GeoJSON Geometry")),
        Err(error) => Err(Failure::invalid_argument(format!("geometry is invalid: {}", error.message()))),
    }
}

/// Whether a geometry has a coordinate (`geometry_lat_lngs` is not empty).
fn has_coordinate(geometry: &Geometry<f64>) -> bool {
    match geometry {
        Geometry::Empty => false,
        Geometry::Point(_) => true,
        Geometry::MultiPoint(points) | Geometry::LineString(points) => !points.is_empty(),
        Geometry::MultiLineString(lines) | Geometry::Polygon(lines) => lines.iter().any(|line| !line.is_empty()),
        Geometry::MultiPolygon(polygons) => polygons.iter().flatten().any(|ring| !ring.is_empty()),
        Geometry::GeometryCollection(geometries) => geometries.iter().any(has_coordinate),
    }
}

fn style_url_of(view: mln_buffer_view) -> Result<String, Failure> {
    let bytes = validate_c_string_view(view, "offline region style_url")?;
    Ok(String::from_utf8_lossy(bytes).into_owned())
}

/// `validate_offline_region_definition` + `to_native_offline_region_definition`.
///
/// # Safety
/// `definition` is null or points at a readable definition.
unsafe fn definition_of(definition: *const mln_offline_region_definition) -> Result<OfflineRegionDefinition, Failure> {
    if definition.is_null() {
        return Err(Failure::invalid_argument("offline region definition must not be null"));
    }
    // SAFETY: the caller's contract.
    let Some(definition) = (unsafe { read_sized(definition) }) else {
        return Err(Failure::invalid_argument("mln_offline_region_definition.size is too small"));
    };
    let invalid = |error: maplibre_native_next::mln_storage::offline::RegionDefinitionError| {
        Failure::invalid_argument(error.what().to_owned())
    };
    match definition.type_ {
        MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID => {
            // SAFETY: the type names the union's member.
            let tile = unsafe { definition.data.tile_pyramid };
            let style_url = style_url_of(tile.style_url)?;
            let (southwest, northeast) = (tile.bounds.southwest, tile.bounds.northeast);
            if !valid_coordinate(&southwest)
                || !valid_coordinate(&northeast)
                || southwest.latitude > northeast.latitude
                || southwest.longitude > northeast.longitude
            {
                return Err(Failure::invalid_argument("offline region bounds are invalid"));
            }
            check_zoom_and_ratio(tile.min_zoom, tile.max_zoom, tile.pixel_ratio)?;
            let lat_lng = |coordinate: mln_lat_lng| {
                LatLng::new(coordinate.latitude, coordinate.longitude)
                    .map_err(|_| Failure::invalid_argument("offline region bounds are invalid"))
            };
            let bounds = LatLngBounds::hull(lat_lng(southwest)?, lat_lng(northeast)?);
            Ok(OfflineTilePyramidRegionDefinition::new(
                style_url,
                bounds,
                tile.min_zoom,
                tile.max_zoom,
                tile.pixel_ratio,
                tile.include_ideographs,
            )
            .map_err(invalid)?
            .into())
        }
        MLN_OFFLINE_REGION_DEFINITION_GEOMETRY => {
            // SAFETY: the type names the union's member.
            let shape = unsafe { definition.data.geometry };
            let style_url = style_url_of(shape.style_url)?;
            check_zoom_and_ratio(shape.min_zoom, shape.max_zoom, shape.pixel_ratio)?;
            let geometry = geometry_of(shape.geometry)?;
            if !has_coordinate(&geometry) {
                return Err(Failure::invalid_argument("offline region geometry must contain at least one coordinate"));
            }
            Ok(OfflineGeometryRegionDefinition::new(
                style_url,
                geometry,
                shape.min_zoom,
                shape.max_zoom,
                shape.pixel_ratio,
                shape.include_ideographs,
            )
            .map_err(invalid)?
            .into())
        }
        _ => Err(Failure::invalid_argument("offline region definition type is invalid")),
    }
}

/// # Safety
/// `metadata` points at `size` readable bytes, or `size` is 0.
unsafe fn metadata_of(metadata: *const u8, size: usize) -> Result<Vec<u8>, Failure> {
    if metadata.is_null() && size != 0 {
        return Err(Failure::invalid_argument("offline region metadata must not be null when non-empty"));
    }
    if size == 0 {
        return Ok(Vec::new());
    }
    // SAFETY: the caller's contract.
    Ok(unsafe { std::slice::from_raw_parts(metadata, size) }.to_vec())
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_create_offline_region(
    runtime: mln_runtime,
    definition: *const mln_offline_region_definition,
    metadata: *const u8,
    metadata_size: usize,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            let definition = definition_of(definition)?;
            let metadata = metadata_of(metadata, metadata_size)?;
            submit(&shared, completion, move |thread, completion| {
                let created = thread.db.borrow_mut().create_region(&definition, &metadata);
                match created {
                    Ok(region) => completion.ok_with(0, RegionInfos::value(std::slice::from_ref(&region))),
                    Err(error) => completion.fail(MLN_STATUS_NATIVE_ERROR, error.to_string()),
                }
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_get_offline_region(
    runtime: mln_runtime,
    region_id: mln_offline_region_id,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            submit(&shared, completion, move |thread, completion| {
                let region = thread.db.borrow_mut().get_region(region_id);
                match region {
                    Ok(Some(region)) => completion.ok_with(0, RegionInfos::value(std::slice::from_ref(&region))),
                    Ok(None) => completion.ok_with(0, RegionInfos::absent()),
                    Err(error) => completion.fail(MLN_STATUS_NATIVE_ERROR, error.to_string()),
                }
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_list_offline_regions(
    runtime: mln_runtime,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            submit(&shared, completion, move |thread, completion| {
                let regions = thread.db.borrow_mut().list_regions();
                match regions {
                    Ok(regions) => completion.ok_with(0, RegionInfos::value(&regions)),
                    Err(error) => completion.fail(MLN_STATUS_NATIVE_ERROR, error.to_string()),
                }
            })
        })
    }
}

/// `validate_offline_side_database_path`: opening the side database blocks,
/// so it happens on the calling thread, and a failure is the call's own
/// diagnostic.
fn validate_side_database_path(view: mln_buffer_view) -> Result<String, Failure> {
    let path = validate_c_string_view(view, "side_database_path")?;
    let path = String::from_utf8_lossy(path).into_owned();
    let unreadable =
        || Failure::invalid_argument("side database path must identify an existing readable database file");
    if path.is_empty() || path.starts_with(':') || path.starts_with("file:") || !std::path::Path::new(&path).is_file() {
        return Err(unreadable());
    }
    Database::try_open(&path, OpenFlag::ReadOnly).map_err(|_| unreadable())?;
    Ok(path)
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_merge_offline_regions(
    runtime: mln_runtime,
    side_database_path: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            let path = validate_side_database_path(side_database_path)?;
            submit(&shared, completion, move |thread, completion| {
                let merged = thread.db.borrow_mut().merge_database(&path);
                match merged {
                    Ok(regions) => completion.ok_with(0, RegionInfos::value(&regions)),
                    Err(error) => completion.fail(MLN_STATUS_NATIVE_ERROR, error.to_string()),
                }
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_update_offline_region_metadata(
    runtime: mln_runtime,
    region_id: mln_offline_region_id,
    metadata: *const u8,
    metadata_size: usize,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let shared = handles::runtime(runtime)?;
            let metadata = metadata_of(metadata, metadata_size)?;
            submit(&shared, completion, move |thread, completion| {
                let updated = thread.db.borrow_mut().update_metadata(region_id, &metadata);
                if let Err(error) = updated {
                    return completion.fail(MLN_STATUS_NATIVE_ERROR, error.to_string());
                }
                match find_region(thread, region_id) {
                    Ok(region) => completion.ok_with(0, RegionInfos::value(std::slice::from_ref(&region))),
                    Err(failure) => fail(completion, failure),
                }
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_get_offline_region_status(
    runtime: mln_runtime,
    region_id: mln_offline_region_id,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            with_region(runtime, region_id, completion, move |thread, region, completion| {
                match thread.region_status(region.get_id()) {
                    Ok(status) => completion.ok_with(0, Value::Lent(Box::new(StatusValue(status_of(&status))))),
                    Err(error) => completion.fail(MLN_STATUS_NATIVE_ERROR, error.to_string()),
                }
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_set_offline_region_observed(
    runtime: mln_runtime,
    region_id: mln_offline_region_id,
    observed: bool,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let state = handles::runtime(runtime)?.resources.clone();
            with_region(runtime, region_id, completion, move |thread, _region, completion| {
                {
                    let mut regions = state.observed_regions.lock().unwrap();
                    let Some(regions) = regions.as_mut() else {
                        // The runtime is closing.
                        return completion.ok();
                    };
                    if observed {
                        regions.insert(region_id);
                    } else {
                        regions.remove(&region_id);
                    }
                }
                let observer =
                    observed.then(|| Rc::new(RegionObserver { state, region_id }) as Rc<dyn OfflineRegionObserver>);
                thread.set_region_observer(region_id, observer);
                completion.ok();
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_set_offline_region_download_state(
    runtime: mln_runtime,
    region_id: mln_offline_region_id,
    state: mln_offline_region_download_state,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            handles::runtime(runtime)?;
            let state = match state {
                MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE => OfflineRegionDownloadState::Inactive,
                MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE => OfflineRegionDownloadState::Active,
                _ => return Err(Failure::invalid_argument("offline region download state is invalid")),
            };
            with_region(runtime, region_id, completion, move |thread, _region, completion| {
                thread.set_region_download_state(region_id, state);
                completion.ok();
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_invalidate_offline_region(
    runtime: mln_runtime,
    region_id: mln_offline_region_id,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            with_region(runtime, region_id, completion, move |thread, region, completion| {
                let error = thread.db.borrow_mut().invalidate_region(region.get_id());
                complete_from(completion, error);
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_delete_offline_region(
    runtime: mln_runtime,
    region_id: mln_offline_region_id,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let state = handles::runtime(runtime)?.resources.clone();
            with_region(runtime, region_id, completion, move |thread, region, completion| {
                if let Some(regions) = state.observed_regions.lock().unwrap().as_mut() {
                    regions.remove(&region_id);
                }
                thread.set_region_observer(region_id, None);
                thread.set_region_download_state(region_id, OfflineRegionDownloadState::Inactive);
                thread.forget_download(region_id);
                let error = thread.db.borrow_mut().delete_region(region);
                complete_from(completion, error);
            })
        })
    }
}
