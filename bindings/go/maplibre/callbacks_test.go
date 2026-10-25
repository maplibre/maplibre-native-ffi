package maplibre

import (
	"context"
	"errors"
	"fmt"
	"log/slog"
	stdruntime "runtime"
	"strings"
	"testing"
	"time"
	"weak"
)

// rootCount reports how many callback registrations owner roots.
func rootCount(owner *bindingOwner) int {
	owner.rootsMu.Lock()
	defer owner.rootsMu.Unlock()
	return len(owner.roots)
}

// failureLog collects callback-side failures, which a callback on a native
// thread cannot report through t.
type failureLog chan error

func newFailureLog() failureLog { return make(failureLog, 16) }

func (log failureLog) add(format string, args ...any) {
	select {
	case log <- fmt.Errorf(format, args...):
	default:
	}
}

func (log failureLog) check(t *testing.T) {
	t.Helper()
	select {
	case err := <-log:
		t.Fatal(err)
	default:
	}
}

func receive[T any](t *testing.T, values <-chan T, what string) T {
	t.Helper()
	select {
	case value := <-values:
		return value
	case <-time.After(testTimeout()):
		t.Fatalf("timed out waiting for %s", what)
		panic("unreachable")
	}
}

// offer hands value to a test without blocking the native thread that calls
// a callback. A value the channel has no room for is dropped.
func offer[T any](values chan<- T, value T) {
	select {
	case values <- value:
	default:
	}
}

func isStyleLoaded(event RuntimeEvent) bool { return event.Type == RuntimeEventTypeMapStyleLoaded }

func isLoadingFailure(url string) func(RuntimeEvent) bool {
	return func(event RuntimeEvent) bool {
		return event.Type == RuntimeEventTypeMapLoadingFailed && strings.Contains(event.Message, url)
	}
}

// A registration is rooted on its owner from the accepted submission until
// native releases it: at removal, at map close, and after a command that
// fails. A submission rejected up front never roots it.
func TestCallbackRootsFollowTheNativeRegistration(t *testing.T) {
	f := newFixture(t)
	await(t, submitted(f.m.SetStyleJson([]byte(emptyStyle))))
	options := CustomGeometrySourceOptions{FetchTile: func(CanonicalTileId) {}}

	await(t, submitted(f.m.AddCustomGeometrySource("removed", options)))
	if roots := rootCount(f.m.bindingOwner); roots != 1 {
		t.Fatalf("roots after the add = %d, want 1", roots)
	}
	await(t, submitted(f.m.RemoveStyleSource("removed")))
	if roots := rootCount(f.m.bindingOwner); roots != 0 {
		t.Fatalf("roots after the removal = %d, want 0", roots)
	}

	for _, rejected := range []struct {
		id      string
		options CustomGeometrySourceOptions
	}{{"", options}, {"rejected", CustomGeometrySourceOptions{}}} {
		if _, err := f.m.AddCustomGeometrySource(rejected.id, rejected.options); !errors.Is(err, StatusInvalidArgument) {
			t.Fatalf("add %q = %v, want StatusInvalidArgument", rejected.id, err)
		}
		if roots := rootCount(f.m.bindingOwner); roots != 0 {
			t.Fatalf("roots after a rejected add of %q = %d, want 0", rejected.id, roots)
		}
	}

	await(t, submitted(f.m.AddCustomGeometrySource("kept", options)))
	awaitFailure(t, submitted(f.m.AddCustomGeometrySource("kept", options)))
	// Native releases a failed command's registration after its completion
	// returns, and before a later command on the map completes.
	await(t, submitted(f.m.RequestRepaint()))
	if roots := rootCount(f.m.bindingOwner); roots != 1 {
		t.Fatalf("roots after a failed add = %d, want 1", roots)
	}
	await(t, submitted(f.m.Close()))
	if roots := rootCount(f.m.bindingOwner); roots != 0 {
		t.Fatalf("roots after the map closed = %d, want 0", roots)
	}
}

