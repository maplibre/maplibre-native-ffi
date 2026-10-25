"""Python callbacks that native holds and invokes."""

import gc
import sys
import threading
import typing
import weakref
from collections.abc import Callable
from dataclasses import replace

import maplibre_native_ffi as mln
import pytest
from support import (
    EMPTY_STYLE,
    TIMEOUT,
    Harness,
    denied_response,
    leak_reports,
    ok_response,
    ready_map,
    result,
)


class _Retiring:
    """A callback whose collection shows that native released it."""

    def __init__(self) -> None:
        self.called = threading.Event()
        self.retired = threading.Event()

    def __call__(self, *_: object) -> bool:
        self.called.set()
        return True

    def __del__(self) -> None:
        self.retired.set()


def test_a_registration_is_rooted_until_native_releases_it(
    map_handle: mln.MapHandle,
) -> None:
    result(map_handle.set_style_json(EMPTY_STYLE))

    # A registration the submission rejects is released before the call
    # returns.
    rejected = _Retiring()
    rejected_retired = rejected.retired
    with pytest.raises(
        mln.MaplibreError,
        check=lambda error: error.status == mln.Status.INVALID_ARGUMENT,
    ):
        map_handle.add_custom_geometry_source(
            "rejected",
            mln.CustomGeometrySourceOptions(fetch_tile=rejected, min_zoom=-1),
        )
    del rejected
    assert rejected_retired.is_set()

    # An accepted one stays rooted while native holds it.
    fetch = _Retiring()
    fetch_retired = fetch.retired
    result(
        map_handle.add_custom_geometry_source(
            "custom",
            mln.CustomGeometrySourceOptions(fetch_tile=fetch, min_zoom=0, max_zoom=2),
        )
    )
    del fetch
    gc.collect()
    assert not fetch_retired.is_set()

    # A command that fails releases its registration once it completes.
    duplicate = _Retiring()
    duplicate_retired = duplicate.retired
    failed = map_handle.add_custom_geometry_source(
        "custom", mln.CustomGeometrySourceOptions(fetch_tile=duplicate)
    )
    with pytest.raises(mln.MaplibreError):
        result(failed)
    del duplicate, failed
    assert duplicate_retired.wait(TIMEOUT)

    # Removing the source releases the accepted one.
    result(map_handle.remove_style_source("custom"))
    assert fetch_retired.wait(TIMEOUT)


def test_a_camera_end_handler_is_rooted_until_it_runs(
    map_handle: mln.MapHandle,
) -> None:
    # The handler sits two records deep in the command's input. Native holds
    # it while the ease runs, and runs it once when its identity is cancelled.
    ended = _Retiring()
    ended_retired = ended.retired
    update = replace(
        mln.CameraUpdate.default(),
        mode=mln.CameraUpdateMode.EASE,
        camera=mln.CameraOptions(zoom=4.0),
        animation=mln.AnimationOptions(
            duration_ms=60000.0,
            transition_id=3,
            end_handler=mln.CameraTransitionHandler(ended),
        ),
    )
    result(map_handle.update_camera(update))
    del update
    gc.collect()
    assert not ended_retired.is_set()
    result(map_handle.cancel_camera_transition(3))
    assert ended.called.is_set()
    del ended
    assert ended_retired.wait(TIMEOUT)

    # A rejected command releases the handler before the call returns.
    rejected = _Retiring()
    rejected_retired = rejected.retired
    delta = replace(
        mln.CameraDelta.default(),
        scale=-1.0,
        animation=mln.AnimationOptions(
            end_handler=mln.CameraTransitionHandler(rejected)
        ),
    )
    with pytest.raises(
        mln.MaplibreError,
        check=lambda error: error.status == mln.Status.INVALID_ARGUMENT,
    ):
        map_handle.apply_camera_delta(delta)
    del delta, rejected
    assert rejected_retired.is_set()


def test_a_registered_callback_does_not_keep_its_receiver_alive() -> None:
    woke = threading.Event()
    receiver: dict[str, object] = {}

    def receive_events(
        _: tuple[mln.RuntimeEvent, ...], receiver: dict[str, object] = receiver
    ) -> None:
        if receiver.get("runtime") is not None:
            woke.set()

    callback = weakref.ref(receive_events)
    runtime = mln.runtime_create(mln.RuntimeOptions.default())
    runtime.discarding.set_event_handler(mln.RuntimeEventHandler(receive_events))
    receiver["runtime"] = runtime
    owner = weakref.ref(runtime)
    map_handle = ready_map(runtime)
    result(map_handle.set_style_json(EMPTY_STYLE))
    assert woke.wait(TIMEOUT)
    result(map_handle.close())

    # The runtime roots the callback, and the callback reaches it, so only
    # the cycle collector can reclaim them, which it can see through the root.
    with leak_reports() as reports:
        del map_handle, runtime, receive_events, receiver
        gc.collect()
    assert owner() is None
    assert callback() is None
    assert reports == ["RuntimeHandle was not explicitly closed"]


