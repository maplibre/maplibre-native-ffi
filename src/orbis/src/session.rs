// Render sessions: an Orbis target (a CAMetalLayer surface, or an RGBA8
// texture to read back) that a map draws into on its runtime's thread, the
// frame demands that drive it, and the frame results it reports.
//
// Orbis's runtime owns its GPU device, so a session renders on that device:
// an attach that names another device (an MTLDevice, an offscreen device of
// another adapter) isn't supported.

use std::collections::VecDeque;
use std::ptr::NonNull;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

use crate::base::{Failure, Result, UserData, boundary, put, sized};
use crate::delivery::{Completion, Value, Wake};
use crate::handles::{self, Object};
use crate::runtime::{Msg, Payload, RuntimeHandle, State};
use crate::types::*;

/// What the caller's threads share with the runtime thread.
pub struct SessionHandle {
    runtime: Arc<RuntimeHandle>,
    id: OnceLock<u64>,
    frame_wake: Option<Wake>,
    results: Mutex<Vec<mln_render_frame_result>>,
    /// Whether the frame wake was called since the last drain.
    notified: AtomicBool,
    /// A pacer's refresh not yet seen by the runtime thread.
    tick: AtomicBool,
    driver: mln_render_driver_kind,
    readback: bool,
    /// Fixed once it fails.
    failure: Mutex<Option<Failure>>,
    state: Mutex<mln_render_session_snapshot>,
}

impl SessionHandle {
    fn id(&self) -> u64 {
        *self.id.get().expect("session id")
    }

    fn push(&self, result: mln_render_frame_result) {
        self.results.lock().unwrap().push(result);
        if !self.notified.swap(true, Ordering::AcqRel)
            && let Some(wake) = &self.frame_wake
        {
            wake.signal();
        }
    }

    fn failed(&self) -> Result {
        match &*self.failure.lock().unwrap() {
            Some(f) => Err(f.clone()),
            None => Ok(()),
        }
    }

    fn command(
        &self,
        completion: Option<Completion>,
        job: impl FnOnce(&mut State, u64) -> Result<Value> + Send + 'static,
    ) -> Result {
        let id = self.id();
        self.runtime.post(move |state| {
            let result = if state.sessions.contains_key(&id) {
                job(state, id)
            } else {
                Err(Failure::state("the session was released"))
            };
            if let Some(c) = completion {
                c.complete(result, 0);
            }
        })
    }
}

pub struct SessionEntry {
    handle: Arc<SessionHandle>,
    map: u64,
    /// None once the target is released.
    target: Option<Target>,
    logical: mln_logical_size,
    scale: f64,
    demands: VecDeque<mln_frame_demand>,
    redraw_wake: Option<Wake>,
    redraw_signaled: bool,
    pacer: Option<Arc<FramePacer>>,
    frame_generation: u64,
    /// The last still image, which a readback returns.
    still: Option<orb_api::Image>,
}

impl Drop for SessionEntry {
    fn drop(&mut self) {
        if let Some(pacer) = self.pacer.take() {
            pacer.remove(&self.handle);
        }
        if let Some(wake) = self.redraw_wake.take() {
            wake.release();
        }
    }
}

impl SessionEntry {
    fn sync_state(&self) {
        let mut s = self.handle.state.lock().unwrap();
        s.extent = mln_logical_extent {
            width: self.logical.width,
            height: self.logical.height,
            scale_factor: self.scale,
        };
        s.frame_generation = self.frame_generation;
        s.has_target = self.target.is_some();
    }
}

fn physical(logical: mln_logical_size, scale: f64) -> Result<(u32, u32)> {
    crate::base::physical(mln_logical_extent {
        width: logical.width,
        height: logical.height,
        scale_factor: scale,
    })
}

/// A retain of the caller's layer, which an accepted attach takes so the
/// layer outlives the surface on it.
struct Retained(UserData);

#[cfg(target_vendor = "apple")]
#[link(name = "CoreFoundation", kind = "framework")]
unsafe extern "C" {
    fn CFRetain(object: *const std::ffi::c_void) -> *const std::ffi::c_void;
    fn CFRelease(object: *const std::ffi::c_void);
}

