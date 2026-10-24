package maplibre

import (
	"fmt"
	"strings"
	"unsafe"
)

// Conversions that generated code applies to values and parameters without
// touching C declarations, so a generator probe can compile them with its own
// stand-in arena.

func bindingStore[T any](value T, arena *bindingArena) *T {
	pointer := (*T)(arena.allocate(unsafe.Sizeof(value)))
	*pointer = value
	return pointer
}

func bindingStoreOptional[In, Out any](value *In, arena *bindingArena, convert func(In, *bindingArena) Out) *Out {
	if value == nil {
		return nil
	}
	return bindingStore(convert(*value, arena), arena)
}

func bindingArray[In, Out any](items []In, arena *bindingArena, convert func(In, *bindingArena) Out) *Out {
	var zero Out
	pointer := (*Out)(arena.array(len(items), unsafe.Sizeof(zero)))
	converted := unsafe.Slice(pointer, len(items))
	for i, item := range items {
		converted[i] = convert(item, arena)
	}
	return pointer
}

func bindingNumber[In, Out ~int8 | ~int16 | ~int32 | ~int64 | ~int | ~uint8 | ~uint16 | ~uint32 | ~uint64 | ~uint | ~uintptr | ~float32 | ~float64](value In, _ *bindingArena) Out {
	return Out(value)
}

func bindingBool[In, Out ~bool](value In, _ *bindingArena) Out { return Out(value) }

func bindingMasked[In, Out any, Mask ~uint8 | ~uint16 | ~uint32 | ~uint64 | ~int32 | ~int64](mask *Mask, bit Mask, field *Out, value *In, arena *bindingArena, convert func(In, *bindingArena) Out) {
	if value == nil {
		return
	}
	*field = convert(*value, arena)
	*mask |= bit
}

func bindingPresent[T any](present bool, value func() T) *T {
	if !present {
		return nil
	}
	copied := value()
	return &copied
}

type bindingFlag struct {
	bit  uint64
	name string
}

// bindingFlagString joins the names of the flags set in value with "|", and
// appends any bits without a name in hex.
func bindingFlagString(value uint64, flags []bindingFlag) string {
	if value == 0 {
		return "0"
	}
	var names []string
	for _, flag := range flags {
		if value&flag.bit != 0 {
			names = append(names, flag.name)
			value &^= flag.bit
		}
	}
	if value != 0 {
		names = append(names, fmt.Sprintf("%#x", value))
	}
	return strings.Join(names, "|")
}