// A camera command's end handler sits two records deep in its input. The map
// roots it while the ease runs, and native runs it once and releases it when
// its identity is cancelled. A command that native rejects roots nothing.
func TestCameraEndHandlerIsRootedUntilItRuns(t *testing.T) {
	f := newFixture(t)
	ends := make(chan CameraTransitionEnd, 2)
	zoom := 4.0
	duration := 60000.0
	identity := uint64(1<<64 - 1)
	update := DefaultCameraUpdate()
	update.Mode = CameraUpdateModeEase
	update.Camera.Zoom = &zoom
	update.Animation.DurationMs = &duration
	update.Animation.TransitionId = &identity
	update.Animation.EndHandler = CameraTransitionHandler{Callback: func(end CameraTransitionEnd) { ends <- end }}
	await(t, submitted(f.m.UpdateCamera(update)))
	if roots := rootCount(f.m.bindingOwner); roots != 1 {
		t.Fatalf("roots while the ease runs = %d, want 1", roots)
	}
	await(t, submitted(f.m.CancelCameraTransition(identity)))
	select {
	case end := <-ends:
		if end.Outcome != CameraTransitionOutcomeCancelled || end.Generation == 0 {
			t.Fatalf("end = %+v, want a cancellation with a generation", end)
		}
	case <-time.After(testTimeout()):
		t.Fatal("the end handler never ran")
	}
	if roots := rootCount(f.m.bindingOwner); roots != 0 {
		t.Fatalf("roots after the end = %d, want 0", roots)
	}

	scale := -1.0
	delta := DefaultCameraDelta()
	delta.Scale = &scale
	delta.Animation.EndHandler = update.Animation.EndHandler
	if _, err := f.m.ApplyCameraDelta(delta); !errors.Is(err, StatusInvalidArgument) {
		t.Fatalf("apply delta = %v, want StatusInvalidArgument", err)
	}
	if roots := rootCount(f.m.bindingOwner); roots != 0 {
		t.Fatalf("roots after a rejected delta = %d, want 0", roots)
	}
	if len(ends) != 0 {
		t.Fatal("the end handler ran more than once")
	}
}

// discardProviderCycle registers a provider whose closure captures its
// runtime, and drops every other reference to both.
func discardProviderCycle(t *testing.T) weak.Pointer[bindingOwner] {
	t.Helper()
	host, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatal(err)
	}
	await(t, submitted(host.SetResourceProvider(ResourceProvider{Callback: func(_ ResourceRequestInfo, request *ResourceRequestHandle) {
		stdruntime.KeepAlive(host)
		_ = request.PassThrough()
	}})))
	return weak.Make(host.bindingOwner)
}

// Native holds a registration weakly, so a callback that captures its own
// receiver leaves a cycle the collector reclaims.
func TestCallbackDoesNotKeepItsReceiverAlive(t *testing.T) {
	awaitUnreachable(t, discardProviderCycle(t), "the runtime in a cycle with its provider")
}

// A panic in a callback stays in the binding, which settles what native waits
// for: a transform's request keeps its URL, and a provider's request fails.
// The binding logs each panic through the default slog logger.
func TestCallbackPanicIsContained(t *testing.T) {
	panics := capturePanicLogs(t)
	f := newFixture(t)
	base := f.serveLoopback(t, map[string]string{"/original.json": emptyStyle})
	panicked := make(chan struct{}, 1)
	await(t, submitted(f.runtime.SetResourceTransform(ResourceTransform{Callback: func(ResourceKind, string, *ResourceTransformRequestHandle) {
		offer(panicked, struct{}{})
		panic("transform failed")
	}})))
	if _, err := f.m.SetStyleUrl(base + "/original.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the style load from the original URL", isStyleLoaded)
	receive(t, panicked, "the transform's panic")
	if got := receive(t, panics, "the transform's panic log"); got != (callbackPanic{"mln_resource_transform_callback", "transform failed"}) {
		t.Fatalf("logged %+v", got)
	}

	await(t, submitted(f.runtime.ClearResourceTransform()))
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(ResourceRequestInfo, *ResourceRequestHandle) {
		panic("provider failed")
	}})))
	if _, err := f.m.SetStyleUrl("custom://unserved.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the provider's loading failure", func(event RuntimeEvent) bool {
		return event.Type == RuntimeEventTypeMapLoadingFailed
	})
	if got := receive(t, panics, "the provider's panic log"); got != (callbackPanic{"mln_resource_provider_callback", "provider failed"}) {
		t.Fatalf("logged %+v", got)
	}
	select {
	case extra := <-panics:
		t.Fatalf("logged another panic: %+v", extra)
	default:
	}
}

