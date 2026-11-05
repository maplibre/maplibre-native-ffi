"""Test the GObject library's generated enums and error domain."""

import unittest

from support import emitted, parse, real_api

from tools.bindgen.emitters import gobject
from tools.bindgen.model import Unsupported
from tools.bindgen.schema import validate

ENUMS = "src/maplibre-native-ffi-enums.h"


class GObjectEmitterTests(unittest.TestCase):
    def test_the_error_domain_codes_are_the_failing_statuses(self):
        header = emitted(gobject, real_api())[ENUMS]
        self.assertIn("  MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT = -1,\n", header)
        self.assertIn("  MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND = -10,\n", header)
        # A GError never carries success.
        self.assertNotIn("MAPLIBRE_NATIVE_FFI_ERROR_OK", header)

    def test_a_bitmask_is_flags_until_a_bit_leaves_their_32_bits(self):
        source = emitted(gobject, real_api())["src/maplibre-native-ffi-enums.c"]
        self.assertIn(
            "G_DEFINE_FLAGS_TYPE(\n"
            "  MaplibreNativeFfiRenderBackendFlag, maplibre_native_ffi_render_backend_flag,\n",
            source,
        )
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
        with self.assertRaisesRegex(Unsupported, "MLN_WIDE_FLAG_HIGH"):
            gobject.lower(api)


if __name__ == "__main__":
    unittest.main()
