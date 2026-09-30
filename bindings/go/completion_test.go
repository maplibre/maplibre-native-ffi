//go:build mlntest

package maplibre

import (
	"context"
	"errors"
	"testing"
)

// The first terminal result wins, and a later one is ignored. A value that
// fails to convert, whether by error or by panic, fails the future instead of
// unwinding into native, and an owned value that a failed conversion adopted
// is disposed: its map no longer holds the runtime open.
func TestCompletionDeliversExactlyOnce(t *testing.T) {
	future, deliver := int32CompletionForTest(func(value int32) (int32, error) { return value, nil })
	first, second := int32(7), int32(8)
	deliver(0, &first)
	deliver(0, &second)
	for range 2 {
		if value := await(t, future); value != 7 {
			t.Fatalf("Await() = %d, want the first delivery", value)
		}
	}

	missing, deliver := int32CompletionForTest(func(value int32) (int32, error) { return value, nil })
	deliver(0, nil)
	if _, err := missing.Await(context.Background()); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("a result with no value: %v, want ErrInvalidState", err)
	}

	panicking, deliver := int32CompletionForTest(func(int32) (int32, error) { panic("conversion failed") })
	deliver(0, &first)
	if _, err := panicking.Await(context.Background()); !errors.Is(err, ErrNative) {
		t.Fatalf("a panicking conversion: %v, want ErrNative", err)
	}

	f := newRuntimeFixture(t)
	owned, err := mapCreateFailingAfterAdoptionForTest(f.runtime)
	if err != nil {
		t.Fatal(err)
	}
	receive(t, owned.Done(), "the failing map creation")
	if _, err := owned.Await(context.Background()); !errors.Is(err, ErrNative) {
		t.Fatalf("a conversion that failed after adopting its map: %v, want ErrNative", err)
	}
	closeOnceCollected(t, f.runtime, "the disposal of the map the failed conversion adopted")
}

// A submission native refuses returns its error at once, and the binding
// deletes the handle it passed as the completion's user data, since native
// never calls the completion or its release.
func TestRejectedSubmissionFreesItsCompletionState(t *testing.T) {
	handle, err := rejectedSubmissionForTest()
	if !errors.Is(err, ErrInvalidState) {
		t.Fatalf("rejected submission: %v, want ErrInvalidState", err)
	}
	defer func() {
		if recover() == nil {
			t.Fatal("the rejected submission's handle is still live")
		}
	}()
	handle.Value()
}

// A failed command is a disposition in the result, not an error from Await,
// and carries its status and diagnostic.
func TestFailedCommandDispositionIsData(t *testing.T) {
	f := newFixture(t)
	awaitCommitted(t, submitted(f.m.SetStyleJson([]byte(emptyStyle))))
	completion := await(t, submitted(f.m.RemoveStyleSource("missing")))
	if completion.Disposition != CommandDispositionFailed || !errors.Is(kindForStatus(completion.RawStatus), ErrNotFound) {
		t.Fatalf("RemoveStyleSource(missing) = %+v, want a failed NOT_FOUND disposition", completion)
	}
	if completion.Diagnostic == "" {
		t.Fatal("the failed disposition carried no diagnostic")
	}
}

// Await returns the context's error when the context ends first, and a nil
// Future reports an error rather than blocking.
func TestAwaitEndsWithItsContext(t *testing.T) {
	pending, _ := int32CompletionForTest(func(value int32) (int32, error) { return value, nil })
	cancelled, cancel := context.WithCancel(context.Background())
	cancel()
	if _, err := pending.Await(cancelled); !errors.Is(err, context.Canceled) {
		t.Fatalf("Await(cancelled) = %v, want context.Canceled", err)
	}
	expired, cancel := context.WithTimeout(context.Background(), 0)
	defer cancel()
	if _, err := pending.Await(expired); !errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("Await(expired) = %v, want context.DeadlineExceeded", err)
	}

	var missing *Future[int32]
	<-missing.Done()
	if _, err := missing.Await(context.Background()); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil Future Await = %v, want ErrInvalidArgument", err)
	}
}
