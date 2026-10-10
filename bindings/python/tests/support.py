"""The shared runtime fixture and the one wait that every test uses."""

from __future__ import annotations

import contextlib
import os
import sys
import threading
import time
import warnings
from collections.abc import Callable, Iterator
from concurrent.futures import Future
from dataclasses import replace

import maplibre_native_ffi as mln

# Emulators, simulators, and software renderers set a larger scale.
TIMEOUT = 10.0 * float(os.environ.get("MLN_TEST_TIMEOUT_SCALE", "1"))

EMPTY_STYLE = b'{"version":8,"sources":{},"layers":[]}'
RED_STYLE = (
    b'{"version":8,"sources":{},"layers":[{"id":"background",'
    b'"type":"background","paint":{"background-color":"#ff0000"}}]}'
)
RED_PIXEL = b"\xff\x00\x00\xff"


def ok_response(data: bytes = EMPTY_STYLE) -> mln.ResourceResponse:
    return mln.ResourceResponse(
        status=mln.ResourceResponseStatus.OK,
        error_reason=mln.ResourceErrorReason.NONE,
        bytes=data,
        must_revalidate=False,
    )


def result[T](future: Future[T]) -> T:
    return future.result(timeout=TIMEOUT)


@contextlib.contextmanager
def leak_reports() -> Iterator[list[str]]:
    """Collect the ResourceWarning messages that unclosed handles report.

    The list fills when the block exits. A report can come from any thread,
    such as a native callback thread that drops the last reference, so this
    relies on the process-wide warning filters that an interpreter without
    context-aware warnings keeps. The abi3 extension does not load on a
    free-threaded build, where those are the default.
    """
    if getattr(sys.flags, "context_aware_warnings", False):
        raise RuntimeError("leak_reports needs process-wide warning filters")
    reports: list[str] = []
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter("always", ResourceWarning)
        yield reports
    reports.extend(
        str(warning.message)
        for warning in caught
        if issubclass(warning.category, ResourceWarning)
    )


class Signal:
    """A wake that native invokes, and the wait that blocks on it.

    ``wait_until`` checks its condition, then blocks until the next wake or
    the deadline. Clearing before each check keeps a wake that arrives during
    the check.
    """

    def __init__(self) -> None:
        self._event = threading.Event()

    def notify(self) -> None:
        self._event.set()

    def wait_until[T](self, check: Callable[[], T | None], what: str) -> T:
        deadline = time.monotonic() + TIMEOUT
        while True:
            self._event.clear()
            value = check()
            if value is not None:
                return value
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not self._event.wait(remaining):
                raise AssertionError(f"timed out waiting for {what}")


def denied_response() -> mln.ResourceResponse:
    return mln.ResourceResponse(
        status=mln.ResourceResponseStatus.ERROR,
        error_reason=mln.ResourceErrorReason.NOT_FOUND,
        bytes=b"",
        must_revalidate=False,
        error_message="the test provider serves no such resource",
    )


type Route = Callable[[mln.ResourceRequest, mln.ResourceRequestHandle], object]


class Harness:
    """One runtime whose events wake the test, and whose provider denies
    every request that no route serves, so no test reaches the network."""

    def __init__(self) -> None:
        self.events = Signal()
        self.routes: dict[str, Route] = {}
        self._pending: list[mln.RuntimeEvent] = []
        self._maps: list[mln.MapHandle] = []
        self.runtime = mln.runtime_create(
            replace(
                mln.RuntimeOptions.default(), event_wake=mln.Wake(self.events.notify)
            )
        )
        result(self.runtime.set_resource_provider(mln.ResourceProvider(self._provide)))

    def _provide(
        self, request: mln.ResourceRequest, handle: mln.ResourceRequestHandle
    ) -> mln.ResourceProviderDecision:
        route = self.routes.get(request.requested_url or "")
        if route is not None:
            decision = route(request, handle)
            if decision is not None:
                return decision
            return mln.ResourceProviderDecision.HANDLE
        handle.complete(denied_response())
        return mln.ResourceProviderDecision.HANDLE

    def create_map(self, **options: object) -> mln.MapHandle:
        map_handle = result(
            self.runtime.create_map(replace(mln.MapOptions.default(), **options))
        )
        self._maps.append(map_handle)
        return map_handle

    def wait_event(
        self,
        event_type: mln.RuntimeEventType,
        match: Callable[[mln.RuntimeEvent], bool] = lambda _: True,
    ) -> mln.RuntimeEvent:
        """Return the first event of this type that matches, draining as the
        runtime's event wake fires."""

        def check() -> mln.RuntimeEvent | None:
            with self.runtime.drain_events() as batch:
                self._pending.extend(batch.get().events)
            for index, event in enumerate(self._pending):
                if event.type == event_type and match(event):
                    del self._pending[index]
                    return event
            return None

        return self.events.wait_until(check, event_type.name)

    def close(self) -> None:
        for map_handle in self._maps:
            if not map_handle.closed:
                result(map_handle.close())
        if not self.runtime.closed:
            result(self.runtime.close())
