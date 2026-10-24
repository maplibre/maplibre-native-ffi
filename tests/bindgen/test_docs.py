"""Summaries, reference links, and doc comment escaping for generated bindings."""

import ast
import unittest
from xml.etree import ElementTree

from support import parse

from tools.bindgen import docs
from tools.bindgen.docs import Doc


class SummaryTests(unittest.TestCase):
    def test_takes_the_first_paragraph_and_marks_c_names_as_code(self):
        comment = """/**
 * Queues a style URL command for mln_map_set_style_url(), which
 * replaces mln_map_options.style.
 *
 * The function copies url before returning acceptance.
 */"""
        self.assertEqual(
            docs.summary(comment),
            "Queues a style URL command for `mln_map_set_style_url()`, which "
            "replaces `mln_map_options.style`.",
        )

    def test_stops_at_a_status_list(self):
        comment = """/**
 * Releases a map.
 * Returns:
 * - MLN_STATUS_OK on success.
 */"""
        self.assertEqual(docs.summary(comment), "Releases a map.")

    def test_keeps_existing_code_spans_and_array_extents(self):
        self.assertEqual(
            docs.summary("/** Borrows mln_lat_lng[count]; `MLN_X` stays. */"),
            "Borrows `mln_lat_lng[count]`; `MLN_X` stays.",
        )

    def test_reads_line_comments_and_skips_a_file_command(self):
        self.assertEqual(
            docs.summary("/// @file api.h\n/// Copies a value.\n///\n/// More."),
            "Copies a value.",
        )


class ReferenceTests(unittest.TestCase):
    def test_rejects_a_header_without_a_page_name(self):
        with self.assertRaises(ValueError):
            docs.reference_url("maplibre_native_c/Upper.h")


class RenderTests(unittest.TestCase):
    doc = Doc(
        "mln_range",
        "Accepts [0, 1] and <x> where a */ b; keeps `mln_x[i]` and & \\.",
        "https://example.invalid/api_8h.html",  # lint: not-fetched
    )

    def test_xml_comment_puts_notes_before_the_reference_in_remarks(self):
        url = "https://example.invalid/x_8h.html"  # lint: not-fetched
        doc = Doc("mln_x", "Does x.", url, ("Uses `y`.",))
        comment = docs.xml_comment(doc)
        remarks = ElementTree.fromstring(
            "<doc>"
            + " ".join(line.removeprefix("/// ") for line in comment.splitlines())
            + "</doc>"
        ).find("remarks")
        paragraphs = remarks.findall("para")
        self.assertEqual(paragraphs[0].find("c").text, "y")
        self.assertEqual(paragraphs[1].find("see").get("href"), doc.url)

    def test_xml_param_documents_a_positional_property(self):
        param = docs.xml_param(self.doc, "Range", "    ")
        element = ElementTree.fromstring(
            "".join(line.strip().removeprefix("/// ") for line in param.splitlines())
        )
        self.assertEqual(element.get("name"), "Range")
        self.assertEqual(element.find("c").text, "mln_x[i]")
        self.assertIn("<x>", element.text)

    def test_docstring_escapes_quotes_and_backslashes(self):
        doc = Doc("mln_quote", 'Holds """ and \\ and "x"')
        statement = docs.docstring(doc, "    ")
        function = ast.parse(f"def f():\n{statement}").body[0]
        # The docstring holds the Markdown, which escapes the backslash.
        self.assertEqual(ast.get_docstring(function), 'Holds """ and \\\\ and "x"')

    def test_wraps_within_the_comment_width(self):
        doc = Doc("mln_long", " ".join(["word"] * 60), docs.REFERENCE + "api_8h.html")
        for comment in (
            docs.line_comment(doc, "    "),
            docs.block_comment(doc, "    "),
            docs.xml_comment(doc, "    "),
        ):
            for line in comment.splitlines():
                if "https://" not in line:
                    self.assertLessEqual(len(line), docs.WIDTH, line)


class IndexTests(unittest.TestCase):
    def test_attaches_comments_across_annotations_only_to_their_declaration(self):
        api = parse("""
/** Modes of a thing. */
typedef enum mln_mode : unsigned {
  /** The first mode. */
  MLN_MODE_FIRST = 1,
  MLN_MODE_SECOND = 2,
} mln_mode;

/** A pair of values. */
typedef struct mln_pair {
  /** The left value. */
  int left;
  int right;
} mln_pair;

/// A count of pairs.
///
/// Counts are never negative.
typedef unsigned mln_count;

/**
 * Reads a pair.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 */
BIND("execution=query;"
     "result=double")
mln_status mln_pair_read(
  const mln_completion* completion, mln_diagnostic* out_diagnostic
);
""")
        index = docs.index(api)
        self.assertEqual(index["mln_pair_read"].summary, "Reads a pair.")
        self.assertEqual(index["mln_pair_read"].url, docs.REFERENCE + "api_8h.html")
        self.assertEqual(index["mln_mode"].summary, "Modes of a thing.")
        self.assertEqual(
            index["MLN_MODE_FIRST"], Doc("MLN_MODE_FIRST", "The first mode.")
        )
        self.assertNotIn("MLN_MODE_SECOND", index)
        self.assertEqual(index["mln_pair"].summary, "A pair of values.")
        self.assertEqual(index["mln_pair.left"].summary, "The left value.")
        self.assertNotIn("mln_pair.right", index)
        self.assertEqual(index["mln_count"].summary, "A count of pairs.")


if __name__ == "__main__":
    unittest.main()