impl Retained {
    fn new(object: UserData) -> Self {
        // SAFETY: a live Objective-C object, by the C API's contract.
        #[cfg(target_vendor = "apple")]
        unsafe {
            CFRetain(object.get());
        }
        Self(object)
    }
}

impl Drop for Retained {
    fn drop(&mut self) {
        // SAFETY: the retain that `new` took.
        #[cfg(target_vendor = "apple")]
        unsafe {
            CFRelease(self.0.get());
        }
    }
}

/// A session's target, and the layer it draws into, released after it.
struct Target {
    target: orb_api::Target,
    _layer: Option<Retained>,
}

/// A CAMetalLayer as an Orbis window target.
fn metal_target(
    state: &State,
    layer: UserData,
    logical: mln_logical_size,
    scale: f64,
) -> Result<Target> {
    let (width, height) = physical(logical, scale)?;
    let layer = NonNull::new(layer.get()).ok_or_else(|| Failure::argument("the layer is null"))?;
    let surface = state.runtime.with_device(|device| match device {
        // SAFETY: the C API's contract: the layer is a live CAMetalLayer that
        // the caller keeps until the session's target is released.
        orb_api::DeviceRef::Modern(gpu) => {
            unsafe { gpu.create_surface_raw(orb_api::RawSurface::MetalLayer(layer), width, height) }
                .map(orb_api::Surface::Modern)
                .map_err(Failure::native)
        }
        orb_api::DeviceRef::Compat(_) => Err(Failure::unsupported(
            "Orbis's Compat tier draws into Android windows, not Metal layers",
        )),
    })?;
    Ok(Target {
        target: orb_api::Target::surface(&state.runtime, surface, scale),
        _layer: Some(Retained::new(UserData(layer.as_ptr()))),
    })
}

enum TargetSpec {
    Offscreen,
    Metal(UserData),
}

