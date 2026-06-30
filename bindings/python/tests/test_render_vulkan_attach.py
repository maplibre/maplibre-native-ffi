from __future__ import annotations

from collections.abc import Iterator
from contextlib import contextmanager

import pytest

import maplibre_native as mln
from maplibre_native import render

from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
    render_until_update,
    skip_or_fail_fixture_setup,
)

try:
    from render_backend_helpers.vulkan import (
        VulkanBorrowedImage,
        VulkanContext,
        VulkanUnavailableError,
    )
except Exception as error:  # pragma: no cover - host fixture dependency
    skip_or_fail_fixture_setup(
        f"Vulkan Python render fixtures are unavailable: {error}",
        "vulkan",
        allow_module_level=True,
    )


def _require_native_vulkan_support() -> None:
    if mln.supported_render_backends() & mln.RenderBackend.VULKAN:
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
def _vulkan_borrowed_image(context: VulkanContext) -> Iterator[VulkanBorrowedImage]:
    try:
        image = context.borrowed_image(width=64, height=64)
    except VulkanUnavailableError as error:
        skip_or_fail_fixture_setup(
            f"Vulkan borrowed-image fixture creation is unavailable: {error}",
            "vulkan",
        )

    try:
        yield image
    finally:
        image.close()


def _assert_public_session_shape(session: render.RenderSessionHandle) -> None:
    assert isinstance(session, render.RenderSessionHandle)
    assert session.closed is False
    assert session.detached is False
    assert callable(session.render_update)
    assert callable(session.close)


def _descriptor_snapshot(
    descriptor: render.VulkanBorrowedTextureDescriptor,
) -> tuple[object, ...]:
    context = descriptor.context
    return (
        descriptor.extent.width,
        descriptor.extent.height,
        descriptor.extent.scale_factor,
        context.instance.address,
        context.physical_device.address,
        context.device.address,
        context.graphics_queue.address,
        context.graphics_queue_family_index,
        context.get_instance_proc_addr.address,
        context.get_device_proc_addr.address,
        descriptor.image.address,
        descriptor.image_view.address,
        descriptor.format,
        descriptor.initial_layout,
        descriptor.final_layout,
    )


def test_invalid_vulkan_surface_attach_reports_native_status() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            with pytest.raises(
                (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
            ) as raised:
                map_handle.attach_vulkan_surface(render.VulkanSurfaceDescriptor())

            assert raised.value.status in {
                mln.MaplibreStatus.INVALID_ARGUMENT,
                mln.MaplibreStatus.UNSUPPORTED,
            }


def test_vulkan_borrowed_texture_attach_reports_public_render_session_shape() -> None:
    _require_native_vulkan_support()

    with _vulkan_context() as context:
        with _vulkan_borrowed_image(context) as image:
            descriptor = image.descriptor()

            with mln.RuntimeHandle() as runtime:
                with runtime.create_map(
                    mln.MapOptions(
                        width=descriptor.extent.width,
                        height=descriptor.extent.height,
                    )
                ) as map_handle:
                    session = map_handle.attach_vulkan_borrowed_texture(descriptor)
                    try:
                        _assert_public_session_shape(session)

                        map_handle.set_style_json(EMPTY_STYLE_JSON)
                        render_until_update(runtime, session)

                        with pytest.raises(mln.UnsupportedFeatureError) as raised:
                            session.acquire_vulkan_owned_texture_frame()
                        assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED
                    finally:
                        session.close()


def test_vulkan_borrowed_texture_session_close_preserves_caller_resources() -> None:
    _require_native_vulkan_support()

    with _vulkan_context() as context:
        with _vulkan_borrowed_image(context) as image:
            descriptor = image.descriptor()
            before_descriptor = _descriptor_snapshot(descriptor)
            before_resources = (image.image, image.image_view, image.memory)

            with mln.RuntimeHandle() as runtime:
                with runtime.create_map(
                    mln.MapOptions(
                        width=descriptor.extent.width,
                        height=descriptor.extent.height,
                    )
                ) as map_handle:
                    session = map_handle.attach_vulkan_borrowed_texture(descriptor)
                    _assert_public_session_shape(session)
                    session.close()

            assert _descriptor_snapshot(descriptor) == before_descriptor
            assert (image.image, image.image_view, image.memory) == before_resources

            replacement_descriptor = image.descriptor()
            assert _descriptor_snapshot(replacement_descriptor) == before_descriptor
