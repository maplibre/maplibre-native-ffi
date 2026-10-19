"""Python callbacks that native holds and invokes."""

import contextlib
import gc
import http.server
import sys
import threading
import typing
import weakref
from collections.abc import Callable, Iterator
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
    result,
)


class _Retiring:
    """A callback whose collection shows that native released it."""

    def __init__(self) -> None:
        self.called = threading.Event()
        self.retired = threading.Event()

    def __call__(self, *_: object) -> int:
        self.called.set()
        return 1

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
    with pytest.raises(mln.InvalidArgumentError):
        map_handle.add_custom_geometry_source(
            "rejected",
            mln.CustomGeometrySourceOptions(fetch_tile=rejected, min_zoom=-1),
        )
    del rejected
    assert rejected_retired.is_set()

    # An accepted one stays rooted while native holds it.
    fetch = _Retiring()
    fetch_retired = fetch.retired
    committed = result(
        map_handle.add_custom_geometry_source(
            "custom",
            mln.CustomGeometrySourceOptions(fetch_tile=fetch, min_zoom=0, max_zoom=2),
        )
    )
    assert committed.disposition == mln.CommandDisposition.COMMITTED
    del fetch
    gc.collect()
    assert not fetch_retired.is_set()

    # A command that fails releases its registration once it completes.
    duplicate = _Retiring()
    duplicate_retired = duplicate.retired
    failed = result(
        map_handle.add_custom_geometry_source(
            "custom", mln.CustomGeometrySourceOptions(fetch_tile=duplicate)
        )
    )
    assert failed.disposition == mln.CommandDisposition.FAILED
    del duplicate
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
    with pytest.raises(mln.InvalidArgumentError):
        map_handle.apply_camera_delta(delta)
    del delta, rejected
    assert rejected_retired.is_set()


def test_a_registered_callback_does_not_keep_its_receiver_alive() -> None:
    woke = threading.Event()
    receiver: dict[str, object] = {}

    def wake(receiver: dict[str, object] = receiver) -> None:
        if receiver.get("runtime") is not None:
            woke.set()

    callback = weakref.ref(wake)
    runtime = mln.runtime_create(
        replace(mln.RuntimeOptions.default(), event_wake=mln.Wake(wake))
    )
    receiver["runtime"] = runtime
    owner = weakref.ref(runtime)
    map_handle = result(runtime.create_map())
    result(map_handle.set_style_json(EMPTY_STYLE))
    assert woke.wait(TIMEOUT)
    result(map_handle.close())

    # The runtime roots the wake, and the wake reaches the runtime, so only
    # the cycle collector can reclaim them, which it can see through the root.
    with leak_reports() as reports:
        del map_handle, runtime, wake, receiver
        gc.collect()
    assert owner() is None
    assert callback() is None
    assert reports == ["RuntimeHandle was not explicitly closed"]


def test_a_callback_can_make_only_the_calls_its_policy_allows(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    refusals: list[BaseException] = []

    def serve(request: mln.ResourceRequest, handle: mln.ResourceRequestHandle) -> None:
        try:
            harness.runtime.barrier()
        except mln.InvalidStateError as error:
            refusals.append(error)
        # Answering its own request is what a provider callback may do.
        handle.complete(ok_response())

    harness.routes["custom://policy.json"] = serve
    map_handle.set_style_url("custom://policy.json")
    harness.wait_event(mln.RuntimeEventType.MAP_STYLE_LOADED)

    (refused,) = refusals
    assert refused.native_status_code is None
    assert "unavailable from this callback" in refused.diagnostic


def test_an_exception_in_a_callback_is_reported_and_contained(
    harness: Harness, map_handle: mln.MapHandle, monkeypatch: pytest.MonkeyPatch
) -> None:
    reports: list[object] = []
    monkeypatch.setattr(sys, "unraisablehook", reports.append)

    def fail(request: mln.ResourceRequest, handle: mln.ResourceRequestHandle) -> None:
        raise RuntimeError("provider failed")

    harness.routes["custom://raises.json"] = fail
    map_handle.set_style_url("custom://raises.json")
    # The failed callback falls back to passing the request through, and no
    # file source serves this scheme.
    harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)

    (report,) = reports
    assert isinstance(report.exc_value, RuntimeError)
    assert str(report.exc_value) == "provider failed"


