from __future__ import annotations

from collections.abc import Iterator
from contextlib import contextmanager
from dataclasses import replace

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import api as render
from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
    RED_BACKGROUND_STYLE_JSON,
    RED_PIXEL,
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
    if mln.supported_render_backend_mask() & mln.RenderBackendFlag.METAL:
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


def _descriptor_snapshot(
    descriptor: render.MetalBorrowedTextureDescriptor,
) -> tuple[object, ...]:
    return (
        descriptor.extent.width,
        descriptor.extent.height,
        descriptor.extent.scale_factor,
        descriptor.texture,
    )


def test_invalid_metal_surface_attach_reports_native_status() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        with pytest.raises(
            (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
        ) as raised:
            map_handle.metal_surface_attach(
                render.MetalSurfaceDescriptor.default(),
                replace(
                    mln.RenderSessionAttachOptions.default(),
                    driver=mln.RenderDriverKind.CORE_WORKER,
                ),
            )

        assert raised.value.status in {
            mln.Status.INVALID_ARGUMENT,
            mln.Status.UNSUPPORTED,
        }


def test_metal_surface_attach_reports_public_render_session_shape() -> None:
    with (
        _metal_context() as context,
        _metal_surface(context) as surface,
        mln.runtime_create() as runtime,
        runtime.map_create(
            replace(
                mln.MapOptions.default(),
                initial_extent=mln.LogicalExtent(surface.width, surface.height, 1.0),
            )
        ).result(timeout=5) as map_handle,
    ):
        session, attach = map_handle.metal_surface_attach(
            surface.descriptor(),
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
        finally:
            close_session(session)


def test_metal_borrowed_texture_attach_reports_public_render_session_shape() -> None:
    with _metal_context() as context, _metal_borrowed_texture(context) as texture:
        descriptor = texture.descriptor()

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
            session, attach = map_handle.metal_borrowed_texture_attach(
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


def test_metal_borrowed_texture_session_close_preserves_caller_resources() -> None:
    with _metal_context() as context, _metal_borrowed_texture(context) as texture:
        descriptor = texture.descriptor()
        before_descriptor = _descriptor_snapshot(descriptor)
        before_texture = texture.texture

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
            session, attach = map_handle.metal_borrowed_texture_attach(
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
        assert texture.texture is before_texture
        assert texture.exists()

        replacement_descriptor = texture.descriptor()
        assert _descriptor_snapshot(replacement_descriptor) == before_descriptor


def test_metal_borrowed_texture_set_target_renders_into_the_replacement() -> None:
    """A caller-owned texture is sized by its owner, so a host that follows a
    resize allocates a texture at the new size and hands it over instead of
    resizing this session.
    """
    with _metal_context() as context, _metal_borrowed_texture(context) as texture:
        descriptor = texture.descriptor()

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
            session, attach = map_handle.metal_borrowed_texture_attach(
                descriptor,
                replace(
                    mln.RenderSessionAttachOptions.default(),
                    driver=mln.RenderDriverKind.CORE_WORKER,
                ),
            )
            finish_render_operation(session, attach)
            try:
                map_handle.set_style_json(RED_BACKGROUND_STYLE_JSON.encode())
                render_until(
                    runtime,
                    session,
                    lambda: _is_painted_red(texture),
                    "the attached texture was never rendered into",
                )

                with pytest.raises(mln.UnsupportedFeatureError) as raised:
                    session.resize(render.RenderTargetExtent(96, 48, 1.0))
                assert raised.value.status == mln.Status.UNSUPPORTED

                with _metal_borrowed_texture(
                    context, width=96, height=48
                ) as replacement:
                    assert not any(replacement.read_rgba())

                    finish_render_operation(
                        session,
                        session.metal_borrowed_texture_set_target(
                            replacement.descriptor()
                        ),
                    )
                    # A retarget replaces the graphics resource only, and a
                    # caller-owned texture has no session resize, so the map
                    # takes the replacement extent from a map resize.
                    map_handle.resize(mln.LogicalExtent(96, 48, 1.0)).result(timeout=5)
                    # The session kept its renderer and paints the
                    # texture it was handed, at the extent handed with
                    # it, once the map has caught up to that extent.
                    render_until(
                        runtime,
                        session,
                        lambda: _is_painted_red(replacement),
                        "the replacement texture was never rendered into",
                    )
                    assert map_extent(map_handle) == (96, 48, pytest.approx(1.0))
            finally:
                close_session(session)


def test_metal_surface_set_target_presents_through_a_new_surface() -> None:
    """A host surface can be destroyed and recreated while the map goes on living.
    This verifies that the handoff is accepted, that the map takes the extent
    handed with it, and that the session stays usable; it does not observe
    presentation through the replacement layer.
    """
    with (
        _metal_context() as context,
        _metal_surface(context) as surface,
        mln.runtime_create() as runtime,
        runtime.map_create(
            replace(
                mln.MapOptions.default(),
                initial_extent=mln.LogicalExtent(surface.width, surface.height, 1.0),
            )
        ).result(timeout=5) as map_handle,
    ):
        session, attach = map_handle.metal_surface_attach(
            surface.descriptor(),
            replace(
                mln.RenderSessionAttachOptions.default(),
                driver=mln.RenderDriverKind.CORE_WORKER,
            ),
        )
        finish_render_operation(session, attach)
        try:
            map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
            render_until_update(runtime, session)

            with _metal_surface(context, width=48, height=24) as replacement:
                finish_render_operation(
                    session, session.metal_surface_set_target(replacement.descriptor())
                )
                # A retarget replaces the graphics resource only, so the map
                # takes the new extent from the session resize that follows.
                finish_render_operation(
                    session, session.resize(replacement.descriptor().extent)
                )
                render_until(
                    runtime,
                    session,
                    lambda: map_extent(map_handle) == (48, 24, pytest.approx(1.0)),
                    "the map never took the replacement surface extent",
                )
                assert (
                    session.get_snapshot().state == render.RenderSessionState.ATTACHED
                )
                assert (
                    request_and_finish_frame(session).disposition
                    == render.RenderResult.RENDERED
                )
        finally:
            close_session(session)


def test_metal_set_target_reports_unsupported_for_another_target_kind() -> None:
    """A session renders through the target kind it attached with, so a
    descriptor for the other kind is rejected and leaves the session usable.
    """
    with (
        _metal_context() as context,
        _metal_borrowed_texture(context) as texture,
        _metal_surface(context) as surface,
        mln.runtime_create() as runtime,
        runtime.map_create(
            replace(
                mln.MapOptions.default(),
                initial_extent=mln.LogicalExtent(surface.width, surface.height, 1.0),
            )
        ).result(timeout=5) as map_handle,
    ):
        session, attach = map_handle.metal_borrowed_texture_attach(
            texture.descriptor(),
            replace(
                mln.RenderSessionAttachOptions.default(),
                driver=mln.RenderDriverKind.CORE_WORKER,
            ),
        )
        finish_render_operation(session, attach)
        try:
            with pytest.raises(mln.UnsupportedFeatureError) as raised:
                finish_render_operation(
                    session,
                    session.metal_surface_set_target(surface.descriptor()),
                )
            assert raised.value.status == mln.Status.UNSUPPORTED
            # The rejection left the session usable.
            request_and_finish_frame(session)
        finally:
            close_session(session)

        session, attach = map_handle.metal_surface_attach(
            surface.descriptor(),
            replace(
                mln.RenderSessionAttachOptions.default(),
                driver=mln.RenderDriverKind.CORE_WORKER,
            ),
        )
        finish_render_operation(session, attach)
        try:
            with pytest.raises(mln.UnsupportedFeatureError) as raised:
                finish_render_operation(
                    session,
                    session.metal_borrowed_texture_set_target(texture.descriptor()),
                )
            assert raised.value.status == mln.Status.UNSUPPORTED
            # The rejection left the session usable.
            request_and_finish_frame(session)
        finally:
            close_session(session)