// A panic report contains a failure in the platform logger.
func TestCallbackPanicReportIsContained(t *testing.T) {
	previous := slog.Default()
	slog.SetDefault(slog.New(failingPanicLogHandler{previous.Handler()}))
	defer slog.SetDefault(previous)
	bindingReportCallbackPanic("mln_wake_callback", "wake failed")
}

type failingPanicLogHandler struct{ slog.Handler }

func (failingPanicLogHandler) Enabled(context.Context, slog.Level) bool { return true }

func (failingPanicLogHandler) Handle(_ context.Context, record slog.Record) error {
	if record.Message == "maplibre: callback panicked" {
		panic("logger failed")
	}
	return nil
}

// callbackPanic is one panic that the binding logged.
type callbackPanic struct {
	callback, panic string
}

// capturePanicLogs routes the default slog logger's callback panics to the
// returned channel until the test ends.
func capturePanicLogs(t *testing.T) <-chan callbackPanic {
	panics := make(chan callbackPanic, 16)
	previous := slog.Default()
	slog.SetDefault(slog.New(panicLogHandler(panics)))
	t.Cleanup(func() { slog.SetDefault(previous) })
	return panics
}

type panicLogHandler chan<- callbackPanic

func (panicLogHandler) Enabled(context.Context, slog.Level) bool   { return true }
func (handler panicLogHandler) WithAttrs([]slog.Attr) slog.Handler { return handler }
func (handler panicLogHandler) WithGroup(string) slog.Handler      { return handler }

func (handler panicLogHandler) Handle(_ context.Context, record slog.Record) error {
	if record.Level != slog.LevelError || record.Message != "maplibre: callback panicked" {
		return nil
	}
	var logged callbackPanic
	record.Attrs(func(attr slog.Attr) bool {
		switch attr.Key {
		case "callback":
			logged.callback = attr.Value.String()
		case "panic":
			logged.panic = fmt.Sprint(attr.Value.Any())
		}
		return true
	})
	offer(handler, logged)
	return nil
}

// A provider can take a request and answer it later from another goroutine,
// once.
func TestProviderAnswersADeferredRequest(t *testing.T) {
	f := newFixture(t)
	requests := make(chan *ResourceRequestHandle, 1)
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(_ ResourceRequestInfo, request *ResourceRequestHandle) {
		offer(requests, request)
	}})))
	if _, err := f.m.SetStyleUrl("custom://deferred.json"); err != nil {
		t.Fatal(err)
	}
	request := receive(t, requests, "the provider's request")
	if err := request.Complete(ResourceResponse{Status: ResourceResponseStatusOk, Bytes: []byte(emptyStyle)}); err != nil {
		t.Fatalf("Complete: %v", err)
	}
	// The accepted completion consumed the handle, so native refuses a second
	// one.
	if err := request.Complete(ResourceResponse{Status: ResourceResponseStatusOk}); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("a second Complete = %v, want StatusInvalidState", err)
	}
	f.awaitEvent(t, "the deferred style load", isStyleLoaded)
}