class _WakeFailure(Exception):
    pass


def test_the_hook_that_receives_a_callback_exception_cannot_call_native(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    # The wake callback may make any native call, but the hook that receives
    # its exception runs on the callback's stack, where the binding refuses
    # every native call.
    refusals: list[bool] = []
    reported = threading.Event()

    def hook(report: sys.UnraisableHookArgs) -> None:
        if not isinstance(report.exc_value, _WakeFailure):
            return
        try:
            mln.network_get_status()
        except mln.InvalidStateError:
            refusals.append(True)
        else:
            refusals.append(False)
        reported.set()

    def wake() -> None:
        raise _WakeFailure

    monkeypatch.setattr(sys, "unraisablehook", hook)
    runtime = mln.runtime_create(
        replace(mln.RuntimeOptions.default(), event_wake=mln.Wake(wake))
    )
    try:
        map_handle = result(runtime.create_map())
        result(map_handle.set_style_json(EMPTY_STYLE))
        assert reported.wait(TIMEOUT)
        result(map_handle.close())
    finally:
        result(runtime.close())
    assert refusals
    assert all(refusals)


def test_a_provider_can_answer_a_request_later_from_another_thread(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    handles: list[mln.ResourceRequestHandle] = []
    provided = threading.Event()

    def defer(request: mln.ResourceRequest, handle: mln.ResourceRequestHandle) -> None:
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

    with pytest.raises(mln.InvalidStateError):
        handle.complete(ok_response())
    handle.close()


def test_a_scoped_response_refuses_use_after_its_callback_and_off_its_thread(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    # The URLs name a loopback port that nothing serves, so the rewritten
    # request fails without leaving the machine.
    style = "http://127.0.0.1:0/style.json"
    rewritten = "http://127.0.0.1:0/rewritten.json"
    scopes: list[mln.ResourceTransformResponseScope] = []
    passed_through: list[mln.ResourceRequestHandle] = []
    refusals: list[str] = []

    def refused(use: Callable[[], object]) -> None:
        try:
            use()
        except mln.InvalidStateError as error:
            refusals.append(error.diagnostic)

    def transform(
        kind: mln.ResourceKind, url: str, response: mln.ResourceTransformResponseScope
    ) -> mln.Status:
        worker = threading.Thread(
            target=refused, args=(lambda: response.set_url(rewritten),)
        )
        worker.start()
        worker.join(TIMEOUT)
        response.set_url(rewritten)
        scopes.append(response)
        return mln.Status.OK

    def pass_through(
        request: mln.ResourceRequest, handle: mln.ResourceRequestHandle
    ) -> mln.ResourceProviderDecision:
        passed_through.append(handle)
        return mln.ResourceProviderDecision.PASS_THROUGH

    harness.routes[style] = pass_through
    result(
        harness.runtime.set_resource_transform(
            mln.ResourceTransform(callback=transform)
        )
    )
    map_handle.set_style_url(style)
    harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)
    refused(lambda: scopes[0].set_url(rewritten))

    assert refusals == [
        "response belongs to its callback thread",
        "response callback has returned",
    ]
    # A request handle passed through with its callback closes with it.
    assert len(passed_through) == 1
    assert passed_through[0].closed
    with pytest.raises(mln.InvalidStateError):
        passed_through[0].complete(ok_response())


@pytest.mark.global_state
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


def test_a_registration_reported_as_rejected_is_not_rooted(harness: Harness) -> None:
    handles: list[mln.ResourceRequestHandle] = []
    provided = threading.Event()

    def hold(request: mln.ResourceRequest, handle: mln.ResourceRequestHandle) -> None:
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
        request: mln.ResourceRequest, handle: mln.ResourceRequestHandle
    ) -> mln.ResourceProviderDecision:
        seen.append((request.kind, request.requested_url))
        if request.kind == mln.ResourceKind.STYLE:
            handle.complete(ok_response(routed_style))
        else:
            handle.complete(denied_response())
        if sprites <= {kind for kind, _ in seen}:
            sprites_seen.set()
        return mln.ResourceProviderDecision.HANDLE

    routes = [
        # Another kind: claims no style or sprite, whatever its URL.
        mln.ResourceRoute(
            kinds=mln.ResourceKindMask.TILE, flags=mln.UrlMatchFlags.GLOB, url="**"
        ),
        # A URL pattern alone claims every kind under it.
        mln.ResourceRoute(
            kinds=mln.ResourceKindMask.ANY,
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
        kinds=mln.ResourceKindMask.ANY, flags=mln.UrlMatchFlags.GLOB, url="**"
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


@contextlib.contextmanager
def _loopback_server(
    requests: list[tuple[str, str | None]],
) -> Iterator[str]:
    """Serve an empty style at every path on a loopback port, and record each
    request's path and X-Rule header."""

    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self) -> None:
            requests.append((self.path, self.headers.get("X-Rule")))
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(EMPTY_STYLE)))
            self.end_headers()
            self.wfile.write(EMPTY_STYLE)

        def log_message(self, format: str, *args: object) -> None:
            pass

    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    serving = threading.Thread(target=server.serve_forever)
    serving.start()
    try:
        yield f"http://127.0.0.1:{server.server_address[1]}"
    finally:
        server.shutdown()
        serving.join(TIMEOUT)
        server.server_close()


