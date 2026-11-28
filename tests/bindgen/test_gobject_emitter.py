"""Test the GObject library's generated enums."""

import unittest

from support import emitted, parse

from tools.bindgen.emitters import gobject
from tools.bindgen.schema import validate

ENUMS = "src/maplibre-native-ffi-enums.h"


class GObjectEmitterTests(unittest.TestCase):
    def test_a_bitmask_with_a_bit_past_32_is_an_integer_with_constants(self):
        api = parse(
            """
typedef enum BIND("kind=bitmask") mln_wide_flag : uint64_t {
  MLN_WIDE_FLAG_HIGH = 1ULL << 40,
} mln_wide_flag;
typedef uint64_t mln_wide_flags;
mln_status mln_widen(mln_wide_flags flags, mln_diagnostic *out_diagnostic);
""",
            defines=("MLN_PROTOCOL_STANDARD_TYPES",),
        )
        validate(api)
        lowered = emitted(gobject, api)
        # A wider bitmask is an integer type with a constant for each bit.
        self.assertIn("typedef guint64 MaplibreNativeFfiWideFlag;\n", lowered[ENUMS])
        self.assertIn(
            "#define MAPLIBRE_NATIVE_FFI_WIDE_FLAG_HIGH"
            " ((MaplibreNativeFfiWideFlag)G_GUINT64_CONSTANT(1099511627776))\n",
            lowered[ENUMS],
        )
        self.assertNotIn(
            "G_DEFINE_FLAGS_TYPE", lowered["src/maplibre-native-ffi-enums.c"]
        )


if __name__ == "__main__":
    unittest.main()
