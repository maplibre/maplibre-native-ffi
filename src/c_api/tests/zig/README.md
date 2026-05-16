# Raw Zig C ABI tests

These tests exercise `maplibre_native_c.h` directly through Zig `@cImport`. They
stay below the public Zig binding so they can cover raw ABI behavior that the
binding hides on purpose.

Keep tests here only when they need unsafe C ABI shapes that the binding cannot
construct: null input or output pointers, undersized structs, unknown raw enum
or flag values, non-null output handles, and stale raw handles. Move semantic
behavior to `bindings/zig/tests` whenever the safe-ish public Zig API can
express it.
