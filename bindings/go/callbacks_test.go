package maplibre

import (
	"context"
	"errors"
	"fmt"
	"log/slog"
	stdruntime "runtime"
	"strings"
	"sync"
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
	awaitCommitted(t, submitted(f.m.SetStyleJson([]byte(emptyStyle))))
	options := CustomGeometrySourceOptions{FetchTile: func(CanonicalTileId) {}}

	awaitCommitted(t, submitted(f.m.AddCustomGeometrySource("removed", options)))
	if roots := rootCount(f.m.bindingOwner); roots != 1 {
		t.Fatalf("roots after the add = %d, want 1", roots)
	}
	awaitCommitted(t, submitted(f.m.RemoveStyleSource("removed")))
	if roots := rootCount(f.m.bindingOwner); roots != 0 {
		t.Fatalf("roots after the removal = %d, want 0", roots)
	}

	for _, rejected := range []struct {
		id      string
		options CustomGeometrySourceOptions
	}{{"", options}, {"rejected", CustomGeometrySourceOptions{}}} {
		if _, err := f.m.AddCustomGeometrySource(rejected.id, rejected.options); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("add %q = %v, want ErrInvalidArgument", rejected.id, err)
		}
		if roots := rootCount(f.m.bindingOwner); roots != 0 {
			t.Fatalf("roots after a rejected add of %q = %d, want 0", rejected.id, roots)
		}
	}

	awaitCommitted(t, submitted(f.m.AddCustomGeometrySource("kept", options)))
	if completion := await(t, submitted(f.m.AddCustomGeometrySource("kept", options))); completion.Disposition != CommandDispositionFailed {
		t.Fatalf("add with a taken ID = %+v, want failed", completion)
	}
	// Native releases a failed command's registration after its completion
	// returns, and before the runtime's next barrier.
	await(t, submitted(f.runtime.Barrier()))
	if roots := rootCount(f.m.bindingOwner); roots != 1 {
		t.Fatalf("roots after a failed add = %d, want 1", roots)
	}
	await(t, submitted(f.m.Close()))
	if roots := rootCount(f.m.bindingOwner); roots != 0 {
		t.Fatalf("roots after the map closed = %d, want 0", roots)
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
	await(t, submitted(host.SetResourceProvider(ResourceProvider{Callback: func(ResourceRequest, *ResourceRequestHandle) ResourceProviderDecision {
		stdruntime.KeepAlive(host)
		return ResourceProviderDecisionPassThrough
	}})))
	return weak.Make(host.bindingOwner)
}

// Native holds a registration weakly, so a callback that captures its own
// receiver leaves a cycle the collector reclaims.
func TestCallbackDoesNotKeepItsReceiverAlive(t *testing.T) {
	awaitUnreachable(t, discardProviderCycle(t), "the runtime in a cycle with its provider")
}

// Inside a callback, the calls its declaration allows succeed, and any other
// call on the binding fails before it reaches native.
func TestCallbackAdmissionPolicy(t *testing.T) {
	f := newFixture(t)
	failures := newFailureLog()
	invoked := make(chan struct{}, 1)
	await(t, submitted(f.runtime.SetResourceTransform(ResourceTransform{Callback: func(_ ResourceKind, url string, response *ResourceTransformResponseScope) Status {
		offer(invoked, struct{}{})
		_, err := f.runtime.Barrier()
		var native *Error
		if !errors.As(err, &native) || !strings.Contains(native.Diagnostic(), "unavailable in this native callback") {
			failures.add("Barrier inside the transform = %v, want the admission error", err)
		}
		if _, ok := native.RawStatus(); ok {
			failures.add("the admission error came from native")
		}
		if err := response.SetUrl(url); err != nil {
			failures.add("SetUrl inside the transform: %v", err)
		}
		return StatusOk
	}})))
	base := f.serveLoopback(t, map[string]string{"/style.json": emptyStyle})
	if _, err := f.m.SetStyleUrl(base + "/style.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the style load", isStyleLoaded)
	receive(t, invoked, "the transform's invocation")
	failures.check(t)
}