/// The attach of every target kind.
///
/// # Safety
/// The caller's pointers.
unsafe fn attach(
    map: mln_map,
    logical: mln_logical_size,
    spec: TargetSpec,
    options: *const mln_render_session_attach_options,
    out_session: *mut mln_render_session,
    completion: *const mln_completion,
) -> Result {
    let map_handle = handles::map(map.0)?;
    // SAFETY: the caller's pointers.
    let options = unsafe { sized(options, "options")? };
    if options.driver != MLN_RENDER_DRIVER_CORE_WORKER {
        return Err(Failure::unsupported(
            "Orbis sessions render on the runtime's thread (MLN_RENDER_DRIVER_CORE_WORKER)",
        ));
    }
    if options.producer_sync_kinds != 0
        && options.producer_sync_kinds & MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE == 0
    {
        return Err(Failure::unsupported(
            "Orbis sessions have no GPU sync objects to share",
        ));
    }
    if out_session.is_null() {
        return Err(Failure::argument("out_session is null"));
    }
    // SAFETY: as above.
    let completion = unsafe { Completion::from_c(completion)? };
    let readback = matches!(spec, TargetSpec::Offscreen);
    let handle = Arc::new(SessionHandle {
        runtime: map_handle.runtime.clone(),
        id: OnceLock::new(),
        frame_wake: Wake::from_c(&options.frame_wake),
        results: Mutex::default(),
        notified: AtomicBool::new(false),
        tick: AtomicBool::new(false),
        driver: options.driver,
        readback,
        failure: Mutex::new(None),
        state: Mutex::new(mln_render_session_snapshot {
            size: std::mem::size_of::<mln_render_session_snapshot>() as u32,
            state: MLN_RENDER_SESSION_STATE_ATTACHING,
            ..Default::default()
        }),
    });
    let id = handles::insert(Object::Session(handle.clone()));
    let _ = handle.id.set(id);
    let redraw_wake = Wake::from_c(&options.redraw_wake);
    let map_id = map.0;
    map_handle.runtime.post(move |state| {
        let result = (|| {
            let entry = state
                .maps
                .get(&map_id)
                .ok_or_else(|| Failure::state("the map was released"))?;
            if entry.session.is_some() {
                return Err(Failure::state("the map already has a render session"));
            }
            let scale = entry.extent.scale_factor;
            let target = match spec {
                TargetSpec::Offscreen => {
                    let (w, h) = physical(logical, scale)?;
                    Target {
                        target: orb_api::Target::texture(&state.runtime, w, h, scale),
                        _layer: None,
                    }
                }
                TargetSpec::Metal(layer) => metal_target(state, layer, logical, scale)?,
            };
            Ok((target, scale))
        })();
        match result {
            Ok((target, scale)) => {
                let map = state.maps.get_mut(&map_id).expect("map");
                map.session = Some(id);
                map.extent.width = logical.width;
                map.extent.height = logical.height;
                let entry = SessionEntry {
                    handle: handle.clone(),
                    map: map_id,
                    target: Some(target),
                    logical,
                    scale,
                    demands: VecDeque::new(),
                    redraw_wake,
                    redraw_signaled: false,
                    pacer: None,
                    frame_generation: 0,
                    still: None,
                };
                handle.state.lock().unwrap().state = MLN_RENDER_SESSION_STATE_ATTACHED;
                entry.sync_state();
                state.sessions.insert(id, entry);
                if let Some(c) = completion {
                    c.complete(Ok(Value::None), 0);
                }
            }
            Err(failure) => {
                handles::remove(id);
                if let Some(c) = completion {
                    c.complete(Err(failure), 0);
                }
            }
        }
    })?;
    // SAFETY: as above.
    unsafe { put(out_session, mln_render_session(id), "out_session") }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_attach_offscreen(
    map: mln_map,
    descriptor: *const mln_offscreen_descriptor,
    options: *const mln_render_session_attach_options,
    device: mln_offscreen_device,
    out_session: *mut mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            if device.0 != 0 {
                handles::offscreen_device(device.0)?;
            }
            let d = sized(descriptor, "descriptor")?;
            attach(
                map,
                d.logical_size,
                TargetSpec::Offscreen,
                options,
                out_session,
                completion,
            )
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_attach_metal_surface(
    map: mln_map,
    descriptor: *const mln_metal_surface_descriptor,
    options: *const mln_render_session_attach_options,
    out_session: *mut mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            if !cfg!(target_vendor = "apple") {
                return Err(Failure::unsupported("Metal is only on Apple platforms"));
            }
            let d = sized(descriptor, "descriptor")?;
            if !d.context.device.is_null() {
                return Err(Failure::unsupported(
                    "Orbis renders on its runtime's own MTLDevice; leave context.device null",
                ));
            }
            if d.layer.is_null() {
                return Err(Failure::argument("descriptor.layer is null"));
            }
            attach(
                map,
                d.logical_size,
                TargetSpec::Metal(UserData(d.layer)),
                options,
                out_session,
                completion,
            )
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_set_metal_surface_target(
    session: mln_render_session,
    replacement: *const mln_metal_surface_replacement,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::session(session.0)?;
            handle.failed()?;
            let r = sized(replacement, "replacement")?;
            let (logical, layer) = (r.logical_size, UserData(r.layer));
            handle.command(Completion::from_c(completion)?, move |state, id| {
                let scale = state.sessions[&id].scale;
                // The old surface goes before the new one is made on its layer.
                state.sessions.get_mut(&id).expect("session").target = None;
                let target = metal_target(state, layer, logical, scale)?;
                let entry = state.sessions.get_mut(&id).expect("session");
                entry.target = Some(target);
                entry.logical = logical;
                entry.sync_state();
                let map = entry.map;
                if let Some(m) = state.maps.get_mut(&map) {
                    (m.extent.width, m.extent.height) = (logical.width, logical.height);
                }
                force_redraw(state, id);
                Ok(Value::None)
            })
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_release_target(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::session(session.0)?;
            handle.command(Completion::from_c(completion)?, |state, id| {
                let entry = state.sessions.get_mut(&id).expect("session");
                entry.target = None;
                entry.sync_state();
                Ok(Value::None)
            })
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_resize(
    session: mln_render_session,
    logical_size: mln_logical_size,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::session(session.0)?;
            handle.failed()?;
            physical(logical_size, 1.0)?;
            handle.command(Completion::from_c(completion)?, move |state, id| {
                let entry = state.sessions.get_mut(&id).expect("session");
                let (w, h) = physical(logical_size, entry.scale)?;
                if let Some(target) = &mut entry.target {
                    target.target.resize(w, h, entry.scale);
                }
                entry.logical = logical_size;
                entry.sync_state();
                let map = entry.map;
                if let Some(m) = state.maps.get_mut(&map) {
                    (m.extent.width, m.extent.height) = (logical_size.width, logical_size.height);
                }
                force_redraw(state, id);
                Ok(Value::None)
            })
        })
    }
}

/// Marks the session's next demand as having something to draw.
pub fn force_redraw(state: &mut State, session: u64) {
    if let Some(entry) = state.sessions.get_mut(&session)
        && let Some(map) = state.maps.get_mut(&entry.map)
    {
        // A zero-length move dirties the map without moving its camera.
        map.map.jump_to(&orb_api::CameraTarget::default());
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_demand_frame(
    session: mln_render_session,
    demand: *const mln_frame_demand,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::session(session.0)?;
            handle.failed()?;
            let demand = *sized(demand, "demand")?;
            if demand.flags & MLN_FRAME_DEMAND_FORCE != 0
                && demand.flags & MLN_FRAME_DEMAND_WAIT_FOR_UPDATE != 0
            {
                return Err(Failure::argument(
                    "MLN_FRAME_DEMAND_FORCE and MLN_FRAME_DEMAND_WAIT_FOR_UPDATE don't combine",
                ));
            }
            if demand.token == MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE {
                return Err(Failure::argument("the still image token is the library's"));
            }
            let id = handle.id();
            handle.runtime.post(move |state| {
                if let Some(entry) = state.sessions.get_mut(&id) {
                    queue_demand(entry, demand);
                }
            })
        })
    }
}

fn queue_demand(entry: &mut SessionEntry, demand: mln_frame_demand) {
    entry.redraw_signaled = false;
    if demand.flags & MLN_FRAME_DEMAND_FORCE == 0
        && let Some(i) = entry.demands.iter().position(|d| d.flags == demand.flags)
    {
        let old = std::mem::replace(&mut entry.demands[i], demand);
        entry.handle.push(result(
            old.token,
            MLN_RENDER_RESULT_SUPERSEDED,
            entry.frame_generation,
            None,
        ));
        return;
    }
    entry.demands.push_back(demand);
}

fn result(
    token: u64,
    disposition: mln_render_result,
    frame_generation: u64,
    view: Option<(mln_map_view, u64, bool)>,
) -> mln_render_frame_result {
    let mut r = mln_render_frame_result {
        size: std::mem::size_of::<mln_render_frame_result>() as u32,
        disposition,
        token,
        frame_generation,
        mode: MLN_RENDER_MODE_FULL,
        ..Default::default()
    };
    if let Some((view, generation, needs_repaint)) = view {
        r.view = view;
        r.generation = generation;
        r.map_update_generation = generation;
        r.needs_repaint = needs_repaint;
    }
    r
}

/// Draws the frames that the sessions' demands and pacers ask for, and
/// reports them; then takes the maps' events.
pub fn pump(state: &mut State) {
    let now = state.now();
    let mut rendered = Vec::new();
    let State { sessions, maps, .. } = state;
    for entry in sessions.values_mut() {
        let Some(map) = maps.get_mut(&entry.map) else {
            continue;
        };
        if entry.handle.tick.swap(false, Ordering::AcqRel)
            && entry.demands.is_empty()
            && map.map.needs_frame()
        {
            entry.demands.push_back(mln_frame_demand {
                size: 0,
                flags: 0,
                token: 0,
            });
        }
        while let Some(demand) = entry.demands.pop_front() {
            let generation = map.map.generation();
            if entry.handle.failure.lock().unwrap().is_some() {
                entry.handle.push(result(
                    demand.token,
                    MLN_RENDER_RESULT_FAILED,
                    entry.frame_generation,
                    None,
                ));
                continue;
            }
            let Some(target) = &mut entry.target else {
                entry.handle.push(result(
                    demand.token,
                    MLN_RENDER_RESULT_TARGET_NOT_READY,
                    entry.frame_generation,
                    None,
                ));
                continue;
            };
            let continuous = map.mode == MLN_MAP_MODE_CONTINUOUS;
            let force = demand.flags & MLN_FRAME_DEMAND_FORCE != 0;
            if !continuous || !(force || map.map.needs_frame()) {
                if continuous && demand.flags & MLN_FRAME_DEMAND_WAIT_FOR_UPDATE != 0 {
                    entry.demands.push_front(demand);
                    break;
                }
                entry.handle.push(result(
                    demand.token,
                    MLN_RENDER_RESULT_NO_UPDATE,
                    entry.frame_generation,
                    None,
                ));
                continue;
            }
            let disposition = match map.map.render(&mut target.target, now) {
                Ok(Some(_report)) => {
                    entry.frame_generation += 1;
                    MLN_RENDER_RESULT_RENDERED
                }
                Ok(None) => MLN_RENDER_RESULT_TARGET_NOT_READY,
                Err(e) => {
                    let failure = Failure::from(e);
                    let status = if failure.status == MLN_STATUS_TARGET_LOST {
                        MLN_STATUS_TARGET_LOST
                    } else {
                        MLN_STATUS_NATIVE_ERROR
                    };
                    {
                        let mut s = entry.handle.state.lock().unwrap();
                        s.state = MLN_RENDER_SESSION_STATE_FAILED;
                        s.failure_status = status;
                    }
                    *entry.handle.failure.lock().unwrap() =
                        Some(Failure::new(status, failure.message));
                    MLN_RENDER_RESULT_FAILED
                }
            };
            let needs_repaint = map.map.needs_frame();
            let view = (disposition == MLN_RENDER_RESULT_RENDERED)
                .then(|| (map.view(), generation, needs_repaint));
            entry.handle.push(result(
                demand.token,
                disposition,
                entry.frame_generation,
                view,
            ));
            if disposition == MLN_RENDER_RESULT_RENDERED {
                rendered.push((entry.map, map.event_mask, needs_repaint));
            }
        }
        entry.sync_state();
        // The host draws at its next refresh when told there's something new.
        if entry.target.is_some()
            && entry.demands.is_empty()
            && !entry.redraw_signaled
            && map.map.needs_frame()
            && let Some(wake) = &entry.redraw_wake
        {
            entry.redraw_signaled = true;
            wake.signal();
        }
    }
    for (map, mask, needs_repaint) in rendered {
        state.event(
            map,
            mask,
            MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED,
            Payload::RenderFrame { needs_repaint },
            String::new(),
        );
    }
    let ids: Vec<u64> = state.maps.keys().copied().collect();
    for id in ids {
        crate::map::take_events(state, id);
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_drain_frame_results(
    session: mln_render_session,
    out_batch: *mut mln_render_frame_batch,
    out_view: *mut mln_render_frame_batch_view,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::session(session.0)?;
            if out_batch.is_null() || out_view.is_null() {
                return Err(Failure::argument("out_batch or out_view is null"));
            }
            let results = {
                let mut results = handle.results.lock().unwrap();
                handle.notified.store(false, Ordering::Release);
                std::mem::take(&mut *results)
            };
            let mut view = mln_render_frame_batch_view {
                size: std::mem::size_of::<mln_render_frame_batch_view>() as u32,
                result_size: std::mem::size_of::<mln_render_frame_result>() as u32,
                results: std::ptr::null(),
                result_count: 0,
            };
            if results.is_empty() {
                out_batch.write(mln_render_frame_batch(0));
            } else {
                let results = Arc::new(results);
                view.results = results.as_ptr();
                view.result_count = results.len();
                out_batch.write(mln_render_frame_batch(handles::insert(Object::Batch(
                    results,
                ))));
            }
            out_view.write(view);
            Ok(())
        })
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_render_frame_batch_release(batch: mln_render_frame_batch) {
    handles::remove(batch.0);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_get_snapshot(
    session: mln_render_session,
    out_snapshot: *mut mln_render_session_snapshot,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::session(session.0)?;
            put(out_snapshot, *handle.state.lock().unwrap(), "out_snapshot")
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_get_capabilities(
    session: mln_render_session,
    out_capabilities: *mut mln_render_session_capabilities,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::session(session.0)?;
            let flags = if handle.readback {
                MLN_RENDER_SESSION_CAPABILITY_READBACK
            } else {
                MLN_RENDER_SESSION_CAPABILITY_PRESENTATION
            };
            put(
                out_capabilities,
                mln_render_session_capabilities {
                    size: std::mem::size_of::<mln_render_session_capabilities>() as u32,
                    driver: handle.driver,
                    texture_ring_depth: 1,
                    flags,
                    producer_sync_kind: MLN_GPU_SYNC_CPU_COMPLETE,
                    consumer_sync_kinds: MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE,
                },
                "out_capabilities",
            )
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_get_failure(
    session: mln_render_session,
    out_message: *mut std::ffi::c_char,
    message_capacity: usize,
    out_message_length: *mut usize,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::session(session.0)?;
            let message = handle
                .failure
                .lock()
                .unwrap()
                .as_ref()
                .map(|f| f.message.clone())
                .unwrap_or_default();
            put(out_message_length, message.len(), "out_message_length")?;
            if !out_message.is_null() && message_capacity > message.len() {
                std::ptr::copy_nonoverlapping(message.as_ptr(), out_message.cast(), message.len());
                *out_message.add(message.len()) = 0;
            }
            Ok(())
        })
    }
}

/// The pixels a readback returns: the last still image, or the texture.
fn readback(state: &mut State, id: u64) -> Result<Value> {
    let entry = state.sessions.get_mut(&id).expect("session");
    let (width, height, pixels) = match &entry.still {
        Some(image) => (image.width, image.height, image.pixels.clone()),
        None => {
            let target = &entry
                .target
                .as_ref()
                .ok_or_else(|| Failure::state("the session has no target"))?
                .target;
            let (w, h) = target.size();
            (w, h, target.read_pixels()?)
        }
    };
    let info = mln_texture_image_info {
        width,
        height,
        stride: width * 4,
        byte_length: pixels.len(),
    };
    Ok(Value::Readback { pixels, info })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_read_texture(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::session(session.0)?;
            handle.failed()?;
            if !handle.readback {
                return Err(Failure::unsupported(
                    "the session lacks MLN_RENDER_SESSION_CAPABILITY_READBACK",
                ));
            }
            handle.command(Completion::from_c(completion)?, readback)
        })
    }
}

