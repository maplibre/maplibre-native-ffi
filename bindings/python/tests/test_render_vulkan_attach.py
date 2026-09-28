from __future__ import annotations

from collections.abc import Iterator
from contextlib import contextmanager
from dataclasses import replace

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import api as render
from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
    assert_attached_session_shape,
    close_session,
    finish_render_operation,
    map_extent,
    render_until,
    render_until_update,
    request_and_finish_frame,
    skip_or_fail_fixture_setup,
)

try:
    from render_backend_helpers.vulkan import (
        VulkanBorrowedImage,
        VulkanContext,
        VulkanUnavailableError,
    )
except (ImportError, OSError, RuntimeError) as error:  # pragma: no cover
    skip_or_fail_fixture_setup(
        f"Vulkan Python render fixtures are unavailable: {error}",
        "vulkan",
        allow_module_level=True,
    )


def _require_native_vulkan_support() -> None:
    if mln.supported_render_backend_mask() & mln.RenderBackendFlag.VULKAN:
        return
    skip_or_fail_fixture_setup(
        "native library was not built with Vulkan render backend support",
        "vulkan",
    )


@contextmanager
def _vulkan_context() -> Iterator[VulkanContext]:
    try:
        context = VulkanContext.create()
    except VulkanUnavailableError as error:
        reason = str(error)
        skip_or_fail_fixture_setup(
            f"Vulkan fixture creation is unavailable: {reason}",
            "vulkan",
        )

    try:
        yield context
    finally:
        context.close()


@contextmanager
def _vulkan_borrowed_image(
    context: VulkanContext,
    *,
    width: int = 64,
    height: int = 64,
) -> Iterator[VulkanBorrowedImage]:
    try:
        image = context.borrowed_image(width=width, height=height)
    except VulkanUnavailableError as error:
        skip_or_fail_fixture_setup(
            f"Vulkan borrowed-image fixture creation is unavailable: {error}",
            "vulkan",
        )

    try:
        yield image
    finally:
        image.close()


def _descriptor_snapshot(
    descriptor: render.VulkanBorrowedTextureDescriptor,
) -> tuple[object, ...]:
    context = descriptor.context
    return (
        descriptor.extent.width,
        descriptor.extent.height,
        descriptor.extent.scale_factor,
        context.instance,
        context.physical_device,
        context.device,
        context.graphics_queue,
        context.graphics_queue_family_index,
        context.get_instance_proc_addr,
        context.get_device_proc_addr,
        descriptor.image,
        descriptor.image_view,
        descriptor.format,
        descriptor.initial_layout,
        descriptor.final_layout,
    )


