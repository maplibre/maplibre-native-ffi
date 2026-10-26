"""Documentation that the generated bindings carry from the C headers.

A generated declaration carries the first paragraph of its C comment and a link
to its header's page in the C API reference. The C header remains the full
contract: status lists, output parameters, and ownership rules name C concepts
that each binding expresses differently, so a binding repeats only the summary.
A callback, and a registration of one, also carries the paragraph under its C
comment's `Threads:` heading, so a host knows where its code runs.

A summary keeps C identifiers verbatim and marks them as code. Each language
renders the summary through one function here, which takes the binding's
symbol resolver. The resolver spells each C identifier as that binding's public
symbol, and a sentence that names a declaration the binding does not expose,
such as a queue member or an output parameter, leaves the binding's summary.
The rendering then applies that language's escaping to the prose and leaves
code spans intact.
"""

from __future__ import annotations

import html
import re
import textwrap
from collections.abc import Callable
from dataclasses import dataclass, replace
from typing import TYPE_CHECKING

from .model import Api

if TYPE_CHECKING:
    from .semantic import BoundApi, FieldPlan, MaskFlag, OperationPlan, ValuePlan

# A binding's public spelling of a C declaration, or None when the binding
# does not expose it. The argument is a C identifier without a call's `()`: a
# type, function, or enum constant, or a `record.member` path.
Symbol = Callable[[str], "str | None"]

REFERENCE = "https://maplibre.org/maplibre-native-ffi/reference/c/"

WIDTH = 80

_SECTION = re.compile(r"^[A-Z][A-Za-z ]*:$")
_IDENTIFIER = re.compile(
    r"\b(?:mln|MLN)_\w+(?:\.[A-Za-z_]\w*)*(?:\[\w*\])?(?:\(\))?",
)
# A C identifier code span: its name, then an array extent or a call's `()`,
# which the binding's spelling keeps.
_REFERENCE = re.compile(r"((?:mln|MLN)_\w+(?:\.[A-Za-z_]\w*)*)(\[\w*\]|\(\))?")
# A sentence ends at a period, question mark, or exclamation mark that
# whitespace and a capital, digit, or code span follow.
_SENTENCE_END = re.compile(r"(?<=[.!?])\s+(?=[A-Z0-9`])")
# A boundary at the end of prose that a code span follows, where the span's
# opening backtick is the next character but sits in the next part.
_SENTENCE_END_BEFORE_CODE = re.compile(r"(?<=[.!?])\s+(?=[A-Z0-9`]|$)")


def _comment_lines(raw: str) -> list[str]:
    """The text lines of a C comment, without its markers."""
    text = raw.strip()
    if text.startswith("/*"):
        text = re.sub(r"^/\*[*!]?<?", "", text)
        text = re.sub(r"\*/$", "", text)
        lines = [re.sub(r"^\s*\*(?!/) ?", "", line) for line in text.splitlines()]
    else:
        lines = [re.sub(r"^\s*//[/!]?<? ?", "", line) for line in text.splitlines()]
    return [line.rstrip() for line in lines]


def _mark_code(text: str) -> str:
    """Mark the C API's identifiers outside existing code spans as code."""
    parts = text.split("`")
    for index in range(0, len(parts), 2):
        parts[index] = _IDENTIFIER.sub(lambda match: f"`{match[0]}`", parts[index])
    return "`".join(parts)


def _paragraph(lines: list[str]) -> str:
    """The paragraph that begins `lines`, as one line of Markdown.

    The paragraph ends at a blank line, a list item, or a section heading such
    as `Returns:`. The C API's identifiers become code spans; the remaining
    prose stays as written.
    """
    paragraph: list[str] = []
    for line in lines:
        stripped = line.strip()
        if not paragraph and not stripped:
            continue
        if not stripped or _SECTION.match(stripped) or stripped.startswith("- "):
            break
        paragraph.append(stripped)
    return _mark_code(" ".join(" ".join(paragraph).split()))


def summary(raw_comment: str) -> str:
    """The first paragraph of a C comment, as one line of Markdown.

    A Doxygen `@file` command line before it is skipped.
    """
    lines = _comment_lines(raw_comment)
    while lines and (
        not lines[0].strip() or lines[0].strip().startswith(("@file", "\\file"))
    ):
        lines.pop(0)
    return _paragraph(lines)


def section(raw_comment: str, heading: str) -> str:
    """The paragraph under a C comment's `heading:` line, or empty."""
    lines = _comment_lines(raw_comment)
    for index, line in enumerate(lines):
        if line.strip() == f"{heading}:":
            return _paragraph(lines[index + 1 :])
    return ""


