from __future__ import annotations

from collections.abc import Iterator
from contextlib import contextmanager

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import render
from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
    render_until,
    render_until_update,
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
        descriptor.image.bits,
        descriptor.image_view.bits,
        descriptor.format,
        descriptor.initial_layout,
        descriptor.final_layout,
    )


def test_invalid_vulkan_surface_attach_reports_native_status() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
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

    with _vulkan_context() as context, _vulkan_borrowed_image(context) as image:
        descriptor = image.descriptor()

        with (
            mln.RuntimeHandle() as runtime,
            runtime.create_map(
                mln.MapOptions(
                    width=descriptor.extent.width,
                    height=descriptor.extent.height,
                )
            ) as map_handle,
        ):
            session = map_handle.attach_vulkan_borrowed_texture(descriptor)
            try:
                _assert_public_session_shape(session)

                map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
                render_until_update(runtime, session)

                with pytest.raises(mln.UnsupportedFeatureError) as raised:
                    session.acquire_vulkan_owned_texture_frame()
                assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED
            finally:
                session.close()


def test_vulkan_borrowed_texture_session_close_preserves_caller_resources() -> None:
    _require_native_vulkan_support()

    with _vulkan_context() as context, _vulkan_borrowed_image(context) as image:
        descriptor = image.descriptor()
        before_descriptor = _descriptor_snapshot(descriptor)
        before_resources = (image.image, image.image_view, image.memory)

        with (
            mln.RuntimeHandle() as runtime,
            runtime.create_map(
                mln.MapOptions(
                    width=descriptor.extent.width,
                    height=descriptor.extent.height,
                )
            ) as map_handle,
        ):
            session = map_handle.attach_vulkan_borrowed_texture(descriptor)
            _assert_public_session_shape(session)
            session.close()

        assert _descriptor_snapshot(descriptor) == before_descriptor
        assert (image.image, image.image_view, image.memory) == before_resources

        replacement_descriptor = image.descriptor()
        assert _descriptor_snapshot(replacement_descriptor) == before_descriptor


def test_vulkan_borrowed_texture_set_target_hands_over_a_replacement() -> None:
    """Spec coverage: BND-175, BND-176.

    A caller-owned image is sized by its owner, so a host that follows a resize
    allocates an image at the new size and hands it over instead of resizing
    this session. This verifies that the handoff is accepted, that the map takes
    the extent handed with it, and that the session stays usable; the Vulkan
    helper has no readback, so it does not check the replacement's pixels.
    """
    _require_native_vulkan_support()

    with _vulkan_context() as context, _vulkan_borrowed_image(context) as image:
        descriptor = image.descriptor()

        with (
            mln.RuntimeHandle() as runtime,
            runtime.create_map(
                mln.MapOptions(
                    width=descriptor.extent.width,
                    height=descriptor.extent.height,
                )
            ) as map_handle,
        ):
            session = map_handle.attach_vulkan_borrowed_texture(descriptor)
            try:
                map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
                render_until_update(runtime, session)

                with pytest.raises(mln.UnsupportedFeatureError) as raised:
                    session.resize(48, 24, 1.0)
                assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED

                with _vulkan_borrowed_image(
                    context, width=48, height=24
                ) as replacement:
                    replacement_descriptor = replacement.descriptor()
                    session.set_vulkan_borrowed_texture_target(replacement_descriptor)

                    # The session kept its renderer and renders at the
                    # extent it was handed, once the map catches up.
                    render_until(
                        runtime,
                        session,
                        lambda: map_handle.get_size() == (48, 24, pytest.approx(1.0)),
                        "the map never took the replacement image extent",
                    )
                    assert (
                        session.render_update().result == render.RenderResult.RENDERED
                    )

                    # A surface descriptor names a target this session
                    # does not have.
                    with pytest.raises(mln.UnsupportedFeatureError) as raised:
                        session.set_vulkan_surface_target(
                            render.VulkanSurfaceDescriptor(
                                extent=replacement_descriptor.extent,
                                context=context.descriptor(),
                                surface=render.VulkanHandle(0x1),
                            )
                        )
                    assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED
                    # The rejection left the session usable.
                    session.render_update()
            finally:
                session.close()