def test_invalid_vulkan_surface_attach_reports_native_status() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        with pytest.raises(
            (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
        ) as raised:
            map_handle.vulkan_surface_attach(
                render.VulkanSurfaceDescriptor.default(),
                replace(
                    mln.RenderSessionAttachOptions.default(),
                    driver=mln.RenderDriverKind.CORE_WORKER,
                ),
            )

        assert raised.value.status in {
            mln.Status.INVALID_ARGUMENT,
            mln.Status.UNSUPPORTED,
        }


def test_vulkan_borrowed_texture_attach_reports_public_render_session_shape() -> None:
    _require_native_vulkan_support()

    with _vulkan_context() as context, _vulkan_borrowed_image(context) as image:
        descriptor = image.descriptor()

        with (
            mln.runtime_create() as runtime,
            runtime.map_create(
                replace(
                    mln.MapOptions.default(),
                    initial_extent=mln.LogicalExtent(
                        descriptor.extent.width, descriptor.extent.height, 1.0
                    ),
                )
            ).result(timeout=5) as map_handle,
        ):
            session, attach = map_handle.vulkan_borrowed_texture_attach(
                descriptor,
                replace(
                    mln.RenderSessionAttachOptions.default(),
                    driver=mln.RenderDriverKind.CORE_WORKER,
                ),
            )
            try:
                finish_render_operation(session, attach)
                assert_attached_session_shape(session)

                map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
                render_until_update(runtime, session)

                with pytest.raises(mln.UnsupportedFeatureError) as raised:
                    session.acquire_frame()
                assert raised.value.status == mln.Status.UNSUPPORTED
            finally:
                close_session(session)


def test_vulkan_borrowed_texture_session_close_preserves_caller_resources() -> None:
    _require_native_vulkan_support()

    with _vulkan_context() as context, _vulkan_borrowed_image(context) as image:
        descriptor = image.descriptor()
        before_descriptor = _descriptor_snapshot(descriptor)
        before_resources = (image.image, image.image_view, image.memory)

        with (
            mln.runtime_create() as runtime,
            runtime.map_create(
                replace(
                    mln.MapOptions.default(),
                    initial_extent=mln.LogicalExtent(
                        descriptor.extent.width, descriptor.extent.height, 1.0
                    ),
                )
            ).result(timeout=5) as map_handle,
        ):
            session, attach = map_handle.vulkan_borrowed_texture_attach(
                descriptor,
                replace(
                    mln.RenderSessionAttachOptions.default(),
                    driver=mln.RenderDriverKind.CORE_WORKER,
                ),
            )
            finish_render_operation(session, attach)
            assert_attached_session_shape(session)
            close_session(session)

        assert _descriptor_snapshot(descriptor) == before_descriptor
        assert (image.image, image.image_view, image.memory) == before_resources

        replacement_descriptor = image.descriptor()
        assert _descriptor_snapshot(replacement_descriptor) == before_descriptor


def test_vulkan_borrowed_texture_set_target_hands_over_a_replacement() -> None:
    """A caller-owned image is sized by its owner, so a host that follows a resize
    allocates an image at the new size and hands it over instead of resizing
    this session. This verifies that the handoff is accepted, that the session
    renders at the replacement extent once a map resize follows the handoff, and
    that the session stays usable; the Vulkan helper has no readback, so it does
    not check the replacement's pixels.
    """
    _require_native_vulkan_support()

    with _vulkan_context() as context, _vulkan_borrowed_image(context) as image:
        descriptor = image.descriptor()

        with (
            mln.runtime_create() as runtime,
            runtime.map_create(
                replace(
                    mln.MapOptions.default(),
                    initial_extent=mln.LogicalExtent(
                        descriptor.extent.width, descriptor.extent.height, 1.0
                    ),
                )
            ).result(timeout=5) as map_handle,
        ):
            session, attach = map_handle.vulkan_borrowed_texture_attach(
                descriptor,
                replace(
                    mln.RenderSessionAttachOptions.default(),
                    driver=mln.RenderDriverKind.CORE_WORKER,
                ),
            )
            finish_render_operation(session, attach)
            try:
                map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
                render_until_update(runtime, session)

                with pytest.raises(mln.UnsupportedFeatureError) as raised:
                    session.resize(render.RenderTargetExtent(48, 24, 1.0))
                assert raised.value.status == mln.Status.UNSUPPORTED

                with _vulkan_borrowed_image(
                    context, width=48, height=24
                ) as replacement:
                    replacement_descriptor = replacement.descriptor()
                    finish_render_operation(
                        session,
                        session.vulkan_borrowed_texture_set_target(
                            replacement_descriptor
                        ),
                    )
                    # A retarget replaces the graphics resource only, and a
                    # caller-owned texture has no session resize, so the map
                    # takes the replacement extent from a map resize.
                    map_handle.resize(mln.LogicalExtent(48, 24, 1.0)).result(timeout=5)
                    # The session kept its renderer and renders at the
                    # extent it was handed, once the map has caught up.
                    render_until(
                        runtime,
                        session,
                        lambda: map_extent(map_handle) == (48, 24, pytest.approx(1.0)),
                        "the map never took the replacement image extent",
                    )
                    assert (
                        request_and_finish_frame(session).disposition
                        == render.RenderResult.RENDERED
                    )

                    # A surface descriptor names a target this session
                    # does not have. The retarget kind is validated before the
                    # descriptor's host handles are read.
                    with pytest.raises(mln.UnsupportedFeatureError) as raised:
                        session.vulkan_surface_set_target(
                            render.VulkanSurfaceDescriptor(
                                extent=replacement_descriptor.extent,
                                context=context.descriptor(),
                                surface=0x1,
                            )
                        )
                    assert raised.value.status == mln.Status.UNSUPPORTED
                    # The rejection left the session usable.
                    request_and_finish_frame(session)
            finally:
                close_session(session)
