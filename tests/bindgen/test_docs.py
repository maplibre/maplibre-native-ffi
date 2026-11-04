"""Summaries, reference links, and doc comment escaping for generated bindings."""

import ast
import unittest
from xml.etree import ElementTree

from support import parse, real_api

from tools.bindgen import async_hosts, docs
from tools.bindgen.compiler import compile_api
from tools.bindgen.docs import Doc
from tools.bindgen.emitters import dart, dotnet, go, kotlin, swift, typescript
from tools.bindgen.names import camel
from tools.bindgen.schema import validate


def verbatim(name):
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

# After TICK, a registration whose synchronous callback only native code can
# supply, and one whose callback an async-only host leaves to native rules.
HOST_FORMS = """
/**
 * Locks a queue.
 *
 * Threads:
 * It runs on a library thread.
 */
BIND("synchronous=true")
typedef void (*mln_lock_callback)(void* context);
/** A queue lock. */
typedef struct mln_lock {
  mln_lock_callback lock BIND("nullable=true");
  void* context BIND("kind=context");
  mln_tick_release release;
} mln_lock BIND("kind=callback_registration;release=release");
/** Takes a queue lock. */
mln_status mln_set_lock(const mln_lock* lock, mln_diagnostic* out_diagnostic);
/**
 * Picks a header.
 *
 * Threads:
 * Runs on a library thread.
 */
typedef bool (*mln_header_callback)(void* context) BIND("failure=false");
/**
 * Headers to add.
 *
 * Set either `callback` or `rules`, not both.
 */
typedef struct mln_header_transform {
  unsigned int size;
  mln_header_callback callback BIND("nullable=true");
  void* context BIND("kind=context");
  mln_tick_release release;
  /** The rules that add headers. */
  const mln_buffer_view* rules BIND("length=rule_count");
  unsigned long rule_count;
} mln_header_transform BIND("kind=callback_registration;release=release");
/** Adds headers. */
mln_status mln_set_header_transform(
  const mln_header_transform* transform, mln_diagnostic* out_diagnostic
);
"""

# A command whose comment has every part that bindings copy, and one that they
# leave in C.
REMOVE = """
typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_release");
void mln_map_release(mln_map map);
/**
 * Removes a source from the map's style.
 *
 * `source_id` names the source, and `completion` reports the outcome. The
 * removal waits for the frame.
 *
 * - `source_id` may be
 *   long.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when `source_id` is empty.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the style has no source named
 *   `source_id`.
 * - MLN_STATUS_INVALID_ARGUMENT when `source_id` is empty.
 * - MLN_STATUS_INVALID_STATE when `completion` is reused.
 *
 * Lifetimes:
 * `source_id` stays valid until the call returns.
 */
BIND("execution=command")
mln_status mln_map_remove_source(
  mln_map map, unsigned source_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
);
"""


def remove_doc() -> Doc:
    api = parse(REMOVE, defines=("MLN_PROTOCOL_COMPLETION_RUNTIME",))
    validate(api)
    doc = compile_api(api).doc("mln_map_remove_source")
    assert doc is not None
    return doc


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
        "mln_map_cancel_camera_transitions": "Map.cancelCameraTransitions",
        "MLN_GESTURE_PHASE_CANCEL": "GesturePhase.cancel",
        "mln_runtime_event.source_type": "RuntimeEvent.sourceType",
    }.get

    def test_spells_c_identifiers_as_binding_symbols(self):
        self.assertEqual(
            docs.translate(
                "Use `mln_map_cancel_camera_transitions()` or `MLN_GESTURE_PHASE_CANCEL`, "
                "as `mln_runtime_event.source_type` reports; `x[i]` stays.",
                self.symbols,
            ),
            "Use `Map.cancelCameraTransitions()` or `GesturePhase.cancel`, as "
            "`RuntimeEvent.sourceType` reports; `x[i]` stays.",
        )

    def test_drops_a_sentence_that_names_an_unexposed_declaration(self):
        self.assertEqual(
            docs.translate(
                "Cancels transitions. The end reaches `mln_delivery_queue` "
                "instead. Prefer `mln_map_cancel_camera_transitions()`.",
                self.symbols,
            ),
            "Cancels transitions. Prefer `Map.cancelCameraTransitions()`.",
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


class LiteralKeyTests(unittest.TestCase):
    def test_a_geojson_key_named_like_a_parameter_keeps_its_spelling(self):
        # Each binding renames a code span that names a parameter, so the
        # cluster queries quote the "cluster_id" key instead.
        api = real_api()
        for emitter in (dart, dotnet, go, kotlin, swift, typescript):
            with self.subTest(emitter=emitter.__name__):
                text = "".join(emitter.lower(api).files.values())
                words = [w for w in text.split() if w not in {"///", "//", "*"}]
                self.assertIn('the "cluster_id" property', " ".join(words))


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
            "/// Tick callback state.\n"
            "///\n"
            "/// The callback runs on a worker thread.\n"
            "///\n"
            "/// See `mln_tick_handler` in the\n"
            f"/// [C API reference]({docs.REFERENCE}api_8h.html).\n",
        )

    def test_a_binding_that_queues_callbacks_states_its_own_threads(self):
        doc = Doc(
            "mln_tick_handler", "Tick callback state.", threads=("Runs on a worker.",)
        )
        queued = docs.Resolver(
            {}, threads={"mln_tick_handler": "Runs on the registering thread."}
        )
        self.assertIn(
            "/// Tick callback state.\n///\n/// Runs on the registering thread.\n",
            docs.line_comment(doc, queued),
        )
        self.assertNotIn("worker", docs.line_comment(doc, queued))

    def test_an_async_host_states_its_threads_only_where_it_runs_a_callback(self):
        # The host runs the wake callback. Native code runs the lock's
        # synchronous callback, so the lock keeps the C threads. Native rules
        # answer for the header callback that the host leaves out, so the
        # header transform names neither that callback nor a thread.
        api = parse(TICK + HOST_FORMS)
        validate(api)
        bound = compile_api(api)
        symbols = docs.Resolver(
            {},
            threads=async_hosts.threads(bound, "Runs on the host's thread."),
            fields=async_hosts.omitted_fields(bound),
        )

        def text(name):
            return docs.line_comment(bound.doc(name), symbols)

        self.assertIn("Runs on the host's thread.", text("mln_tick_handler"))
        self.assertIn("Runs on the host's thread.", text("mln_tick_set_callback"))
        self.assertIn("It runs on a library thread.", text("mln_lock"))
        self.assertNotIn("host", text("mln_lock"))
        for name in ("mln_header_transform", "mln_set_header_transform"):
            self.assertNotIn("thread", text(name), name)
            self.assertNotIn("callback", text(name), name)
        self.assertIn("rules", text("mln_header_transform.rules"))


