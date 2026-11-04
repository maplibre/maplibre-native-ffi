# Snippets for the docs site's Create a map page. They are type-checked with
# the tests, which never run them.

import maplibre_native_ffi as mln


def report_failure(error: mln.MaplibreError) -> None:
    pass


# Attaches a render target, as the render target guides show.
def attach_target(
    map_handle: mln.MapHandle,
) -> mln.RenderSessionHandle:
    raise NotImplementedError


def create_a_map() -> None:
    # #region runtime
    runtime = mln.RuntimeHandle.create()
    # #endregion runtime

    # #region map
    map_handle = runtime.create_map(
        mln.MapOptions(
            initial_extent=mln.LogicalExtent(
                width=800, height=600, scale_factor=2.0
            ),
            map_mode=mln.MapMode.CONTINUOUS,
        )
    )
    map_handle.ready.result()
    # #endregion map

    # #region style
    map_handle.discarding.set_style_url(
        "https://demotiles.maplibre.org/style.json"
    )
    # #endregion style

    update = mln.CameraUpdate(
        mode=mln.CameraUpdateMode.JUMP,
        camera=mln.CameraOptions(zoom=3),
    )

    # #region wait
    map_handle.update_camera(update).result()

    # Use the discarding variant to ignore the result.
    map_handle.discarding.update_camera(update)
    # #endregion wait

    # #region errors
    try:
        map_handle.update_camera(update).result()
    except mln.MaplibreError as error:
        # Its message describes the failure.
        report_failure(error)
    # #endregion errors

    session = attach_target(map_handle)

    # #region close
    session.dispose()
    map_handle.dispose()
    runtime.close().result()
    # #endregion close