// Native copies a provider's routes, so the caller may reuse the slice once the
// submission returns. Only a request that a route matches reaches the
// callback, and any other passes through to native loading. A route that names
// a kind claims no other kind, and one that names only a URL pattern claims
// every kind.
func TestProviderRoutesDecideWhichRequestsReachIt(t *testing.T) {
	f := newFixture(t)
	const routedStyle = `{"version":8,"sources":{"tiles":{"type":"vector","url":"custom://any/tiles.json"}},"layers":[]}`
	type routed struct {
		url  string
		kind ResourceKind
	}
	requests := make(chan routed, 8)
	deny := denyingProvider().Callback
	allKindsRoute := DefaultResourceRoute()
	allKindsRoute.Flags = UrlMatchFlagsGlob
	allKindsRoute.Url = "custom://any/**"
	routes := []ResourceRoute{
		{Kinds: ResourceKindMaskTile, Flags: UrlMatchFlagsGlob, Url: "**"},
		{Kinds: ResourceKindMaskStyle, Url: "custom://routed/style.json"},
		allKindsRoute,
	}
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Routes: routes, Callback: func(request ResourceRequestInfo, handle *ResourceRequestHandle) {
		offer(requests, routed{*request.RequestedUrl, request.Kind})
		if *request.RequestedUrl != "custom://routed/style.json" {
			deny(request, handle)
			return
		}
		_ = handle.Complete(ResourceResponse{Status: ResourceResponseStatusOk, Bytes: []byte(routedStyle)})
	}})))
	for i := range routes {
		routes[i] = ResourceRoute{Url: "custom://overwritten.json"}
	}

	if _, err := f.m.SetStyleUrl("custom://routed/style.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the routed style load", isStyleLoaded)
	for _, want := range []routed{
		{"custom://routed/style.json", ResourceKindStyle},
		{"custom://any/tiles.json", ResourceKindSource},
	} {
		if got := receive(t, requests, want.url); got != want {
			t.Fatalf("the provider received %+v, want %+v", got, want)
		}
	}

	// No route claims this style, so native loading fails it for its scheme.
	if _, err := f.m.SetStyleUrl("custom://unrouted/style.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the unrouted style's loading failure", isLoadingFailure("custom://unrouted/style.json"))
	select {
	case got := <-requests:
		t.Fatalf("the provider received %+v, which no route matches", got)
	default:
	}
}

// A header transform's response object works only inside its invocation and
// on the thread native called it on.
func TestScopedResponseRejectsLateAndForeignUse(t *testing.T) {
	if !headerTransformSupported {
		t.Skip("HTTP header transforms are unsupported on OpenHarmony")
	}
	f := newFixture(t)
	failures := newFailureLog()
	responses := make(chan *HttpHeaderTransformResponseScope, 1)
	await(t, submitted(f.runtime.SetHttpHeaderTransform(HttpHeaderTransform{Callback: func(_ ResourceKind, _ string, response *HttpHeaderTransformResponseScope) Status {
		foreign := make(chan error, 1)
		go func() { foreign <- response.SetHeader("X-Test", "1") }()
		if err := <-foreign; !errors.Is(err, StatusInvalidState) {
			failures.add("SetHeader from another goroutine = %v, want StatusInvalidState", err)
		}
		offer(responses, response)
		return StatusOk
	}})))
	base := f.serveLoopback(t, map[string]string{"/style.json": emptyStyle})
	if _, err := f.m.SetStyleUrl(base + "/style.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the style load", isStyleLoaded)
	failures.check(t)
	response := receive(t, responses, "the header transform's response")
	if err := response.SetHeader("X-Test", "late"); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("SetHeader after the callback returned = %v, want StatusInvalidState", err)
	}
}

// A transform owns the request its callback receives, and another goroutine
// answers it after the callback returns; MapLibre fetches the answer.
func TestTransformRequestAnsweredAfterItsCallback(t *testing.T) {
	f := newFixture(t)
	base := f.serveLoopback(t, map[string]string{"/rewritten.json": emptyStyle})
	requests := make(chan *ResourceTransformRequestHandle, 1)
	await(t, submitted(f.runtime.SetResourceTransform(ResourceTransform{Callback: func(_ ResourceKind, url string, request *ResourceTransformRequestHandle) {
		if strings.HasSuffix(url, "/original.json") {
			requests <- request
			return
		}
		_ = request.Close()
	}})))
	if _, err := f.m.SetStyleUrl(base + "/original.json"); err != nil {
		t.Fatal(err)
	}
	request := receive(t, requests, "the transform's request")
	answered := make(chan error, 1)
	go func() {
		answered <- request.Complete(base + "/rewritten.json")
	}()
	if err := receive(t, answered, "the answer"); err != nil {
		t.Fatalf("Complete: %v", err)
	}
	// The accepted completion consumed the handle, so native refuses a second
	// one.
	if err := request.Complete(base + "/rewritten.json"); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("a second Complete = %v, want StatusInvalidState", err)
	}
	f.awaitEvent(t, "the style load from the answer", isStyleLoaded)
}