class BodyTests(unittest.TestCase):
    def test_reads_paragraphs_lists_and_error_items_but_not_lifetimes(self):
        doc = remove_doc()
        self.assertEqual(
            doc.paragraphs,
            (
                (
                    "`source_id` names the source, and `completion` reports the "
                    "outcome. The removal waits for the frame."
                ),
                ("`source_id` may be long.",),
            ),
        )
        # The item for success is not an error.
        self.assertEqual(
            doc.returns, ("`MLN_STATUS_INVALID_ARGUMENT` when `source_id` is empty.",)
        )
        self.assertEqual(
            doc.completes[0],
            "`MLN_STATUS_NOT_FOUND` when the style has no source named `source_id`.",
        )
        self.assertNotIn("stays valid", repr(doc))

    def test_hides_the_parameters_that_a_binding_takes_over(self):
        doc = remove_doc()
        self.assertEqual(doc.parameters, ("map", "source_id"))
        self.assertEqual(set(doc.hidden), {"completion", "out_diagnostic"})
        api = parse(TICK)
        validate(api)
        callback = compile_api(api).doc("mln_tick_callback")
        assert callback is not None
        self.assertEqual(callback.hidden, ("context",))


class ErrorsTests(unittest.TestCase):
    symbols = docs.Resolver(
        {
            "MLN_STATUS_INVALID_ARGUMENT": "Status.InvalidArgument",
            "MLN_STATUS_NOT_FOUND": "Status.NotFound",
            "MLN_STATUS_INVALID_STATE": "Status.InvalidState",
        },
        parameter=camel,
    )

    def test_copies_the_comment_in_the_binding_s_terms(self):
        # The sentence and the error that name the completion leave, as does
        # the lifetime, and a repeated error appears once.
        self.assertEqual(
            docs.line_comment(remove_doc(), self.symbols, errors="# Errors"),
            "/// Removes a source from the map's style.\n"
            "///\n"
            "/// The removal waits for the frame.\n"
            "///\n"
            "/// - `sourceId` may be long.\n"
            "///\n"
            "/// See `mln_map_remove_source` in the\n"
            f"/// [C API reference]({docs.REFERENCE}api_8h.html).\n"
            "///\n"
            "/// # Errors\n"
            "///\n"
            "/// - `Status.InvalidArgument` when `sourceId` is empty.\n"
            "/// - `Status.NotFound` when the style has no source named `sourceId`.\n",
        )

    def test_a_form_that_discards_its_completion_omits_its_errors(self):
        comment = docs.line_comment(docs.discarding(remove_doc()), self.symbols)
        self.assertIn("/// Errors:\n///\n/// - `Status.InvalidArgument`", comment)
        self.assertNotIn("NotFound", comment)

    def test_xml_lists_errors_in_the_remarks(self):
        comment = docs.xml_comment(remove_doc(), self.symbols)
        remarks = ElementTree.fromstring(
            "<doc>"
            + " ".join(line.removeprefix("/// ") for line in comment.splitlines())
            + "</doc>"
        ).find("remarks")
        assert remarks is not None
        self.assertEqual(remarks.findall("para")[-1].text, "Errors:")
        self.assertEqual(
            [
                item.findtext("description/c")
                for item in remarks.findall("list")[-1].findall("item")
            ],
            ["Status.InvalidArgument", "Status.NotFound"],
        )

    def test_go_lists_errors_as_plain_text(self):
        comment = docs.go_comment(remove_doc(), self.symbols, "RemoveSource")
        self.assertIn(
            "// Errors:\n"
            "//   - Status.InvalidArgument when sourceId is empty.\n"
            "//   - Status.NotFound when the style has no source named sourceId.\n",
            comment,
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
        comment = docs.xml_comment(doc, verbatim)
        remarks = ElementTree.fromstring(
            "<doc>"
            + " ".join(line.removeprefix("/// ") for line in comment.splitlines())
            + "</doc>"
        ).find("remarks")
        paragraphs = remarks.findall("para")
        self.assertEqual(paragraphs[0].find("c").text, "y")
        self.assertEqual(paragraphs[1].find("see").get("href"), doc.url)

    def test_block_comment_neither_closes_nor_nests_a_comment(self):
        doc = Doc("mln_glob", "Matches `*.tiles/**` and `a*/b`.")
        body = (
            docs.block_comment(doc, verbatim).removeprefix("/**").removesuffix("*/\n")
        )
        self.assertNotIn("/*", body)
        self.assertNotIn("*/", body)

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
