# Raw C API tests

These Unity tests exercise `maplibre_native_c.h` directly from C. They stay
below the language bindings so they can cover raw ABI behavior that bindings
hide on purpose.

Tests belong here when they require unsafe C API shapes that a binding cannot
construct: null input or output pointers, undersized structs, unknown raw enum
or flag values, preinitialized output handles, and stale raw handles. Semantic
behavior belongs in each applicable binding's test suite whenever its public API
can express the scenario.