// Installing a process-global callback replaces the registration native held,
// and native releases the one it dropped, as clearing releases the last.
func TestReplacingAGlobalCallbackReleasesThePreviousOne(t *testing.T) {
	globalRoots := func() int {
		bindingGlobalRoots.Lock()
		defer bindingGlobalRoots.Unlock()
		return len(bindingGlobalRoots.values)
	}
	t.Cleanup(func() { _ = LogClearCallback() })
	baseline := globalRoots()
	for range 2 {
		if err := LogSetCallback(LogHandler{Callback: func(LogSeverity, LogEvent, int64, string) uint32 { return 0 }}); err != nil {
			t.Fatal(err)
		}
		if live := globalRoots() - baseline; live != 1 {
			t.Fatalf("global roots after an install = %d, want 1", live)
		}
	}
	if err := LogClearCallback(); err != nil {
		t.Fatal(err)
	}
	if live := globalRoots() - baseline; live != 0 {
		t.Fatalf("global roots after the clear = %d, want 0", live)
	}
}

// A cancel callback is rooted only when native stores it. On a live request it
// is rooted until it runs, and a second one is refused; on a request native
// already cancelled, SetCancelCallback reports the cancellation, roots
// nothing, and never runs the callback.
func TestCancelCallbackIsRootedOnlyWhenStored(t *testing.T) {
	f := newFixture(t)
	requests := make(chan *ResourceRequestHandle, 2)
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(_ ResourceRequestInfo, request *ResourceRequestHandle) {
		offer(requests, request)
	}})))

	if _, err := f.m.SetStyleUrl("custom://held.json"); err != nil {
		t.Fatal(err)
	}
	held := receive(t, requests, "the held request")
	ran := make(chan struct{})
	if cancelled, err := held.SetCancelCallback(ResourceRequestCancelHandler{Callback: func() { close(ran) }}); cancelled || err != nil {
		t.Fatalf("SetCancelCallback on a live request = (%v, %v), want (false, nil)", cancelled, err)
	}
	if _, err := held.SetCancelCallback(ResourceRequestCancelHandler{Callback: func() {}}); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("a second SetCancelCallback = %v, want StatusInvalidState", err)
	}
	if roots := rootCount(held.bindingOwner); roots != 1 {
		t.Fatalf("roots on the live request = %d, want 1", roots)
	}
	// Closing the map discards the request, which runs the callback.
	await(t, submitted(f.m.Close()))
	receive(t, ran, "the cancel callback")
	awaitCondition(t, "the cancel callback's release", func() bool { return rootCount(held.bindingOwner) == 0 })

	other := createMap(t, f.runtime, DefaultMapOptions())
	if _, err := other.SetStyleUrl("custom://discarded.json"); err != nil {
		t.Fatal(err)
	}
	discarded := receive(t, requests, "the discarded request")
	await(t, submitted(other.Close()))
	// Nothing signals a cancellation that has no callback to run.
	awaitCondition(t, "the request's cancellation", func() bool {
		cancelled, err := discarded.IsCancelled()
		return err != nil || cancelled
	})
	cancelled, err := discarded.SetCancelCallback(ResourceRequestCancelHandler{Callback: func() { t.Error("a callback ran for an already cancelled request") }})
	if !cancelled || err != nil {
		t.Fatalf("SetCancelCallback on a cancelled request = (%v, %v), want (true, nil)", cancelled, err)
	}
	if roots := rootCount(discarded.bindingOwner); roots != 0 {
		t.Fatalf("roots on the cancelled request = %d, want 0", roots)
	}
	if err := discarded.Close(); err != nil {
		t.Fatal(err)
	}
	if _, err := discarded.SetCancelCallback(ResourceRequestCancelHandler{Callback: func() {}}); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("SetCancelCallback after Close = %v, want StatusInvalidState", err)
	}
	if err := held.Close(); err != nil {
		t.Fatal(err)
	}
}