def test_transform_rules_rewrite_a_request_without_a_callback(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    requests: list[tuple[str, str | None]] = []
    with _loopback_server(requests) as base:
        original = f"{base}/original.json"
        rewritten = f"{base}/rewritten.json"
        harness.routes[original] = lambda request, handle: (
            mln.ResourceProviderDecision.PASS_THROUGH
        )

        rewrite_rules = [
            # Another kind: no match.
            mln.ResourceRewriteRule(
                kinds=mln.ResourceKindMask.TILE,
                flags=mln.UrlMatchFlags.FLAGS_NONE,
                url=original,
                replacement_url=f"{base}/tile.json",
            ),
            mln.ResourceRewriteRule(
                kinds=mln.ResourceKindMask.STYLE,
                flags=mln.UrlMatchFlags.FLAGS_NONE,
                url=original,
                replacement_url=rewritten,
            ),
        ]
        header_rules = [
            # Header rules match the URL that the resource rules produced.
            mln.HttpHeaderRule(
                kinds=mln.ResourceKindMask.ANY,
                flags=mln.UrlMatchFlags.FLAGS_NONE,
                url=rewritten,
                headers=(mln.HttpHeader(name="X-Rule", value="applied"),),
            ),
        ]
        rewrite = harness.runtime.set_resource_transform(
            mln.ResourceTransform(rules=typing.cast(typing.Any, rewrite_rules))
        )
        headers = harness.runtime.set_http_header_transform(
            mln.HttpHeaderTransform(rules=typing.cast(typing.Any, header_rules))
        )
        # Native holds its own copies, so these changes reach no request.
        rewrite_rules.clear()
        header_rules[0] = replace(
            header_rules[0], headers=(mln.HttpHeader(name="X-Rule", value="replaced"),)
        )
        result(rewrite)
        result(headers)

        map_handle.set_style_url(original)
        harness.wait_event(mln.RuntimeEventType.MAP_STYLE_LOADED)

    assert requests == [("/rewritten.json", "applied")]
