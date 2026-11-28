"""Check the Go binding plan."""

import unittest

from support import emitted, parse, real_api

from tools.bindgen.emitters import go


class GoEmitterTests(unittest.TestCase):
    def test_dispose_is_public_only_where_c_has_one_besides_a_void_release(self):
        source = emitted(go, real_api())["generated_api.go"]
        disposes = {
            line.split("*", 1)[1].split(")", 1)[0]
            for line in source.splitlines()
            if line.startswith("func (receiver *") and ") Dispose() error" in line
        }
        self.assertEqual(
            disposes, {"MapHandle", "RenderSessionHandle", "RuntimeHandle"}
        )

    def test_operations_cannot_take_owner_member_names(self):
        api = parse("""
typedef unsigned long long mln_host BIND("kind=handle;release=mln_host_destroy");
void mln_host_destroy(mln_host host);
mln_status mln_host_is_closed(mln_host host, bool *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        result = go.lower(api)
        self.assertEqual(result.generated, ["mln_host_destroy"])
        self.assertIn(
            "reserved for owner state", result.unsupported["mln_host_is_closed"]
        )


if __name__ == "__main__":
    unittest.main()
