from __future__ import annotations

from collections.abc import Iterator
from contextlib import contextmanager

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import render
from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
    RED_BACKGROUND_STYLE_JSON,
    RED_PIXEL,
    render_until,
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
except (ImportError, OSError, RuntimeError) as error:
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
def _metal_surface(
    context: MetalContext,
    *,
    width: int = 32,
    height: int = 16,
) -> Iterator[MetalSurface]:
    try:
        surface = context.surface(width=width, height=height)
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
    *,
    width: int = 64,
    height: int = 64,
) -> Iterator[MetalBorrowedTexture]:
    try:
        texture = context.borrowed_texture(width=width, height=height)
    except MetalUnavailableError as error:
        skip_or_fail_fixture_setup(
            f"Metal borrowed-texture fixture creation is unavailable: {error}",
            "metal",
        )

    try:
        yield texture
    finally:
        texture.close()


def _is_painted_red(texture: MetalBorrowedTexture) -> bool:
    """Return whether the whole texture carries the style's background color."""
    return texture.read_rgba() == RED_PIXEL * (texture.width * texture.height)


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
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        with pytest.raises(
            (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
        ) as raised:
            map_handle.attach_metal_surface(render.MetalSurfaceDescriptor())

        assert raised.value.status in {
            mln.MaplibreStatus.INVALID_ARGUMENT,
            mln.MaplibreStatus.UNSUPPORTED,
        }


def test_metal_surface_attach_reports_public_render_session_shape() -> None:
    with (
        _metal_context() as context,
        _metal_surface(context) as surface,
        mln.RuntimeHandle() as runtime,
        runtime.create_map(
            mln.MapOptions(width=surface.width, height=surface.height)
        ) as map_handle,
    ):
        session = map_handle.attach_metal_surface(surface.descriptor())
        try:
            _assert_public_session_shape(session)

            map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
            render_until_update(runtime, session)
        finally:
            session.close()


def test_metal_borrowed_texture_attach_reports_public_render_session_shape() -> None:
    with _metal_context() as context, _metal_borrowed_texture(context) as texture:
        descriptor = texture.descriptor()

        with (
            mln.RuntimeHandle() as runtime,
            runtime.create_map(
                mln.MapOptions(
                    width=descriptor.extent.width,
                    height=descriptor.extent.height,
                )
            ) as map_handle,
        ):
            session = map_handle.attach_metal_borrowed_texture(descriptor)
            try:
                _assert_public_session_shape(session)

                map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
                render_until_update(runtime, session)

                with pytest.raises(mln.UnsupportedFeatureError) as raised:
                    session.acquire_metal_owned_texture_frame()
                assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED
            finally:
                session.close()


def test_metal_borrowed_texture_session_close_preserves_caller_resources() -> None:
    with _metal_context() as context, _metal_borrowed_texture(context) as texture:
        descriptor = texture.descriptor()
        before_descriptor = _descriptor_snapshot(descriptor)
        before_texture = texture.texture

        with (
            mln.RuntimeHandle() as runtime,
            runtime.create_map(
                mln.MapOptions(
                    width=descriptor.extent.width,
                    height=descriptor.extent.height,
                )
            ) as map_handle,
        ):
            session = map_handle.attach_metal_borrowed_texture(descriptor)
            _assert_public_session_shape(session)
            session.close()

        assert _descriptor_snapshot(descriptor) == before_descriptor
        assert texture.texture is before_texture
        assert texture.exists()

        replacement_descriptor = texture.descriptor()
        assert _descriptor_snapshot(replacement_descriptor) == before_descriptor


def test_metal_borrowed_texture_set_target_renders_into_the_replacement() -> None:
    """Spec coverage: BND-175.

    A caller-owned texture is sized by its owner, so a host that follows a
    resize allocates a texture at the new size and hands it over instead of
    resizing this session.
    """
    with _metal_context() as context, _metal_borrowed_texture(context) as texture:
        descriptor = texture.descriptor()

        with (
            mln.RuntimeHandle() as runtime,
            runtime.create_map(
                mln.MapOptions(
                    width=descriptor.extent.width,
                    height=descriptor.extent.height,
                )
            ) as map_handle,
        ):
            session = map_handle.attach_metal_borrowed_texture(descriptor)
            try:
                map_handle.set_style_json(RED_BACKGROUND_STYLE_JSON.encode())
                render_until(
                    runtime,
                    session,
                    lambda: _is_painted_red(texture),
                    "the attached texture was never rendered into",
                )

                with pytest.raises(mln.UnsupportedFeatureError) as raised:
                    session.resize(96, 48, 1.0)
                assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED

                with _metal_borrowed_texture(
                    context, width=96, height=48
                ) as replacement:
                    assert not any(replacement.read_rgba())

                    session.set_metal_borrowed_texture_target(replacement.descriptor())

                    # The session kept its renderer and paints the
                    # texture it was handed, at the extent handed with
                    # it, once the map has caught up to that extent.
                    render_until(
                        runtime,
                        session,
                        lambda: _is_painted_red(replacement),
                        "the replacement texture was never rendered into",
                    )
                    assert map_handle.get_size() == (
                        96,
                        48,
                        pytest.approx(1.0),
                    )
            finally:
                session.close()


def test_metal_surface_set_target_presents_through_a_new_surface() -> None:
    """Spec coverage: BND-175.

    A host surface can be destroyed and recreated while the map goes on living.
    This verifies that the handoff is accepted, that the map takes the extent
    handed with it, and that the session stays usable; it does not observe
    presentation through the replacement layer.
    """
    with (
        _metal_context() as context,
        _metal_surface(context) as surface,
        mln.RuntimeHandle() as runtime,
        runtime.create_map(
            mln.MapOptions(width=surface.width, height=surface.height)
        ) as map_handle,
    ):
        session = map_handle.attach_metal_surface(surface.descriptor())
        try:
            map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
            render_until_update(runtime, session)

            with _metal_surface(context, width=48, height=24) as replacement:
                session.set_metal_surface_target(replacement.descriptor())

                render_until(
                    runtime,
                    session,
                    lambda: map_handle.get_size() == (48, 24, pytest.approx(1.0)),
                    "the map never took the replacement surface extent",
                )
                assert session.detached is False
                assert session.render_update() == render.RenderResult.RENDERED
        finally:
            session.close()


def test_metal_set_target_reports_unsupported_for_another_target_kind() -> None:
    """Spec coverage: BND-176.

    A session renders through the target kind it attached with, so a
    descriptor for the other kind is rejected and leaves the session usable.
    """
    with (
        _metal_context() as context,
        _metal_borrowed_texture(context) as texture,
        _metal_surface(context) as surface,
        mln.RuntimeHandle() as runtime,
        runtime.create_map(
            mln.MapOptions(width=surface.width, height=surface.height)
        ) as map_handle,
    ):
        session = map_handle.attach_metal_borrowed_texture(texture.descriptor())
        try:
            with pytest.raises(mln.UnsupportedFeatureError) as raised:
                session.set_metal_surface_target(surface.descriptor())
            assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED
            # The rejection left the session usable.
            session.render_update()
        finally:
            session.close()

        session = map_handle.attach_metal_surface(surface.descriptor())
        try:
            with pytest.raises(mln.UnsupportedFeatureError) as raised:
                session.set_metal_borrowed_texture_target(texture.descriptor())
            assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED
            # The rejection left the session usable.
            session.render_update()
        finally:
            session.close()