def reference_url(header: str) -> str:
    """The C API reference page for a header path under `include/`.

    Doxygen names a file page after the file's base name, doubling each
    underscore and writing the dot as `_8`.
    """
    name = header.rsplit("/", 1)[-1]
    if not re.fullmatch(r"[a-z0-9_]+\.h", name):
        raise ValueError(f"{header}: no Doxygen page name for this header")
    return REFERENCE + name.replace("_", "__").replace(".", "_8") + ".html"


@dataclass(frozen=True)
class Doc:
    """The documentation that a binding declaration carries."""

    symbol: str
    summary: str
    url: str = ""
    # Paragraphs that a binding adds after the summary, about its own behavior.
    notes: tuple[str, ...] = ()
    # The `Threads:` paragraphs of a callback, or of the callbacks that a
    # registration takes, which the summary ends with.
    threads: tuple[str, ...] = ()


def index(api: Api) -> dict[str, Doc]:
    """The documentation of each documented public declaration.

    Keys are C names; a record field's key is `record.field`. A declaration
    without a comment is absent, because the reference lists only documented
    declarations. A typedef without its own comment takes the comment of the
    record or enum that it names.
    """
    internal = set(api.runtime_exports) | set(api.runtime_types)
    docs: dict[str, Doc] = {}

    def add(name: str, comment: str, header: str | None) -> None:
        text = summary(comment)
        if text and name not in internal:
            threads = section(comment, "Threads")
            docs[name] = Doc(
                name,
                text,
                reference_url(header) if header else "",
                threads=(threads,) if threads else (),
            )

    for function in api.functions:
        add(function.name, function.documentation, function.location.path)
    for record in api.records:
        add(record.name, record.documentation, record.location.path)
        for field in record.fields:
            if field.name and record.name not in internal:
                add(f"{record.name}.{field.name}", field.documentation, None)
    for enum in api.enums:
        add(enum.name, enum.documentation, enum.location.path)
        for value in enum.values:
            if enum.name not in internal:
                add(value.name, value.documentation, None)
    for typedef in api.typedefs:
        if typedef.name not in docs:
            add(typedef.name, typedef.documentation, typedef.location.path)
    return docs


def with_threads(doc: Doc, threads) -> Doc:
    """`doc` ending with each of `threads` that it does not already carry."""
    added = tuple(dict.fromkeys(t for t in threads if t and t not in doc.threads))
    return replace(doc, threads=doc.threads + added) if added else doc


def spans(markdown: str) -> list[tuple[str, bool]]:
    """Split Markdown into (text, is_code) runs at its code spans."""
    return [
        (part, index % 2 == 1) for index, part in enumerate(markdown.split("`")) if part
    ]


def sentences(markdown: str) -> list[str]:
    """Split Markdown into its sentences, never inside a code span."""
    found, current = [], ""
    parts = markdown.split("`")
    for index, part in enumerate(parts):
        if index % 2:
            current += f"`{part}`"
            continue
        boundary = (
            _SENTENCE_END if index == len(parts) - 1 else _SENTENCE_END_BEFORE_CODE
        )
        first, *rest = boundary.split(part)
        current += first
        for piece in rest:
            found.append(current.strip())
            current = piece
    found.append(current.strip())
    return [sentence for sentence in found if sentence]


class DocError(ValueError):
    """A summary whose first sentence names what a binding does not expose."""


def _rewrite(sentence: str, symbol: Symbol) -> tuple[str, str | None]:
    """The sentence with each C identifier spelled as `symbol` resolves it, and
    the first identifier that it cannot resolve, or None."""
    parts = sentence.split("`")
    for index in range(1, len(parts), 2):
        match = _REFERENCE.fullmatch(parts[index])
        if match is None:
            continue
        resolved = symbol(match[1])
        if resolved is None:
            return sentence, match[1]
        parts[index] = resolved + (match[2] or "")
    return "`".join(parts), None


def translate(markdown: str, symbol: Symbol, name: str = "") -> str:
    """A summary in a binding's terms.

    Each C identifier becomes the symbol that the binding exposes. A sentence
    that names a declaration the binding does not expose describes something
    the binding hides, such as a queue, a callback context, or an output
    parameter, and leaves the summary. The first sentence states what the
    declaration is, so it must not name one; `name` identifies the summary in
    the error.
    """
    kept = []
    for index, sentence in enumerate(sentences(markdown)):
        rewritten, missing = _rewrite(sentence, symbol)
        if missing is None:
            kept.append(rewritten)
        elif index == 0:
            raise DocError(
                f"{name}: the first sentence names {missing}, which this binding "
                "does not expose; reword the C comment's first sentence"
            )
    return " ".join(kept)


