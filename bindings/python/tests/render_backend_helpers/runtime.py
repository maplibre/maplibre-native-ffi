from __future__ import annotations

import time

import maplibre_native as mln
from maplibre_native import render

EMPTY_STYLE_JSON = '{"version":8,"sources":{},"layers":[]}'


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