// Admission is per OS thread. A goroutine that a callback starts runs on
// another thread, so a call the callback may not make succeeds there while the
// callback still runs. The callback waits only for the submission's acceptance,
// which needs nothing from the callback's thread.
func TestGoroutineFromCallbackIsNotRestricted(t *testing.T) {
	f := newFixture(t)
	failures := newFailureLog()
	type barrier struct {
		future *Future[struct{}]
		err    error
	}
	barriers := make(chan barrier, 1)
	var spawn sync.Once
	await(t, submitted(f.runtime.SetResourceTransform(ResourceTransform{Callback: func(_ ResourceKind, _ string, _ *ResourceTransformResponseScope) Status {
		spawn.Do(func() {
			if _, err := f.runtime.Barrier(); !errors.Is(err, ErrInvalidState) {
				failures.add("Barrier inside the transform = %v, want ErrInvalidState", err)
			}
			accepted := make(chan barrier, 1)
			go func() {
				future, err := f.runtime.Barrier()
				accepted <- barrier{future, err}
			}()
			select {
			case result := <-accepted:
				barriers <- result
			case <-time.After(testTimeout()):
				failures.add("the goroutine's Barrier did not return while the transform ran")
			}
		})
		return StatusOk
	}})))
	base := f.serveLoopback(t, map[string]string{"/style.json": emptyStyle})
	if _, err := f.m.SetStyleUrl(base + "/style.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the style load", isStyleLoaded)
	failures.check(t)
	result := receive(t, barriers, "the goroutine's barrier")
	if result.err != nil {
		t.Fatalf("Barrier from the goroutine: %v", result.err)
	}
	await(t, result.future)
}

// A panic in a callback stays in the binding, which returns the callback's
// declared failure value to native: a transform's rewrite is dropped, and a
// provider's request passes through to the built-in sources. The binding logs
// each panic through the default slog logger.
func TestCallbackPanicIsContained(t *testing.T) {
	panics := capturePanicLogs(t)
	f := newFixture(t)
	base := f.serveLoopback(t, map[string]string{"/original.json": emptyStyle})
	panicked := make(chan struct{}, 1)
	await(t, submitted(f.runtime.SetResourceTransform(ResourceTransform{Callback: func(_ ResourceKind, _ string, response *ResourceTransformResponseScope) Status {
		_ = response.SetUrl(base + "/rewritten.json")
		offer(panicked, struct{}{})
		panic("transform failed")
	}})))
	if _, err := f.m.SetStyleUrl(base + "/original.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the style load from the original URL", isStyleLoaded)
	receive(t, panicked, "the transform's panic")
	if got := receive(t, panics, "the transform's panic log"); got != (callbackPanic{"mln_resource_transform_callback", "transform failed", true}) {
		t.Fatalf("logged %+v", got)
	}

	await(t, submitted(f.runtime.ClearResourceTransform()))
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(ResourceRequest, *ResourceRequestHandle) ResourceProviderDecision {
		panic("provider failed")
	}})))
	if _, err := f.m.SetStyleUrl("custom://unserved.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the pass-through's loading failure", isLoadingFailure("custom://unserved.json"))
	if got := receive(t, panics, "the provider's panic log"); got != (callbackPanic{"mln_resource_provider_callback", "provider failed", true}) {
		t.Fatalf("logged %+v", got)
	}
	select {
	case extra := <-panics:
		t.Fatalf("logged another panic: %+v", extra)
	default:
	}
}

// A panic log reaches its handler on the callback's stack, so the handler may
// not reenter native, even from a callback such as the wake callback that
// admits every native call.
func TestCallbackPanicHandlerCannotCallNative(t *testing.T) {
	panics := capturePanicLogs(t)
	// A trampoline holds its thread while it reports, as this test does.
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()
	bindingReportCallbackPanic("mln_wake_callback", "wake failed")
	if got := receive(t, panics, "the wake's panic log"); got != (callbackPanic{"mln_wake_callback", "wake failed", true}) {
		t.Fatalf("logged %+v", got)
	}
}

// callbackPanic is one panic that the binding logged, and whether the binding
// refused a native call from the log's handler.
type callbackPanic struct {
	callback, panic string
	refused         bool
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
	_, err := NetworkGetStatus()
	logged.refused = errors.Is(err, ErrInvalidState)
	offer(handler, logged)
	return nil
}

