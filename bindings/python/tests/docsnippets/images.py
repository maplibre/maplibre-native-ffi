# Snippets for the docs site's Render images page. They are type-checked with
# the tests, which never run them.

import maplibre_native_ffi as mln


def copy_pixels(data: bytes, stride: int) -> None:
    pass


def images(runtime: mln.RuntimeHandle) -> None:
    # #region map
    map_handle, map_ready = runtime.create_map(
        mln.MapOptions(
            initial_extent=mln.LogicalExtent(
                width=800, height=600, scale_factor=2.0
            ),
            map_mode=mln.MapMode.STATIC,
        )
    )
    map_ready.result()
    # #endregion map

    # #region style
    map_handle.discarding.set_style_url(
        "https://demotiles.maplibre.org/style.json"
    )
    # #endregion style

    # #region attach
    session, session_ready = map_handle.attach_offscreen(
        mln.OffscreenDescriptor(
            logical_size=mln.LogicalSize(width=800, height=600)
        ),
        # The library renders on its own thread.
        mln.RenderSessionAttachOptions(
            driver=mln.RenderDriverKind.CORE_WORKER
        ),
    )
    session_ready.result()
    # #endregion attach

    # #region image
    map_handle.discarding.update_camera(
        mln.CameraUpdate(
            mode=mln.CameraUpdateMode.JUMP,
            camera=mln.CameraOptions(
                center=mln.LatLng(latitude=48.8566, longitude=2.3522),
                zoom=4,
            ),
        )
    )

    map_handle.request_still_image().result()
    # #endregion image

    # #region read
    image = session.read_texture().result()
    copy_pixels(image.data, image.info.stride)
    # #endregion read

    session.dispose()
    map_handle.close().result()


def images_tile(
    runtime: mln.RuntimeHandle, z: int, x: int, y: int
) -> None:
    # #region tile
    map_handle, ready = runtime.create_map(
        mln.MapOptions(
            initial_extent=mln.LogicalExtent(
                width=512, height=512, scale_factor=1.0
            ),
            map_mode=mln.MapMode.TILE,
        )
    )
    ready.result()

    map_handle.discarding.update_camera(
        mln.CameraUpdate(
            mode=mln.CameraUpdateMode.JUMP,
            camera=mln.camera_for_tile(
                mln.CanonicalTileId(z=z, x=x, y=y),
                mln.LogicalSize(width=512, height=512),
            ),
        )
    )
    # #endregion tile

    map_handle.close().result()
