# Snippets for the docs site's Create a map page. They are type-checked with
# the tests, which never run them.

import maplibre_native_ffi as mln


def copy_pixels(data: bytes, stride: int) -> None:
    pass


def create_a_map() -> None:
    # #region runtime
    runtime = mln.RuntimeHandle.create()
    # #endregion runtime

    # #region map
    extent = mln.LogicalExtent(width=512, height=512, scale_factor=1.0)
    map_handle, ready = runtime.create_map(
        mln.MapOptions(initial_extent=extent, map_mode=mln.MapMode.STATIC)
    )
    ready.result()
    # #endregion map

    # #region style
    map_handle.discarding.set_style_url("https://demotiles.maplibre.org/style.json")
    # #endregion style

    # #region attach
    device = mln.GraphicsDeviceHandle.create()

    session, ready = map_handle.attach_graphics_device_texture(
        device,
        mln.GraphicsDeviceTextureDescriptor(extent=extent),
        # The library renders on its own thread.
        mln.RenderSessionAttachOptions(driver=mln.RenderDriverKind.CORE_WORKER),
    )
    ready.result()
    # #endregion attach

    # #region render
    map_handle.request_still_image().result()
    # #endregion render

    # #region render
    image = session.read_texture().result()
    # #endregion render

    # #region render
    copy_pixels(image.data, image.info.stride)
    # #endregion render

    # #region close
    session.dispose()
    device.close()
    map_handle.close().result()
    runtime.close().result()
    # #endregion close
