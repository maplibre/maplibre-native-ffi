# Snippets for the docs site's Camera page. They are type-checked with the
# tests, which never run them.

import maplibre_native_ffi as mln


def use_camera(camera: mln.Camera) -> None:
    pass


def fly_ended(completed: bool) -> None:
    pass


# #region gesture
def create_recognizer(
    map_handle: mln.MapHandle,
) -> mln.GestureRecognizerHandle:
    return map_handle.gesture_recognizer_create()


def schedule_tick(result: mln.GestureResult) -> None:
    pass  # On the next frame, or at result.next_tick_ms.


def pointer_down(
    recognizer: mln.GestureRecognizerHandle,
    id: int,
    position: mln.ScreenPoint,
    time_ms: float,
) -> None:
    result = recognizer.handle(
        mln.GestureInputPointerDownVariant(
            mln.GesturePointerDown(
                id=id, kind=mln.PointerKind.TOUCH, position=position
            )
        ),
        time_ms,
    )
    schedule_tick(result)


def pointer_move(
    recognizer: mln.GestureRecognizerHandle,
    id: int,
    position: mln.ScreenPoint,
    time_ms: float,
) -> None:
    result = recognizer.handle(
        mln.GestureInputPointerMoveVariant(
            mln.GesturePointer(id=id, position=position)
        ),
        time_ms,
    )
    schedule_tick(result)


def tick(
    recognizer: mln.GestureRecognizerHandle, time_ms: float
) -> None:
    schedule_tick(recognizer.tick(time_ms))


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
                center=mln.LatLng(latitude=48.8566, longitude=2.3522),
                zoom=12,
            ),
        )
    )
    # #endregion jump

    # #region animate
    # Runs on the binding's dispatcher thread.
    def on_fly_end(end: mln.CameraTransitionEnd) -> None:
        fly_ended(
            end.outcome == mln.CameraTransitionOutcome.COMPLETED
        )

    map_handle.discarding.update_camera(
        mln.CameraUpdate(
            mode=mln.CameraUpdateMode.FLY,
            camera=mln.CameraOptions(
                center=mln.LatLng(
                    latitude=40.7128, longitude=-74.006
                ),
                zoom=10,
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
    map_handle.discarding.fit_camera_to_lat_lng_bounds(
        bounds,
        mln.CameraFitUpdate(
            mode=mln.CameraUpdateMode.EASE,
            animation=mln.AnimationOptions(duration_ms=500),
        ),
    )
    # #endregion fit

    # #region limit
    map_handle.discarding.set_bounds(
        mln.BoundOptions(bounds=bounds, min_zoom=5, max_zoom=16)
    )
    # #endregion limit

    with create_recognizer(map_handle) as recognizer:
        pointer_down(recognizer, 1, mln.ScreenPoint(x=100, y=100), 0)
        pointer_move(recognizer, 1, mln.ScreenPoint(x=112, y=96), 16)
        tick(recognizer, 32)

    # #region convert
    with map_handle.projection_create() as projection:
        coordinate = projection.lat_lng_for_pixel(
            mln.ScreenPoint(x=100, y=200)
        )
        point = projection.pixel_for_lat_lng(coordinate)
    # #endregion convert
    del point
