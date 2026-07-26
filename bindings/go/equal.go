package maplibre

// Structural comparison helpers for the option structs. Optional fields are pointers, so the
// built-in == operator compares pointer identity rather than the values behind them; the Equal
// methods built on these helpers compare the values.

// equalPointer reports whether two optional fields hold equal values. A nil pointer marks an
// absent field and is equal only to another nil pointer.
func equalPointer[T comparable](left, right *T) bool {
	if left == nil || right == nil {
		return left == nil && right == nil
	}
	return *left == *right
}

// equalStrings reports whether two optional string lists are equal. A nil list marks an absent
// field and is never equal to an empty list, matching how the field masks are populated.
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

// equalJSON reports whether two optional JSON filters are equal.
func equalJSON(left, right *JSONValue) bool {
	if left == nil || right == nil {
		return left == nil && right == nil
	}
	return left.Equal(*right)
}