/// The tile a TILE map's camera shows: its zoom fits one tile to the map.
fn tile_of(camera: &mln_camera, logical: mln_logical_size) -> Result<orb_api::TileRequest> {
    let z = (camera.zoom - (f64::from(logical.width) / 512.0).log2()).round();
    if !(0.0..=30.0).contains(&z) {
        return Err(Failure::argument("the camera's zoom names no tile"));
    }
    let n = 2f64.powi(z as i32);
    let x = ((camera.center.longitude + 180.0) / 360.0 * n).floor();
    let lat = camera.center.latitude.to_radians();
    let y = ((1.0 - (lat.tan() + 1.0 / lat.cos()).ln() / std::f64::consts::PI) / 2.0 * n).floor();
    Ok(orb_api::TileRequest {
        tile_matrix_set: orb_api::TileMatrixSet::by_id("WebMercatorQuad")
            .map_err(|e| Failure::native(e.to_string()))?,
        z: z as u8,
        col: x.clamp(0.0, n - 1.0) as u32,
        row: y.clamp(0.0, n - 1.0) as u32,
        size: logical.width,
        scale: 1.0,
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_request_still_image(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::map(map.0)?;
            let completion = Completion::from_c(completion)?;
            let id = map.0;
            handle.runtime.post(move |state| {
                let result = still_image(state, id);
                let generation = state.maps.get(&id).map_or(0, |m| m.map.generation());
                if let Some(c) = completion {
                    c.complete(result.map(|()| Value::None), generation);
                }
            })
        })
    }
}

