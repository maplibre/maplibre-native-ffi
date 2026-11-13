# Snippets for the docs site's Render to a window page. They are type-checked
# with the tests, which never run them.

from collections.abc import Callable

import maplibre_native_ffi as mln


# Stand-ins for the host's event loop, display timer, and graphics thread.
def run_on_event_loop(task: Callable[[], None]) -> None:
    pass


def run_at_next_refresh(task: Callable[[], None]) -> None:
    pass


def run_on_graphics_thread(task: Callable[[], None]) -> None:
    pass


def replace_session() -> None:
    pass


def destroy_surface() -> None:
    pass


# #region attach
session: mln.RenderSessionHandle
# #endregion attach


# #region frame-loop
# Draws the map if it changed since the last frame.
def demand_frame() -> None:
    session.demand_frame(mln.FrameDemand())


def drain_frame_results() -> None:
    results = session.drain_frame_results()
    if not results:
        return
    if any(
        result.disposition == mln.RenderResult.FAILED
        for result in results
    ):
        replace_session()  # Release the session and attach a new one.


# #endregion frame-loop


# #region frame-wake
# Both run on the binding's dispatcher thread.
def on_redraw_wake() -> None:
    run_at_next_refresh(demand_frame)


def on_frame_wake() -> None:
    run_on_event_loop(drain_frame_results)


# #endregion frame-wake


# #region resize
# Runs when the window changes size, or moves to a display with
# another scale factor.
def on_window_resized(
    width: int, height: int, scale_factor: float
) -> None:
    # The window's new size in logical pixels, and its scale factor.
    session.discarding.resize(
        mln.LogicalExtent(
            width=width, height=height, scale_factor=scale_factor
        )
    )


# #endregion resize


# #region service
# Your graphics context is current on this thread.
def service_session() -> None:
    session.service_driver_work()


# Runs on the binding's dispatcher thread.
def on_driver_work_wake() -> None:
    run_on_graphics_thread(service_session)


# #endregion service


def surface_lost() -> None:
    # #region lost-surface
    session.release_target().result()
    # #endregion lost-surface


def surface_restored(display: int, new_surface: int) -> None:
    # #region lost-surface
    session.set_target(
        mln.RenderTargetWaylandSurfaceVariant(
            mln.WaylandSurfaceTarget(
                logical_size=mln.LogicalSize(width=800, height=600),
                display=display,
                surface=new_surface,
            )
        )
    ).result()
    # #endregion lost-surface


def render_to_window(
    map_handle: mln.MapHandle,
    display: int,
    window_surface: int,
) -> None:
    global session
    # #region attach
    # With no device, the session creates one on the platform's own
    # backend.
    session = map_handle.attach(
        mln.RenderTargetWaylandSurfaceVariant(
            mln.WaylandSurfaceTarget(
                # The window's size in logical pixels. The map's
                # scale factor turns it into pixels.
                logical_size=mln.LogicalSize(width=800, height=600),
                # Your wl_display and wl_surface.
                display=display,
                surface=window_surface,
            )
        ),
        None,
        mln.RenderSessionAttachOptions(
            driver=mln.RenderDriverKind.CORE_WORKER,
            redraw_wake=mln.Wake(on_redraw_wake),
            frame_wake=mln.Wake(on_frame_wake),
        ),
    )
    session.ready.result()
    # #endregion attach

    on_window_resized(1024, 768, 2.0)

    # #region service
    graphics_thread_options = mln.RenderSessionAttachOptions(
        driver=mln.RenderDriverKind.CALLER_GRAPHICS_THREAD,
        redraw_wake=mln.Wake(on_redraw_wake),
        frame_wake=mln.Wake(on_frame_wake),
        driver_work_wake=mln.Wake(on_driver_work_wake),
    )
    # #endregion service
    del graphics_thread_options

    session.release().result()
    destroy_surface()  # For example, with wl_surface_destroy.
