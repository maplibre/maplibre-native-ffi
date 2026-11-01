# Snippets for the docs site's Render images page. They are type-checked with
# the tests, which never run them.

import maplibre_native_ffi as mln


# The center of tile z/x/y. The snippet leaves the tile math to the reader.
def tile_center(z: int, x: int, y: int) -> mln.LatLng:
    return mln.LatLng(latitude=0.0, longitude=0.0)


def copy_pixels(data: bytes, stride: int) -> None:
    pass


def images(runtime: mln.RuntimeHandle, z: int, x: int, y: int) -> None:
    # #region map
    extent = mln.LogicalExtent(width=512, height=512, scale_factor=1.0)
    map_handle, map_ready = runtime.create_map(
        mln.MapOptions(initial_extent=extent, map_mode=mln.MapMode.TILE)
    )
    map_ready.result()
    # #endregion map

    # #region style
    map_handle.discarding.set_style_url("https://demotiles.maplibre.org/style.json")
    # #endregion style

    # #region attach
    device = mln.GraphicsDeviceHandle.create()

    session, session_ready = map_handle.attach_graphics_device_texture(
        device,
        mln.GraphicsDeviceTextureDescriptor(extent=extent),
        # The library renders on its own thread.
        mln.RenderSessionAttachOptions(driver=mln.RenderDriverKind.CORE_WORKER),
    )
    session_ready.result()
    # #endregion attach

    # #region image
    map_handle.discarding.update_camera(
        mln.CameraUpdate(
            mode=mln.CameraUpdateMode.JUMP,
            camera=mln.CameraOptions(center=tile_center(z, x, y), zoom=z),
        )
    )

    map_handle.request_still_image().result()
    # #endregion image

    # #region read
    image = session.read_texture().result()
    copy_pixels(image.data, image.info.stride)
    # #endregion read

    # #region close
    session.dispose()
    device.close()
    map_handle.close().result()
    # #endregion close