def _paragraphs(doc: Doc, symbol: Symbol) -> tuple[str, list[str]]:
    """The translated summary, which ends with the threads, and the notes."""
    text = " ".join(
        part
        for part in (
            translate(doc.summary, symbol, doc.symbol),
            *(
                (getattr(symbol, "threads", None),)
                if doc.threads and getattr(symbol, "threads", None)
                else (translate(threads, symbol, doc.symbol) for threads in doc.threads)
            ),
        )
        if part
    )
    return text, [translate(note, symbol, doc.symbol) for note in doc.notes]


def resolver(
    bound: BoundApi,
    *,
    type: Callable[[str], str | None],
    operation: Callable[[OperationPlan], str | None],
    constant: Callable[[ValuePlan, str], str | None],
    field: Callable[[ValuePlan, FieldPlan], str | None],
    flag: Callable[[ValuePlan, MaskFlag], str | None] | None = None,
    extra: dict[str, str] | None = None,
    threads: str | None = None,
) -> Symbol:
    """A binding's symbol resolver over the declarations that a host can name.

    Those are the public values, handles, and callbacks, each public
    operation, the constants of each public enum, the public fields of each
    public record, and the flags of a record's mask. Each function spells a
    declaration as the binding generates it, or returns None for one that it
    does not generate. `extra` names declarations that a binding's handwritten
    runtime presents.
    """
    names: dict[str, str | None] = {}
    for native, value in bound.public_values.items():
        names[native] = type(native)
        for member, _ in value.enum_values:
            names[member] = constant(value, member)
        for member in value.fields:
            if member.public:
                names[f"{native}.{member.name}"] = field(value, member)
        for mask_flag in value.mask_flags:
            names[mask_flag.name] = flag(value, mask_flag) if flag else None
    for native in (*bound.public_handles, *bound.callbacks):
        names.setdefault(native, type(native))
    for plan in bound.operations:
        if plan.role == "public":
            names[plan.name] = operation(plan)
    found = {name: spelled for name, spelled in names.items() if spelled}
    found.update(extra or {})
    return Resolver(found, threads)


@dataclass(frozen=True)
class Resolver:
    """A binding's symbol resolver, and how its own callbacks run.

    `threads` replaces a callback's C `Threads:` paragraph in a binding that
    delivers callbacks through a queue it drains itself, where the native
    threads never run host code.
    """

    names: dict[str, str]
    threads: str | None = None

    def __call__(self, name: str) -> str | None:
        return self.names.get(name)


def _render(markdown: str, escape, code) -> str:
    return "".join(
        code(text) if is_code else escape(text) for text, is_code in spans(markdown)
    )


def _escape_markdown(text: str) -> str:
    """Escape prose for CommonMark, where brackets would start links."""
    return re.sub(r"([\\\[\]<>*])", r"\\\1", text)


def _escape_kdoc(text: str) -> str:
    """Escape prose for KDoc, which reads a backslash literally.

    Character references keep brackets from naming links and keep `*/` from
    closing the comment. Dokka writes a decoded reference into its HTML
    unescaped, so a word with an angle bracket becomes a code span instead.
    """
    return re.sub(
        r"(?P<code>\S*[<>]\S*)|[\[\]*&]",
        lambda match: f"`{match[0]}`" if match["code"] else f"&#{ord(match[0])};",
        text,
    )


# Wrapping never breaks the text of the reference link.
_LINK_TEXT = "C API reference"
_UNBROKEN = _LINK_TEXT.replace(" ", "\0")


def _wrap(text: str, width: int) -> list[str]:
    lines = textwrap.wrap(
        text.replace(_LINK_TEXT, _UNBROKEN),
        max(width, 40),
        break_long_words=False,
        break_on_hyphens=False,
    )
    return [line.replace(_UNBROKEN, _LINK_TEXT) for line in lines]


def markdown_lines(
    doc: Doc | None, width: int, symbol: Symbol, escape=_escape_markdown
) -> list[str]:
    """A Markdown doc comment's lines, wrapped to `width` columns of text."""
    if doc is None:
        return []
    text, notes = _paragraphs(doc, symbol)
    lines = _wrap(_render(text, escape, lambda c: f"`{c}`"), width)
    for note in notes:
        lines += ["", *_wrap(_render(note, escape, lambda c: f"`{c}`"), width)]
    if doc.url:
        lines += [
            "",
            *_wrap(f"See `{doc.symbol}` in the [{_LINK_TEXT}]({doc.url}).", width),
        ]
    return lines


