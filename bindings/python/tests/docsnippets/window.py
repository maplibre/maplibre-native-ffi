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
# Set while the window has no surface, and when it closes.
paused = False


def request_frame() -> None:
    session.request_frame(
        mln.FrameDemand(
            flags=mln.FrameDemandFlag.PRESENT | mln.FrameDemandFlag.WAIT_FOR_UPDATE
        )
    )


def drain_frame_results() -> None:
    results = session.drain_frame_results()
    if not results:
        return
    if any(result.disposition == mln.RenderResult.FAILED for result in results):
        replace_session()  # Close the session and attach a new one.
    elif not paused:
        run_at_next_refresh(request_frame)


# #endregion frame-loop


# #region frame-wake
# Runs on the binding's dispatcher thread.
def on_frame_wake() -> None:
    run_on_event_loop(drain_frame_results)


# #endregion frame-wake


# #region resize
def on_window_resized(width: int, height: int) -> None:
    # The scale factor the session has now.
    session.discarding.resize(
        mln.LogicalExtent(width=width, height=height, scale_factor=2.0)
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
    global paused
    # #region lost-surface
    paused = True
    session.release_target().result()
    # #endregion lost-surface


def surface_restored(vulkan: mln.VulkanContextDescriptor, new_surface: int) -> None:
    global paused
    # #region lost-surface
    session.set_vulkan_surface_target(
        mln.VulkanSurfaceDescriptor(
            extent=mln.LogicalExtent(width=800, height=600, scale_factor=2.0),
            context=vulkan,
            surface=new_surface,
        )
    ).result()
    paused = False
    request_frame()
    # #endregion lost-surface


def render_to_window(
    map_handle: mln.MapHandle,
    vulkan: mln.VulkanContextDescriptor,
    window_surface: int,
) -> None:
    global session, paused
    # #region attach
    session, ready = map_handle.attach_vulkan_surface(
        mln.VulkanSurfaceDescriptor(
            # The window's size in logical pixels, and its scale factor.
            extent=mln.LogicalExtent(width=800, height=600, scale_factor=2.0),
            # Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
            context=vulkan,
            # Your VkSurfaceKHR.
            surface=window_surface,
        ),
        mln.RenderSessionAttachOptions(
            driver=mln.RenderDriverKind.CORE_WORKER,
            frame_wake=mln.Wake(on_frame_wake),
        ),
    )
    ready.result()
    # #endregion attach

    request_frame()
    on_window_resized(1024, 768)

    # #region service
    graphics_thread_options = mln.RenderSessionAttachOptions(
        driver=mln.RenderDriverKind.CALLER_GRAPHICS_THREAD,
        frame_wake=mln.Wake(on_frame_wake),
        driver_work_wake=mln.Wake(on_driver_work_wake),
    )
    # #endregion service
    del graphics_thread_options

    # #region close
    paused = True
    session.close().result()
    destroy_surface()  # For example, with vkDestroySurfaceKHR.
    # #endregion close