// A provider can take a request and answer it later from another goroutine.
func TestProviderAnswersADeferredRequest(t *testing.T) {
	f := newFixture(t)
	requests := make(chan *ResourceRequestHandle, 1)
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(_ ResourceRequest, request *ResourceRequestHandle) ResourceProviderDecision {
		offer(requests, request)
		return ResourceProviderDecisionHandle
	}})))
	if _, err := f.m.SetStyleUrl("custom://deferred.json"); err != nil {
		t.Fatal(err)
	}
	request := receive(t, requests, "the provider's request")
	if err := request.Complete(ResourceResponse{Status: ResourceResponseStatusOk, Bytes: []byte(emptyStyle)}); err != nil {
		t.Fatalf("Complete: %v", err)
	}
	if err := request.Close(); err != nil {
		t.Fatalf("Close: %v", err)
	}
	f.awaitEvent(t, "the deferred style load", isStyleLoaded)
}

// Inside the decision, the request handle decides the outcome: a completion
// claims the request even when the callback returns pass-through, and a close
// without one fails the request. Closing twice is safe.
func TestProviderDecisionFollowsTheRequestHandle(t *testing.T) {
	f := newFixture(t)
	failures := newFailureLog()
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if *request.RequestedUrl == "custom://answered.json" {
			if err := handle.Complete(ResourceResponse{Status: ResourceResponseStatusOk, Bytes: []byte(emptyStyle)}); err != nil {
				failures.add("Complete: %v", err)
			}
		}
		for range 2 {
			if err := handle.Close(); err != nil {
				failures.add("Close: %v", err)
			}
		}
		return ResourceProviderDecisionPassThrough
	}})))
	if _, err := f.m.SetStyleUrl("custom://answered.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the answered style load", isStyleLoaded)
	if _, err := f.m.SetStyleUrl("custom://closed.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEventType(t, RuntimeEventTypeMapLoadingFailed)
	failures.check(t)
}

// A transform's response object works only inside its invocation and on the
// thread native called it on.
func TestScopedResponseRejectsLateAndForeignUse(t *testing.T) {
	f := newFixture(t)
	failures := newFailureLog()
	responses := make(chan *ResourceTransformResponseScope, 1)
	await(t, submitted(f.runtime.SetResourceTransform(ResourceTransform{Callback: func(_ ResourceKind, url string, response *ResourceTransformResponseScope) Status {
		foreign := make(chan error, 1)
		go func() { foreign <- response.SetUrl(url) }()
		if err := <-foreign; !errors.Is(err, ErrInvalidState) {
			failures.add("SetUrl from another goroutine = %v, want ErrInvalidState", err)
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
	response := receive(t, responses, "the transform's response")
	if err := response.SetUrl(base + "/late.json"); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("SetUrl after the callback returned = %v, want ErrInvalidState", err)
	}
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
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(_ ResourceRequest, request *ResourceRequestHandle) ResourceProviderDecision {
		offer(requests, request)
		return ResourceProviderDecisionHandle
	}})))

	if _, err := f.m.SetStyleUrl("custom://held.json"); err != nil {
		t.Fatal(err)
	}
	held := receive(t, requests, "the held request")
	ran := make(chan struct{})
	if cancelled, err := held.SetCancelCallback(ResourceRequestCancelHandler{Callback: func() { close(ran) }}); cancelled || err != nil {
		t.Fatalf("SetCancelCallback on a live request = (%v, %v), want (false, nil)", cancelled, err)
	}
	if _, err := held.SetCancelCallback(ResourceRequestCancelHandler{Callback: func() {}}); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("a second SetCancelCallback = %v, want ErrInvalidState", err)
	}
	if roots := rootCount(held.bindingOwner); roots != 1 {
		t.Fatalf("roots on the live request = %d, want 1", roots)
	}
	// Closing the map discards the request, which runs the callback.
	await(t, submitted(f.m.Close()))
	receive(t, ran, "the cancel callback")
	awaitCondition(t, "the cancel callback's release", func() bool { return rootCount(held.bindingOwner) == 0 })

	other := await(t, submitted(f.runtime.CreateMap(DefaultMapOptions())))
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
	if _, err := discarded.SetCancelCallback(ResourceRequestCancelHandler{Callback: func() {}}); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("SetCancelCallback after Close = %v, want ErrInvalidState", err)
	}
	if err := held.Close(); err != nil {
		t.Fatal(err)
	}
}
