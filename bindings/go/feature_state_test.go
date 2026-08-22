package maplibre

import (
	"bytes"
	"errors"
	"testing"
)

// Feature state belongs to the map store, so a set is observed by an ordered
// get without a render session or a loaded source.
func TestMapFeatureStateRoundTrip(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)

	featureID := "42"
	selector := FeatureStateSelector{SourceID: "source", FeatureID: &featureID}

	// Missing feature state reads as an empty JSON object.
	before, err := awaitForTest(m.FeatureState(selector))
	if err != nil {
		t.Fatalf("FeatureState() before set: %v", err)
	}
	if !bytes.Equal(before, []byte("{}")) {
		t.Fatalf("FeatureState() before set = %q, want an empty object", before)
	}

	setID, err := m.SetFeatureState(selector, []byte(`{"hover":true}`))
	requireCommandCommitted(t, runtime, setID, err)
	after, err := awaitForTest(m.FeatureState(selector))
	if err != nil {
		t.Fatalf("FeatureState() after set: %v", err)
	}
	if !bytes.Contains(after, []byte("hover")) {
		t.Fatalf("FeatureState() after set = %q, want the stored hover key", after)
	}

	// Removing one key returns the feature to the empty object.
	stateKey := "hover"
	removeID, err := m.RemoveFeatureState(FeatureStateSelector{
		SourceID:  "source",
		FeatureID: &featureID,
		StateKey:  &stateKey,
	})
	requireCommandCommitted(t, runtime, removeID, err)
	cleared, err := awaitForTest(m.FeatureState(selector))
	if err != nil {
		t.Fatalf("FeatureState() after remove: %v", err)
	}
	if !bytes.Equal(cleared, []byte("{}")) {
		t.Fatalf("FeatureState() after remove = %q, want an empty object", cleared)
	}
}

func TestMapFeatureStateRejectsInvalidSelectors(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	// Set and get require a feature ID.
	noFeature := FeatureStateSelector{SourceID: "source"}
	if _, err := m.SetFeatureState(noFeature, []byte(`{"hover":true}`)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetFeatureState() without feature ID error = %v, want ErrInvalidArgument", err)
	}
	if _, err := m.FeatureState(noFeature); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("FeatureState() without feature ID error = %v, want ErrInvalidArgument", err)
	}

	// A state key without a feature ID selects nothing removable.
	stateKey := "hover"
	if _, err := m.RemoveFeatureState(FeatureStateSelector{
		SourceID: "source",
		StateKey: &stateKey,
	}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("RemoveFeatureState() with key but no feature ID error = %v, want ErrInvalidArgument", err)
	}

	// State must be one JSON object. An empty view is rejected before
	// acceptance; a non-object fails the accepted command.
	featureID := "42"
	withFeature := FeatureStateSelector{SourceID: "source", FeatureID: &featureID}
	if _, err := m.SetFeatureState(withFeature, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetFeatureState() with empty state error = %v, want ErrInvalidArgument", err)
	}
	future, err := m.SetFeatureState(withFeature, []byte(`[1,2,3]`))
	requireCommandFailedWith(t, nil, future, err, ErrInvalidArgument)
}
