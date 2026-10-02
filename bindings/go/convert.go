package maplibre

import "unsafe"

// Conversions that generated code applies to values and parameters without
// touching C declarations, so a generator probe can compile them with its own
// stand-in arena.

// bindingNullableBytes borrows optional text or bytes by address, keeping
// present empty text distinct from absent text with a non-null address.
func bindingNullableBytes[T ~string | ~[]byte](value *T, arena *bindingArena) unsafe.Pointer {
	if value == nil {
		return nil
	}
	pointer := arena.bytes([]byte(*value))
	if pointer == nil {
		pointer = arena.allocate(1)
	}
	return pointer
}

// bindingOptionalBytes borrows optional text or bytes by address, which native
// reads as absent when empty.
func bindingOptionalBytes[T ~string | ~[]byte](value *T, arena *bindingArena) unsafe.Pointer {
	if value == nil {
		return nil
	}
	return arena.bytes([]byte(*value))
}

// bindingLen returns the length of optional text or bytes, or zero.
func bindingLen[T ~string | ~[]byte](value *T) int {
	if value == nil {
		return 0
	}
	return len(*value)
}

// bindingStore copies a native value into the arena and returns its address.
func bindingStore[T any](value T, arena *bindingArena) *T {
	pointer := (*T)(arena.allocate(unsafe.Sizeof(value)))
	*pointer = value
	return pointer
}

// bindingStoreOptional converts and stores an optional value, or returns nil.
func bindingStoreOptional[In, Out any](value *In, arena *bindingArena, convert func(In, *bindingArena) Out) *Out {
	if value == nil {
		return nil
	}
	return bindingStore(convert(*value, arena), arena)
}

// bindingArray converts items into a native array in the arena. An empty
// array has a null address.
func bindingArray[In, Out any](items []In, arena *bindingArena, convert func(In, *bindingArena) Out) *Out {
	var zero Out
	pointer := (*Out)(arena.array(len(items), unsafe.Sizeof(zero)))
	converted := unsafe.Slice(pointer, len(items))
	for i, item := range items {
		converted[i] = convert(item, arena)
	}
	return pointer
}

// bindingNullableArray converts items like bindingArray, keeping a present
// empty array distinct from an absent one with a non-null address.
func bindingNullableArray[In, Out any](items []In, arena *bindingArena, convert func(In, *bindingArena) Out) *Out {
	pointer := bindingArray(items, arena, convert)
	if items != nil && pointer == nil {
		pointer = (*Out)(arena.allocate(1))
	}
	return pointer
}

// bindingNumber and bindingBool convert a binding scalar to its native type,
// in the converter shape that the array, store, and field helpers take.
func bindingNumber[In, Out ~int8 | ~int16 | ~int32 | ~int64 | ~int | ~uint8 | ~uint16 | ~uint32 | ~uint64 | ~uint | ~uintptr | ~float32 | ~float64](value In, _ *bindingArena) Out {
	return Out(value)
}

func bindingBool[In, Out ~bool](value In, _ *bindingArena) Out { return Out(value) }

// bindingMasked writes a present optional value into a presence-masked native
// field and marks bit in mask. An absent value leaves both unchanged.
func bindingMasked[In, Out any, Mask ~uint8 | ~uint16 | ~uint32 | ~uint64 | ~int32 | ~int64](mask *Mask, bit Mask, field *Out, value *In, arena *bindingArena, convert func(In, *bindingArena) Out) {
	if value == nil {
		return
	}
	*field = convert(*value, arena)
	*mask |= bit
}

// bindingFlagged writes a present optional value into a native field whose
// presence is a bool flag. An absent value leaves both unchanged.
func bindingFlagged[In, Out any, Flag ~bool](flag *Flag, field *Out, value *In, arena *bindingArena, convert func(In, *bindingArena) Out) {
	if value == nil {
		return
	}
	*field = convert(*value, arena)
	*flag = true
}

// bindingPresent copies an optional native field: value runs only when the
// field is present, and an absent field is nil.
func bindingPresent[T any](present bool, value func() T) *T {
	if !present {
		return nil
	}
	copied := value()
	return &copied
}
