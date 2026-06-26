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
    from render_backend_helpers.metal import (
        MetalBorrowedTexture,
        MetalContext,
        MetalSurface,
        MetalUnavailableError,
    )
except Exception as error:
    MetalBorrowedTexture = None  # type: ignore[assignment]
    MetalContext = None  # type: ignore[assignment]
    MetalSurface = None  # type: ignore[assignment]
    MetalUnavailableError = RuntimeError  # type: ignore[assignment]
    _METAL_FIXTURE_IMPORT_ERROR = error
else:
    _METAL_FIXTURE_IMPORT_ERROR = None


def _require_native_metal_support() -> None:
    if mln.supported_render_backends() & mln.RenderBackend.METAL:
        return
    skip_or_fail_fixture_setup(
        "native library was not built with Metal render backend support",
        "metal",
    )


def _require_metal_fixture_support() -> None:
    if MetalContext is None:
        detail = (
            f": {_METAL_FIXTURE_IMPORT_ERROR}" if _METAL_FIXTURE_IMPORT_ERROR else ""
        )
        skip_or_fail_fixture_setup(
            f"Metal Python render fixtures are unavailable{detail}",
            "metal",
        )


@contextmanager
def _metal_context() -> Iterator[MetalContext]:
    _require_native_metal_support()
    _require_metal_fixture_support()
    try:
        context = MetalContext.create()
    except MetalUnavailableError as error:
        skip_or_fail_fixture_setup(
            f"Metal fixture creation is unavailable: {error}",
            "metal",
        )

    try:
        yield context
    finally:
        context.close()


@contextmanager
def _metal_surface(context: MetalContext) -> Iterator[MetalSurface]:
    try:
        surface = context.surface(width=32, height=16)
    except MetalUnavailableError as error:
        skip_or_fail_fixture_setup(
            f"Metal surface fixture creation is unavailable: {error}",
            "metal",
        )

    try:
        yield surface
    finally:
        surface.close()


@contextmanager
def _metal_borrowed_texture(
    context: MetalContext,
) -> Iterator[MetalBorrowedTexture]:
    try:
        texture = context.borrowed_texture(width=64, height=64)
    except MetalUnavailableError as error:
        skip_or_fail_fixture_setup(
            f"Metal borrowed-texture fixture creation is unavailable: {error}",
            "metal",
        )

    try:
        yield texture
    finally:
        texture.close()


def _assert_public_session_shape(session: render.RenderSessionHandle) -> None:
    assert isinstance(session, render.RenderSessionHandle)
    assert session.closed is False
    assert session.detached is False
    assert callable(session.render_update)
    assert callable(session.close)


def _descriptor_snapshot(
    descriptor: render.MetalBorrowedTextureDescriptor,
) -> tuple[object, ...]:
    return (
        descriptor.extent.width,
        descriptor.extent.height,
        descriptor.extent.scale_factor,
        descriptor.texture.address,
    )


def test_invalid_metal_surface_attach_reports_native_status() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            with pytest.raises(
                (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
            ) as raised:
                map_handle.attach_metal_surface(render.MetalSurfaceDescriptor())

            assert raised.value.status in {
                mln.MaplibreStatus.INVALID_ARGUMENT,
                mln.MaplibreStatus.UNSUPPORTED,
            }


def test_metal_surface_attach_reports_public_render_session_shape() -> None:
    with _metal_context() as context:
        with _metal_surface(context) as surface:
            with mln.RuntimeHandle() as runtime:
                with runtime.create_map(
                    mln.MapOptions(width=surface.width, height=surface.height)
                ) as map_handle:
                    session = map_handle.attach_metal_surface(surface.descriptor())
                    try:
                        _assert_public_session_shape(session)

                        map_handle.set_style_json(EMPTY_STYLE_JSON)
                        render_until_update(runtime, session)
                    finally:
                        session.close()


def test_metal_borrowed_texture_attach_reports_public_render_session_shape() -> None:
    with _metal_context() as context:
        with _metal_borrowed_texture(context) as texture:
            descriptor = texture.descriptor()

            with mln.RuntimeHandle() as runtime:
                with runtime.create_map(
                    mln.MapOptions(
                        width=descriptor.extent.width,
                        height=descriptor.extent.height,
                    )
                ) as map_handle:
                    session = map_handle.attach_metal_borrowed_texture(descriptor)
                    try:
                        _assert_public_session_shape(session)

                        map_handle.set_style_json(EMPTY_STYLE_JSON)
                        render_until_update(runtime, session)

                        with pytest.raises(mln.UnsupportedFeatureError) as raised:
                            session.acquire_metal_owned_texture_frame()
                        assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED
                    finally:
                        session.close()


def test_metal_borrowed_texture_session_close_preserves_caller_resources() -> None:
    with _metal_context() as context:
        with _metal_borrowed_texture(context) as texture:
            descriptor = texture.descriptor()
            before_descriptor = _descriptor_snapshot(descriptor)
            before_texture = texture.texture

            with mln.RuntimeHandle() as runtime:
                with runtime.create_map(
                    mln.MapOptions(
                        width=descriptor.extent.width,
                        height=descriptor.extent.height,
                    )
                ) as map_handle:
                    session = map_handle.attach_metal_borrowed_texture(descriptor)
                    _assert_public_session_shape(session)
                    session.close()

            assert _descriptor_snapshot(descriptor) == before_descriptor
            assert texture.texture is before_texture
            assert texture.exists()

            replacement_descriptor = texture.descriptor()
            assert _descriptor_snapshot(replacement_descriptor) == before_descriptor