def line_comment(
    doc: Doc | None, symbol: Symbol, indent: str = "", marker: str = "///"
) -> str:
    """A Markdown doc comment of `marker` lines, as Rust, Swift, Dart, and Zig use."""
    lines = markdown_lines(doc, WIDTH - len(indent) - len(marker) - 1, symbol)
    return "".join(f"{indent}{marker} {line}".rstrip() + "\n" for line in lines)


def block_comment(doc: Doc | None, symbol: Symbol, indent: str = "") -> str:
    """A KDoc block comment, which must not contain the closing `*/`."""
    lines = markdown_lines(doc, WIDTH - len(indent) - 3, symbol, _escape_kdoc)
    if not lines:
        return ""
    # A code span keeps its characters, so only a space can split its `*/`.
    body = "".join(
        f"{indent} * {line.replace('*/', '* /')}".rstrip() + "\n" for line in lines
    )
    return f"{indent}/**\n{body}{indent} */\n"


def _xml_escape(text: str) -> str:
    return html.escape(text, quote=False)


def _xml_text(summary: str) -> str:
    return _render(summary, _xml_escape, lambda c: f"<c>{_xml_escape(c)}</c>")


def xml_comment(doc: Doc | None, symbol: Symbol, indent: str = "") -> str:
    """A .NET XML doc comment, with code spans as `<c>` elements."""
    if doc is None:
        return ""
    width = WIDTH - len(indent) - 4
    text, notes = _paragraphs(doc, symbol)
    lines = ["<summary>", *_wrap(_xml_text(text), width), "</summary>"]
    remarks = [_xml_text(note) for note in notes]
    if doc.url:
        remarks.append(
            f"See <c>{_xml_escape(doc.symbol)}</c> in the "
            f'<see href="{_xml_escape(doc.url)}">C API reference</see>.'
        )
    if len(remarks) > 1:
        remarks = [f"<para>{remark}</para>" for remark in remarks]
    if remarks:
        lines += [
            "<remarks>",
            *(line for remark in remarks for line in _wrap(remark, width)),
            "</remarks>",
        ]
    return "".join(f"{indent}/// {line}\n" for line in lines)


def xml_param(doc: Doc | None, symbol: Symbol, name: str, indent: str = "") -> str:
    """A .NET XML `<param>` element, which documents a positional record's
    property."""
    if doc is None:
        return ""
    lines = [
        f'<param name="{name}">',
        *_wrap(_xml_text(_paragraphs(doc, symbol)[0]), WIDTH - len(indent) - 4),
        "</param>",
    ]
    return "".join(f"{indent}/// {line}\n" for line in lines)


def docstring(
    doc: Doc | None, symbol: Symbol, indent: str, notes: tuple[str, ...] = ()
) -> str:
    """A Python docstring statement, with `notes` as further paragraphs.

    The lines escape backslashes and the quotes that would end the literal.
    """
    width = WIDTH - len(indent) - 3
    lines = markdown_lines(doc, width, symbol)
    for note in notes:
        lines += ["", *_wrap(note, width)] if lines else _wrap(note, width)
    if not lines:
        return ""
    lines = [line.replace("\\", "\\\\").replace('"""', '\\"\\"\\"') for line in lines]
    if len(lines) == 1:
        line = lines[0][:-1] + '\\"' if lines[0].endswith('"') else lines[0]
        return f'{indent}"""{line}"""\n'
    body = "".join(f"{indent}{line}".rstrip() + "\n" for line in lines[1:])
    return f'{indent}"""{lines[0]}\n{body}{indent}"""\n'


def go_comment(
    doc: Doc | None,
    symbol: Symbol,
    name: str | None,
    indent: str = "",
    *,
    operation: bool = False,
) -> str:
    """A Go doc comment, which begins with the declared name.

    Go renders doc comments as plain text and links bare URLs, so the summary
    drops its code marks. An operation's summary opens with a verb, which
    continues the name; another declaration's summary follows a sentence that
    names its C declaration. A member, which takes no `name`, carries only its
    summary.
    """
    if doc is None:
        return ""
    text = "".join(part for part, _ in spans(_paragraphs(doc, symbol)[0]))
    if name:
        first, _, rest = text.partition(" ")
        if operation and re.fullmatch(r"[A-Z][a-z]+", first):
            text = f"{name} {first.lower()} {rest}"
        else:
            text = f"{name} corresponds to {doc.symbol}. {text}"
    lines = _wrap(text, WIDTH - len(indent) - 3)
    if doc.url:
        lines += ["", f"See {doc.symbol} in the C API reference:", doc.url]
    return "".join(f"{indent}// {line}".rstrip() + "\n" for line in lines)
