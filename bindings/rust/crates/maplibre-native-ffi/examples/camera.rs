//! Snippets for the docs site's Camera page. They compile with the tests,
//! which never run them.

#![allow(dead_code)]

use maplibre_native_ffi::*;

fn use_camera(_camera: &Camera) {}

fn fly_ended(_completed: bool) {}

// #region gesture
fn begin_gesture(map: &MapHandle) -> Result<()> {
    map.discarding().cancel_transitions()?;
    map.discarding().apply_camera_delta(&CameraDelta {
        gesture_phase: GesturePhase::Begin,
        ..Default::default()
    })
}

fn apply_drag(map: &MapHandle, movement: ScreenPoint) -> Result<()> {
    map.discarding().apply_camera_delta(&CameraDelta {
        offset: Some(movement),
        gesture_phase: GesturePhase::Update,
        ..Default::default()
    })
}

fn apply_pinch(map: &MapHandle, scale: f64, center: ScreenPoint) -> Result<()> {
    map.discarding().apply_camera_delta(&CameraDelta {
        scale: Some(scale),
        anchor: Some(center),
        gesture_phase: GesturePhase::Update,
        ..Default::default()
    })
}

fn end_gesture(map: &MapHandle) -> Result<()> {
    map.discarding().apply_camera_delta(&CameraDelta {
        gesture_phase: GesturePhase::End,
        ..Default::default()
    })
}
// #endregion gesture

async fn camera(map: &MapHandle) -> Result<()> {
    // #region read
    let snapshot = map.get_snapshot()?;
    use_camera(&snapshot.view.camera);

    let result = map.get_camera()?.await?;
    use_camera(&result.camera);
    // #endregion read

    // #region jump
    map.discarding().update_camera(&CameraUpdate {
        mode: CameraUpdateMode::Jump,
        camera: CameraOptions {
            center: Some(LatLng::new(48.8566, 2.3522)),
            zoom: Some(12.0),
            ..Default::default()
        },
        ..Default::default()
    })?;
    // #endregion jump

    // #region animate
    map.discarding().update_camera(&CameraUpdate {
        mode: CameraUpdateMode::Fly,
        camera: CameraOptions {
            center: Some(LatLng::new(40.7128, -74.006)),
            zoom: Some(10.0),
            ..Default::default()
        },
        animation: AnimationOptions {
            duration_ms: Some(3000.0),
            // Runs on a library thread.
            end_handler: CameraTransitionHandler::new(|end| {
                fly_ended(end.outcome == CameraTransitionOutcome::Completed);
            }),
            ..Default::default()
        },
        ..Default::default()
    })?;
    // #endregion animate

    // #region fit
    let bounds = LatLngBounds {
        southwest: LatLng::new(45.8, 5.9),
        northeast: LatLng::new(47.8, 10.5),
    };
    // None uses the default fit options.
    let fitted = map.camera_for_lat_lng_bounds(bounds, None)?.await?;

    map.discarding().update_camera(&CameraUpdate {
        mode: CameraUpdateMode::Ease,
        camera: fitted,
        animation: AnimationOptions {
            duration_ms: Some(500.0),
            ..Default::default()
        },
        ..Default::default()
    })?;
    // #endregion fit

    // #region limit
    map.discarding().set_bounds(&BoundOptions {
        bounds: Some(bounds),
        min_zoom: Some(5.0),
        max_zoom: Some(16.0),
        ..Default::default()
    })?;
    // #endregion limit

    begin_gesture(map)?;
    apply_drag(map, ScreenPoint::new(12.0, -4.0))?;
    apply_pinch(map, 1.1, ScreenPoint::new(200.0, 150.0))?;
    end_gesture(map)?;

    // #region convert
    let projection = MapProjectionHandle::create(&map.get_snapshot()?.view)?;

    let coordinate = projection.lat_lng_for_pixel(ScreenPoint::new(100.0, 200.0))?;
    let point = projection.pixel_for_lat_lng(coordinate)?;

    drop(projection);
    // #endregion convert
    let _ = point;

    Ok(())
}

fn main() {}
