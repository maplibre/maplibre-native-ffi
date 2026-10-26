"""Summaries, reference links, and doc comment escaping for generated bindings."""

import ast
import unittest
from xml.etree import ElementTree

from support import parse

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api
from tools.bindgen.docs import Doc
from tools.bindgen.model import ModelError
from tools.bindgen.schema import validate


def verbatim(name):
    """A resolver that spells every C identifier as itself."""
    return name


# A documented callback, its registration, and the global that registers it.
TICK = """
/**
 * Receives a tick.
 *
 * Threads:
 * The callback runs on a worker thread.
 */
typedef void (*mln_tick_callback)(void* context);
typedef void (*mln_tick_release)(void* context);
/** Tick callback state. */
typedef struct mln_tick_handler {
  unsigned int size;
  mln_tick_callback callback;
  void* context BIND("kind=context");
  mln_tick_release release;
} mln_tick_handler BIND("kind=callback_registration;release=release");
/** Installs a tick callback. */
mln_status mln_tick_set_callback(
  const mln_tick_handler* handler, mln_diagnostic* out_diagnostic
);
"""


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


class TranslateTests(unittest.TestCase):
    symbols = {
        "mln_map_cancel_transitions": "Map.cancelTransitions",
        "MLN_GESTURE_PHASE_CANCEL": "GesturePhase.cancel",
        "mln_runtime_event.source_type": "RuntimeEvent.sourceType",
    }.get

    def test_spells_c_identifiers_as_binding_symbols(self):
        self.assertEqual(
            docs.translate(
                "Use `mln_map_cancel_transitions()` or `MLN_GESTURE_PHASE_CANCEL`, "
                "as `mln_runtime_event.source_type` reports; `x[i]` stays.",
                self.symbols,
            ),
            "Use `Map.cancelTransitions()` or `GesturePhase.cancel`, as "
            "`RuntimeEvent.sourceType` reports; `x[i]` stays.",
        )

    def test_drops_a_sentence_that_names_an_unexposed_declaration(self):
        self.assertEqual(
            docs.translate(
                "Cancels transitions. The end reaches `mln_delivery_queue` "
                "instead. Prefer `mln_map_cancel_transitions()`.",
                self.symbols,
            ),
            "Cancels transitions. Prefer `Map.cancelTransitions()`.",
        )

    def test_keeps_sentences_together_across_code_spans(self):
        self.assertEqual(
            docs.sentences("Reads `a. B` first. Then `mln_x.y`. Done."),
            ["Reads `a. B` first.", "Then `mln_x.y`.", "Done."],
        )

    def test_a_sentence_that_starts_with_a_code_span_ends_the_one_before(self):
        self.assertEqual(
            docs.translate(
                "Copies a value. `mln_queue` is internal.", self.symbols, "mln_value"
            ),
            "Copies a value.",
        )

    def test_rejects_a_first_sentence_that_names_an_unexposed_declaration(self):
        with self.assertRaisesRegex(docs.DocError, "mln_thing: .* mln_queue"):
            docs.translate(
                "Copies `mln_queue` state. It runs later.", self.symbols, "mln_thing"
            )

    def test_every_renderer_translates_its_summary(self):
        doc = Doc("mln_x", "Ends with `MLN_GESTURE_PHASE_CANCEL`.")
        for comment in (
            docs.line_comment(doc, self.symbols),
            docs.block_comment(doc, self.symbols),
            docs.xml_comment(doc, self.symbols),
            docs.docstring(doc, self.symbols, ""),
            docs.go_comment(doc, self.symbols, None),
        ):
            self.assertIn("GesturePhase.cancel", comment)
            self.assertNotIn("MLN_GESTURE", comment)


class ThreadsTests(unittest.TestCase):
    def test_a_callback_and_its_registrations_end_with_its_threads(self):
        api = parse(TICK)
        validate(api)
        bound = compile_api(api)
        threads = ("The callback runs on a worker thread.",)
        for name in ("mln_tick_callback", "mln_tick_handler", "mln_tick_set_callback"):
            self.assertEqual(bound.doc(name).threads, threads, name)
        self.assertEqual(
            docs.line_comment(bound.doc("mln_tick_handler"), verbatim),
            "/// Tick callback state. The callback runs on a worker thread.\n"
            "///\n"
            "/// See `mln_tick_handler` in the\n"
            f"/// [C API reference]({docs.REFERENCE}api_8h.html).\n",
        )

    def test_a_binding_that_queues_callbacks_states_its_own_threads(self):
        doc = Doc(
            "mln_tick_handler", "Tick callback state.", threads=("Runs on a worker.",)
        )
        queued = docs.Resolver({}, threads="Runs on the registering thread.")
        self.assertIn(
            "Tick callback state. Runs on the registering thread.",
            docs.line_comment(doc, queued),
        )
        self.assertNotIn("worker", docs.line_comment(doc, queued))

    def test_a_documented_callback_must_state_its_threads(self):
        api = parse(TICK.replace("Threads:", "Notes:"))
        with self.assertRaisesRegex(ModelError, "mln_tick_callback: .*Threads:"):
            validate(api)


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
        comment = docs.xml_comment(doc, verbatim)
        remarks = ElementTree.fromstring(
            "<doc>"
            + " ".join(line.removeprefix("/// ") for line in comment.splitlines())
            + "</doc>"
        ).find("remarks")
        paragraphs = remarks.findall("para")
        self.assertEqual(paragraphs[0].find("c").text, "y")
        self.assertEqual(paragraphs[1].find("see").get("href"), doc.url)

    def test_xml_param_documents_a_positional_property(self):
        param = docs.xml_param(self.doc, verbatim, "Range", "    ")
        element = ElementTree.fromstring(
            "".join(line.strip().removeprefix("/// ") for line in param.splitlines())
        )
        self.assertEqual(element.get("name"), "Range")
        self.assertEqual(element.find("c").text, "mln_x[i]")
        self.assertIn("<x>", element.text)

    def test_docstring_escapes_quotes_and_backslashes(self):
        doc = Doc("mln_quote", 'Holds """ and \\ and "x"')
        statement = docs.docstring(doc, verbatim, "    ")
        function = ast.parse(f"def f():\n{statement}").body[0]
        # The docstring holds the Markdown, which escapes the backslash.
        self.assertEqual(ast.get_docstring(function), 'Holds """ and \\\\ and "x"')

    def test_wraps_within_the_comment_width(self):
        doc = Doc("mln_long", " ".join(["word"] * 60), docs.REFERENCE + "api_8h.html")
        for comment in (
            docs.line_comment(doc, verbatim, "    "),
            docs.block_comment(doc, verbatim, "    "),
            docs.xml_comment(doc, verbatim, "    "),
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
