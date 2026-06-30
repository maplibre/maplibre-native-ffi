from __future__ import annotations

import os
import time

import pytest

import maplibre_native as mln
from maplibre_native import render

EMPTY_STYLE_JSON = '{"version":8,"sources":{},"layers":[]}'


def is_configured_render_backend(
    backend: str,
    *,
    context_provider: str | None = None,
) -> bool:
    if os.environ.get("MLN_FFI_RENDER_BACKEND", "").lower() != backend:
        return False
    if context_provider is None:
        return True
    return (
        os.environ.get("MLN_FFI_OPENGL_CONTEXT_PROVIDER", "").lower()
        == context_provider
    )


def skip_or_fail_fixture_setup(
    reason: str,
    backend: str,
    *,
    context_provider: str | None = None,
    allow_module_level: bool = False,
) -> None:
    if is_configured_render_backend(backend, context_provider=context_provider):
        pytest.fail(reason)
    pytest.skip(reason, allow_module_level=allow_module_level)


def wait_for_runtime_event(
    runtime: mln.RuntimeHandle,
    event_type: mln.RuntimeEventType,
    *,
    iterations: int = 5000,
) -> mln.RuntimeEvent:
    for _ in range(iterations):
        runtime.run_once()
        while event := runtime.poll_event():
            if event.event_type == event_type:
                return event
        time.sleep(0.001)
    raise AssertionError(f"runtime event {event_type!r} was not observed")


def render_until_update(
    runtime: mln.RuntimeHandle,
    session: render.RenderSessionHandle,
    *,
    iterations: int = 5000,
) -> None:
    event = wait_for_runtime_event(
        runtime,
        mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE,
        iterations=iterations,
    )
    assert event.event_type == mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE
    session.render_update()
