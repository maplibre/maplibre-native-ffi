package maplibre

import "bytes"

// Structural comparison helpers for the option structs, whose optional fields
// are pointers.

// equalPointer reports whether two optional fields hold equal values. A nil
// pointer marks an absent field and is equal only to another nil pointer.
func equalPointer[T comparable](left, right *T) bool {
	if left == nil || right == nil {
		return left == nil && right == nil
	}
	return *left == *right
}

func clonePointer[T any](value *T) *T {
	if value == nil {
		return nil
	}
	cloned := new(T)
	*cloned = *value
	return cloned
}

func cloneBytes(value []byte) []byte {
	if value == nil {
		return nil
	}
	return append([]byte{}, value...)
}

// equalStrings reports whether two optional string lists are equal. A nil list
// marks an absent field and is never equal to an empty list.
func equalStrings(left, right []string) bool {
	if (left == nil) != (right == nil) || len(left) != len(right) {
		return false
	}
	for index := range left {
		if left[index] != right[index] {
			return false
		}
	}
	return true
}

// equalBoundsConstraint reports whether two optional camera center constraints
// are equal. Bounds are compared only for the bounded case, which reads them.
func equalBoundsConstraint(left, right *BoundsConstraint) bool {
	if left == nil || right == nil {
		return left == nil && right == nil
	}
	if left.Kind != right.Kind {
		return false
	}
	return left.Kind != BoundsConstraintBounded || left.Bounds == right.Bounds
}

// equalOptionalBytes reports whether two optional byte fields are equal. A nil
// slice marks an absent field and is never equal to an empty one.
func equalOptionalBytes(left []byte, right []byte) bool {
	return (left == nil) == (right == nil) && bytes.Equal(left, right)
}

// equalStretches compares stretch slices by content, keeping a present empty
// slice distinct from an absent one.
func equalStretches(left, right []ImageStretch) bool {
	if (left == nil) != (right == nil) || len(left) != len(right) {
		return false
	}
	for index := range left {
		if left[index] != right[index] {
			return false
		}
	}
	return true
}

// cloneStretches copies a stretch slice, keeping a present empty slice distinct
// from an absent one.
func cloneStretches(stretches []ImageStretch) []ImageStretch {
	if stretches == nil {
		return nil
	}
	cloned := make([]ImageStretch, len(stretches))
	copy(cloned, stretches)
	return cloned
}
