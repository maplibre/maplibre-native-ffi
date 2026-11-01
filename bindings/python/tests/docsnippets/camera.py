# Snippets for the docs site's Camera page. They are type-checked with the
# tests, which never run them.

import maplibre_native_ffi as mln


def use_camera(camera: mln.Camera) -> None:
    pass


def fly_ended(completed: bool) -> None:
    pass


# #region gesture
def begin_gesture(map_handle: mln.MapHandle) -> None:
    map_handle.discarding.cancel_transitions()
    map_handle.discarding.apply_camera_delta(
        mln.CameraDelta(gesture_phase=mln.GesturePhase.BEGIN)
    )


def apply_drag(map_handle: mln.MapHandle, movement: mln.ScreenPoint) -> None:
    map_handle.discarding.apply_camera_delta(
        mln.CameraDelta(offset=movement, gesture_phase=mln.GesturePhase.UPDATE)
    )


def apply_pinch(
    map_handle: mln.MapHandle, scale: float, center: mln.ScreenPoint
) -> None:
    map_handle.discarding.apply_camera_delta(
        mln.CameraDelta(
            scale=scale, anchor=center, gesture_phase=mln.GesturePhase.UPDATE
        )
    )


def end_gesture(map_handle: mln.MapHandle) -> None:
    map_handle.discarding.apply_camera_delta(
        mln.CameraDelta(gesture_phase=mln.GesturePhase.END)
    )


# #endregion gesture


def camera(map_handle: mln.MapHandle) -> None:
    # #region read
    snapshot = map_handle.get_snapshot()
    use_camera(snapshot.view.camera)

    result = map_handle.get_camera().result()
    use_camera(result.camera)
    # #endregion read

    # #region jump
    map_handle.discarding.update_camera(
        mln.CameraUpdate(
            mode=mln.CameraUpdateMode.JUMP,
            camera=mln.CameraOptions(
                center=mln.LatLng(latitude=48.8566, longitude=2.3522), zoom=12
            ),
        )
    )
    # #endregion jump

    # #region animate
    # Runs on the binding's dispatcher thread.
    def on_fly_end(end: mln.CameraTransitionEnd) -> None:
        fly_ended(end.outcome == mln.CameraTransitionOutcome.COMPLETED)

    map_handle.discarding.update_camera(
        mln.CameraUpdate(
            mode=mln.CameraUpdateMode.FLY,
            camera=mln.CameraOptions(
                center=mln.LatLng(latitude=40.7128, longitude=-74.006), zoom=10
            ),
            animation=mln.AnimationOptions(
                duration_ms=3000,
                end_handler=mln.CameraTransitionHandler(on_fly_end),
            ),
        )
    )
    # #endregion animate

    # #region fit
    bounds = mln.LatLngBounds(
        southwest=mln.LatLng(latitude=45.8, longitude=5.9),
        northeast=mln.LatLng(latitude=47.8, longitude=10.5),
    )
    fitted = map_handle.camera_for_lat_lng_bounds(bounds).result()

    map_handle.discarding.update_camera(
        mln.CameraUpdate(
            mode=mln.CameraUpdateMode.EASE,
            camera=fitted,
            animation=mln.AnimationOptions(duration_ms=500),
        )
    )
    # #endregion fit

    # #region limit
    map_handle.discarding.set_bounds(
        mln.BoundOptions(bounds=bounds, min_zoom=5, max_zoom=16)
    )
    # #endregion limit

    begin_gesture(map_handle)
    apply_drag(map_handle, mln.ScreenPoint(x=12, y=-4))
    apply_pinch(map_handle, 1.1, mln.ScreenPoint(x=200, y=150))
    end_gesture(map_handle)

    # #region convert
    with mln.MapProjectionHandle.create(map_handle.get_snapshot().view) as projection:
        coordinate = projection.lat_lng_for_pixel(mln.ScreenPoint(x=100, y=200))
        point = projection.pixel_for_lat_lng(coordinate)
    # #endregion convert
    del point