def test_an_exception_in_a_callback_is_reported_and_contained(
    harness: Harness, map_handle: mln.MapHandle, monkeypatch: pytest.MonkeyPatch
) -> None:
    reports: list[object] = []
    monkeypatch.setattr(sys, "unraisablehook", reports.append)

    def fail(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        raise RuntimeError("provider failed")

    harness.routes["custom://raises.json"] = fail
    map_handle.set_style_url("custom://raises.json")
    # Native fails the request when the callback raises.
    harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)

    (report,) = reports
    assert isinstance(report.exc_value, RuntimeError)
    assert str(report.exc_value) == "provider failed"


def test_a_provider_can_answer_a_request_later_from_another_thread(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    handles: list[mln.ResourceRequestHandle] = []
    provided = threading.Event()

    def defer(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        handles.append(handle)
        provided.set()

    harness.routes["custom://later.json"] = defer
    map_handle.set_style_url("custom://later.json")
    assert provided.wait(TIMEOUT)
    handle = handles.pop()
    assert handle.is_cancelled() is False

    answer = threading.Thread(target=handle.complete, args=(ok_response(),))
    answer.start()
    answer.join(TIMEOUT)
    assert not answer.is_alive()
    harness.wait_event(mln.RuntimeEventType.MAP_STYLE_LOADED)
    # The answer consumed the handle, so a second answer fails.
    assert handle.is_closed
    with pytest.raises(
        mln.MaplibreError, check=lambda error: error.status == mln.Status.INVALID_STATE
    ):
        handle.complete(ok_response())


def test_a_scoped_response_refuses_use_after_its_callback_and_off_its_thread(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    # The URL names a loopback port that nothing serves, so the request fails
    # after the header transform without leaving the machine.
    style = "http://127.0.0.1:1/style.json"
    scopes: list[mln.HttpHeaderTransformResponseScope] = []
    passed_through: list[mln.ResourceRequestHandle] = []
    refusals: list[str] = []

    def refused(use: Callable[[], object]) -> None:
        try:
            use()
        except mln.MaplibreError as error:
            assert error.status == mln.Status.INVALID_STATE
            refusals.append(error.diagnostic)

    def transform(
        kind: mln.ResourceKind,
        url: str,
        response: mln.HttpHeaderTransformResponseScope,
    ) -> None:
        if url != style:
            return
        worker = threading.Thread(
            target=refused, args=(lambda: response.set_header("X-Test", "other"),)
        )
        worker.start()
        worker.join(TIMEOUT)
        response.set_header("X-Test", "inside")
        scopes.append(response)

    def pass_through(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        passed_through.append(handle)
        handle.pass_through()

    harness.routes[style] = pass_through
    result(
        harness.runtime.set_http_header_transform(
            mln.HttpHeaderTransform(callback=transform)
        )
    )
    map_handle.set_style_url(style)
    harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)
    refused(lambda: scopes[0].set_header("X-Test", "late"))
    result(harness.runtime.clear_http_header_transform())

    assert refusals == [
        "response belongs to its callback thread",
        "response callback has returned",
    ]
    # Passing a request through consumed its handle.
    assert len(passed_through) == 1
    assert passed_through[0].is_closed


def test_a_header_transform_fails_deliberately_with_a_status_and_reports_a_bug(
    harness: Harness, map_handle: mln.MapHandle, monkeypatch: pytest.MonkeyPatch
) -> None:
    # The URLs name a loopback port that nothing serves, so each request fails
    # after the header transform without leaving the machine.
    refused = mln.MaplibreError(mln.Status.INVALID_ARGUMENT, "the transform refused")
    broken = mln.MaplibreError(mln.Status.OK, "the transform failed without a status")
    raised = {
        "http://127.0.0.1:1/refused.json": refused,
        "http://127.0.0.1:1/broken.json": broken,
    }
    reports: list[typing.Any] = []
    monkeypatch.setattr(sys, "unraisablehook", reports.append)

    def transform(
        kind: mln.ResourceKind,
        url: str,
        response: mln.HttpHeaderTransformResponseScope,
    ) -> None:
        if url in raised:
            raise raised[url]

    result(
        harness.runtime.set_http_header_transform(
            mln.HttpHeaderTransform(callback=transform)
        )
    )
    for url in raised:
        harness.routes[url] = lambda request, handle: handle.pass_through()
        map_handle.set_style_url(url)
        harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)
    result(harness.runtime.clear_http_header_transform())

    # A failure status is deliberate, so only the error without one is reported.
    assert [report.exc_value for report in reports] == [broken]


def test_a_transform_request_is_answered_from_another_thread_after_its_callback(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    style = "custom://transformed.json"
    received: list[mln.ResourceTransformRequestHandle] = []
    taken = threading.Event()

    def transform(
        kind: mln.ResourceKind, url: str, request: mln.ResourceTransformRequestHandle
    ) -> None:
        if url != style:
            request.close()
            return
        received.append(request)
        taken.set()

    # The provider passes the style through to the online source, which waits
    # for the transform's answer.
    harness.routes[style] = lambda request, handle: handle.pass_through()
    result(
        harness.runtime.set_resource_transform(
            mln.ResourceTransform(callback=transform)
        )
    )
    map_handle.set_style_url(style)
    assert taken.wait(TIMEOUT)
    request = received[0]

    def answer() -> None:
        request.complete("unsupported://rewritten.json")

    worker = threading.Thread(target=answer)
    worker.start()
    worker.join(TIMEOUT)
    # The answer consumed the request, so a second answer fails.
    assert request.is_closed
    with pytest.raises(
        mln.MaplibreError, check=lambda error: error.status == mln.Status.INVALID_STATE
    ):
        request.complete("unsupported://again.json")
    # The online source cannot load the rewritten scheme.
    harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)
    result(harness.runtime.clear_resource_transform())


def test_replacing_the_log_callback_releases_the_previous_one(
    map_handle: mln.MapHandle,
) -> None:
    first, second = _Retiring(), _Retiring()
    first_retired, second_retired = first.retired, second.retired
    logged = second.called

    try:
        mln.log_set_callback(mln.LogHandler(first))
        mln.log_set_callback(mln.LogHandler(second))
        del first
        assert first_retired.wait(TIMEOUT)

        # The replacement receives what the map logs.
        map_handle.set_style_json(b"{")
        assert logged.wait(TIMEOUT)
    finally:
        mln.log_clear_callback()
    del second
    assert second_retired.wait(TIMEOUT)


def test_a_log_handler_that_returns_none_leaves_records_unconsumed(
    map_handle: mln.MapHandle, monkeypatch: pytest.MonkeyPatch
) -> None:
    reports: list[object] = []
    monkeypatch.setattr(sys, "unraisablehook", reports.append)

    class Unconsuming(_Retiring):
        def __call__(self, *_: object) -> None:
            self.called.set()

    handler = Unconsuming()
    logged, retired = handler.called, handler.retired
    try:
        mln.log_set_callback(mln.LogHandler(handler))
        del handler
        map_handle.set_style_json(b"{")
        assert logged.wait(TIMEOUT)
    finally:
        mln.log_clear_callback()
    # Once native releases the handler, no call of it is in flight.
    assert retired.wait(TIMEOUT)
    assert reports == []


def test_a_registration_reported_as_rejected_is_not_rooted(harness: Harness) -> None:
    handles: list[mln.ResourceRequestHandle] = []
    provided = threading.Event()

    def hold(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        handles.append(handle)
        provided.set()

    harness.routes["custom://discarded.json"] = hold
    map_handle = harness.create_map()
    map_handle.set_style_url("custom://discarded.json")
    assert provided.wait(TIMEOUT)
    handle = handles.pop()
    # Closing the map discards the request before a callback is registered.
    result(map_handle.close())
    assert handle.is_cancelled() is True

    callback = _Retiring()
    retired = callback.retired
    assert (
        handle.set_cancel_callback(mln.ResourceRequestCancelHandler(callback)) is True
    )
    del callback
    assert retired.is_set()
    handle.close()


def test_a_providers_routes_decide_which_requests_reach_it(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    seen: list[tuple[mln.ResourceKind, str | None]] = []
    sprites = {mln.ResourceKind.SPRITE_JSON, mln.ResourceKind.SPRITE_IMAGE}
    sprites_seen = threading.Event()
    routed_style = (
        b'{"version":8,"sprite":"custom://routed/sprite","sources":{},"layers":[]}'
    )

    def provide(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        seen.append((request.kind, request.requested_url))
        if request.kind == mln.ResourceKind.STYLE:
            handle.complete(ok_response(routed_style))
        else:
            handle.complete(denied_response())
        if sprites <= {kind for kind, _ in seen}:
            sprites_seen.set()

    routes = [
        # Another kind: claims no style or sprite, whatever its URL.
        mln.ResourceRoute(
            kinds=mln.ResourceKindMask.TILE,
            flags=mln.UrlMatchFlags.GLOB,
            url="**",
        ),
        # A URL pattern alone claims every kind under it.
        mln.ResourceRoute(
            flags=mln.UrlMatchFlags.GLOB,
            url="custom://routed/**",
        ),
    ]
    registered = harness.runtime.set_resource_provider(
        mln.ResourceProvider(callback=provide, routes=typing.cast(typing.Any, routes))
    )
    # Native holds its own copy, so a route that would claim everything,
    # added after the call, claims nothing.
    routes[0] = mln.ResourceRoute(
        kinds=mln.ResourceKindMask.ALL,
        flags=mln.UrlMatchFlags.GLOB,
        url="**",
    )
    routes.clear()
    result(registered)

    # A style outside the routes passes through to native, which serves no
    # such scheme.
    map_handle.set_style_url("custom://elsewhere/style.json")
    harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)
    assert seen == []

    map_handle.set_style_url("custom://routed/style.json")
    harness.wait_event(mln.RuntimeEventType.MAP_STYLE_LOADED)
    assert sprites_seen.wait(TIMEOUT)
    assert seen[0] == (mln.ResourceKind.STYLE, "custom://routed/style.json")
    assert all(
        url is not None and url.startswith("custom://routed/") for _, url in seen
    )