/// Renders a STATIC or TILE map's image into its session (Orbis's Still and
/// Tile modes), and reports it as a frame with the still image token.
fn still_image(state: &mut State, id: u64) -> Result {
    let State { sessions, maps, .. } = state;
    let map = maps
        .get_mut(&id)
        .ok_or_else(|| Failure::state("the map was released"))?;
    let session = map
        .session
        .ok_or_else(|| Failure::state("the map has no render session"))?;
    let entry = sessions.get_mut(&session).expect("session");
    let image = match map.mode {
        MLN_MAP_MODE_STATIC => {
            map.map
                .snapshot(entry.logical.width, entry.logical.height, entry.scale)?
        }
        MLN_MAP_MODE_TILE => {
            let mut tile = tile_of(&map.camera(), entry.logical)?;
            tile.scale = entry.scale;
            map.map.render_tile(&tile)?
        }
        _ => {
            return Err(Failure::state(
                "still images are for MLN_MAP_MODE_STATIC and MLN_MAP_MODE_TILE maps",
            ));
        }
    };
    entry.still = Some(image);
    entry.frame_generation += 1;
    let generation = map.map.generation();
    entry.handle.push(result(
        MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE,
        MLN_RENDER_RESULT_RENDERED,
        entry.frame_generation,
        Some((map.view(), generation, false)),
    ));
    entry.sync_state();
    Ok(())
}

