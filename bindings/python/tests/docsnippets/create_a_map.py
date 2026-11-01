# Snippets for the docs site's Create a map page. They are type-checked with
# the tests, which never run them.

import maplibre_native_ffi as mln


# Attaches a render target, as the Rendering guides show.
def attach_target(map_handle: mln.MapHandle) -> mln.RenderSessionHandle:
    raise NotImplementedError


def create_a_map() -> None:
    # #region runtime
    runtime = mln.RuntimeHandle.create()
    # #endregion runtime

    # #region map
    map_handle, ready = runtime.create_map(
        mln.MapOptions(
            initial_extent=mln.LogicalExtent(width=800, height=600, scale_factor=2.0),
            map_mode=mln.MapMode.CONTINUOUS,
        )
    )
    ready.result()
    # #endregion map

    session = attach_target(map_handle)

    # #region close
    session.dispose()
    map_handle.close().result()
    runtime.close().result()
    # #endregion close