fn end(
    session: mln_render_session,
    completion: Option<Completion>,
    wait: bool,
    state_after: mln_render_session_state,
) -> Result {
    let handle = handles::session(session.0)?;
    handles::remove(session.0);
    let id = handle.id();
    let job = move |state: &mut State| {
        if let Some(entry) = state.sessions.remove(&id) {
            if let Some(map) = state.maps.get_mut(&entry.map) {
                map.session = None;
            }
            entry.handle.state.lock().unwrap().state = state_after;
            if let Some(wake) = entry.handle.frame_wake {
                wake.release();
            }
        }
        Ok(())
    };
    if wait {
        handle.runtime.call(job)
    } else {
        handle.runtime.post(move |state| {
            let _ = job(state);
            if let Some(c) = completion {
                c.complete(Ok(Value::None), 0);
            }
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_release(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            end(
                session,
                Completion::from_c(completion)?,
                false,
                MLN_RENDER_SESSION_STATE_DETACHED,
            )
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_detach(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            end(
                session,
                Completion::from_c(completion)?,
                false,
                MLN_RENDER_SESSION_STATE_DETACHED,
            )
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_dispose(
    session: mln_render_session,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointer.
    unsafe {
        boundary(out_diagnostic, || {
            end(session, None, true, MLN_RENDER_SESSION_STATE_DETACHED)
        })
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_offscreen_device_create(
    _options: *const mln_offscreen_device_options,
    out_device: *mut mln_offscreen_device,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            put(
                out_device,
                mln_offscreen_device(handles::insert(Object::OffscreenDevice)),
                "out_device",
            )
        })
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_offscreen_device_release(device: mln_offscreen_device) {
    handles::remove(device.0);
}

// MARK: - Frame pacers

/// Demands frames of its sessions at the display's refreshes.
pub struct FramePacer {
    sessions: Mutex<Vec<(Arc<SessionHandle>, std::sync::mpsc::Sender<Msg>)>>,
    #[cfg(target_os = "macos")]
    _link: Mutex<display_link::Link>,
}

impl FramePacer {
    fn add(&self, session: &Arc<SessionHandle>) {
        let tx = session.runtime.sender();
        self.sessions.lock().unwrap().push((session.clone(), tx));
    }

    fn remove(&self, session: &Arc<SessionHandle>) {
        self.sessions
            .lock()
            .unwrap()
            .retain(|(s, _)| !Arc::ptr_eq(s, session));
    }

    /// A display refresh, on the display link's thread.
    fn refresh(&self) {
        for (session, tx) in self.sessions.lock().unwrap().iter() {
            if !session.tick.swap(true, Ordering::AcqRel) {
                let _ = tx.send(Msg::Pump);
            }
        }
    }
}

#[cfg(target_os = "macos")]
mod display_link {
    //! CoreVideo's display link: a callback at each refresh of the active
    //! displays, on a thread of its own.
    use std::ffi::c_void;

    type CVDisplayLinkRef = *mut c_void;
    type Callback = unsafe extern "C" fn(
        CVDisplayLinkRef,
        *const c_void,
        *const c_void,
        u64,
        *mut u64,
        *mut c_void,
    ) -> i32;

    #[link(name = "CoreVideo", kind = "framework")]
    unsafe extern "C" {
        fn CVDisplayLinkCreateWithActiveCGDisplays(out: *mut CVDisplayLinkRef) -> i32;
        fn CVDisplayLinkSetOutputCallback(
            link: CVDisplayLinkRef,
            callback: Callback,
            context: *mut c_void,
        ) -> i32;
        fn CVDisplayLinkStart(link: CVDisplayLinkRef) -> i32;
        fn CVDisplayLinkStop(link: CVDisplayLinkRef) -> i32;
        fn CVDisplayLinkRelease(link: CVDisplayLinkRef);
    }

    pub struct Link(CVDisplayLinkRef);
    // SAFETY: a CVDisplayLink is thread-safe.
    unsafe impl Send for Link {}

    impl Link {
        /// A link that refreshes the pacer at `context`, which outlives it.
        pub fn start(context: *const c_void) -> Result<Self, String> {
            unsafe extern "C" fn output(
                _: CVDisplayLinkRef,
                _: *const c_void,
                _: *const c_void,
                _: u64,
                _: *mut u64,
                context: *mut c_void,
            ) -> i32 {
                // SAFETY: `context` is the pacer, which stops the link before
                // it goes.
                let pacer = unsafe { &*context.cast::<super::FramePacer>() };
                pacer.refresh();
                0
            }
            let mut link = std::ptr::null_mut();
            // SAFETY: CoreVideo's API, with an out pointer.
            unsafe {
                if CVDisplayLinkCreateWithActiveCGDisplays(&mut link) != 0 {
                    return Err("CVDisplayLinkCreateWithActiveCGDisplays failed".into());
                }
                CVDisplayLinkSetOutputCallback(link, output, context.cast_mut());
                CVDisplayLinkStart(link);
            }
            Ok(Self(link))
        }
    }

    impl Drop for Link {
        fn drop(&mut self) {
            // SAFETY: the link this made; stopping waits out a callback.
            unsafe {
                CVDisplayLinkStop(self.0);
                CVDisplayLinkRelease(self.0);
            }
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_apple_frame_pacer_create(
    options: *const mln_frame_pacer_options,
    out_pacer: *mut mln_frame_pacer,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            if !options.is_null() && sized(options, "options")?.fields != 0 {
                return Err(Failure::unsupported(
                    "the Orbis pacer demands frames at every refresh",
                ));
            }
            if out_pacer.is_null() {
                return Err(Failure::argument("out_pacer is null"));
            }
            #[cfg(target_os = "macos")]
            {
                let pacer = Arc::new_cyclic(|weak: &std::sync::Weak<FramePacer>| FramePacer {
                    sessions: Mutex::default(),
                    _link: Mutex::new(
                        display_link::Link::start(weak.as_ptr().cast()).expect("display link"),
                    ),
                });
                return put(
                    out_pacer,
                    mln_frame_pacer(handles::insert(Object::Pacer(pacer))),
                    "out_pacer",
                );
            }
            #[allow(unreachable_code)]
            Err(Failure::unsupported(
                "Orbis's Apple frame pacer is macOS only so far",
            ))
        })
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_frame_pacer_release(pacer: mln_frame_pacer) {
    // Sessions keep it until they go.
    handles::remove(pacer.0);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_set_frame_pacer(
    session: mln_render_session,
    pacer: mln_frame_pacer,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointer.
    unsafe {
        boundary(out_diagnostic, || {
            let handle = handles::session(session.0)?;
            handle.failed()?;
            let pacer = if pacer.0 == 0 {
                None
            } else {
                Some(handles::pacer(pacer.0)?)
            };
            let id = handle.id();
            handle.runtime.post(move |state| {
                let Some(entry) = state.sessions.get_mut(&id) else {
                    return;
                };
                if entry.redraw_wake.is_some() {
                    return;
                }
                if let Some(old) = entry.pacer.take() {
                    old.remove(&entry.handle);
                }
                if let Some(pacer) = &pacer {
                    pacer.add(&entry.handle);
                }
                entry.pacer = pacer;
            })
        })
    }
}
