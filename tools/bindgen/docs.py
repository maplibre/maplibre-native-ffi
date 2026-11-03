"""Documentation that the generated bindings carry from the C headers.

A generated declaration carries its whole C comment except the sections that
only a C caller needs, then a link to its header's page in the C API
reference. A C comment is a one-sentence summary, paragraphs about behavior,
and these sections in order: `Threads:`, `Returns:`, `Completes with:`, and
`Lifetimes:`. A binding repeats the summary, the behavior, and the `Threads:`
paragraph. It turns the statuses that `Returns:` and `Completes with:` list,
other than `MLN_STATUS_OK`, into one list of errors, and leaves `Lifetimes:`
and any other section in C. A callback's `Threads:` paragraph also documents
each registration of it.

The text keeps C identifiers verbatim and marks them as code. Each language
renders it through one function here, which takes the binding's symbol
resolver. The resolver spells each C identifier, and each of the
declaration's parameters, as that binding's public symbol. A sentence or an
error that names something the binding does not expose, such as a queue
member or an output parameter, leaves the binding's text. The rendering then
applies that language's escaping to the prose and leaves code spans intact.
"""

from __future__ import annotations

import dataclasses
import html
import re
import textwrap
from collections.abc import Callable, Iterable
from dataclasses import dataclass, replace
from typing import TYPE_CHECKING

from .model import Api

if TYPE_CHECKING:
    from .semantic import BoundApi, FieldPlan, MaskFlag, OperationPlan, ValuePlan

# A binding's public spelling of a C declaration, or None when the binding
# does not expose it. The argument is a C identifier without a call's `()`: a
# type, function, or enum constant, or a `record.member` path.
Symbol = Callable[[str], "str | None"]

# A paragraph of Markdown, or a list of items that are each one line of it.
Block = str | tuple[str, ...]

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
# The status that reports success, which no error list repeats.
_OK = "`MLN_STATUS_OK`"


def _comment_lines(raw: str) -> list[str]:
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


def _markdown(lines: list[str]) -> str:
    """Lines of prose as one line of Markdown, with C identifiers as code."""
    return _mark_code(" ".join(" ".join(lines).split()))


def _blocks(lines: list[str]) -> list[Block]:
    """The paragraphs and lists of `lines`, which blank lines separate.

    A list item starts with `- `, and a line indented under it continues it.
    """
    blocks: list[Block] = []
    paragraph: list[str] = []
    items: list[list[str]] = []

    def end() -> None:
        if paragraph:
            blocks.append(_markdown(paragraph))
        if items:
            blocks.append(tuple(_markdown(item) for item in items))
        paragraph.clear()
        items.clear()

    for line in lines:
        stripped = line.strip()
        if not stripped:
            end()
        elif stripped.startswith("- "):
            if paragraph:
                end()
            items.append([stripped[2:]])
        elif items and line.startswith(" "):
            items[-1].append(stripped)
        else:
            if items:
                end()
            paragraph.append(stripped)
    end()
    return blocks


def _parse(raw_comment: str) -> tuple[list[Block], dict[str, list[Block]]]:
    """A C comment's blocks before its first section heading, and the blocks
    under each heading, such as `Returns:`, by the heading's name.

    A Doxygen `@file` command line before the comment's text is skipped.
    """
    lines = _comment_lines(raw_comment)
    while lines and (
        not lines[0].strip() or lines[0].strip().startswith(("@file", "\\file"))
    ):
        lines.pop(0)
    body: list[str] = []
    sections: dict[str, list[str]] = {}
    current = body
    for line in lines:
        if _SECTION.match(line.strip()):
            current = sections.setdefault(line.strip()[:-1], [])
        else:
            current.append(line)
    return _blocks(body), {heading: _blocks(text) for heading, text in sections.items()}


def _paragraph(blocks: list[Block]) -> str:
    return blocks[0] if blocks and isinstance(blocks[0], str) else ""


def _items(blocks: list[Block]) -> tuple[str, ...]:
    """The items of a section's lists, except one for `MLN_STATUS_OK`."""
    return tuple(
        item
        for block in blocks
        if isinstance(block, tuple)
        for item in block
        if not item.startswith(_OK)
    )


def summary(raw_comment: str) -> str:
    """The first paragraph of a C comment, as one line of Markdown."""
    return _paragraph(_parse(raw_comment)[0])


def section(raw_comment: str, heading: str) -> str:
    """The paragraph under a C comment's `heading:` line, or empty."""
    return _paragraph(_parse(raw_comment)[1].get(heading, []))


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
    symbol: str
    summary: str
    url: str = ""
    # Paragraphs that a binding adds after the C text, about its own behavior.
    notes: tuple[str, ...] = ()
    # The `Threads:` paragraphs of a callback, or of the callbacks that a
    # registration takes.
    threads: tuple[str, ...] = ()
    # The paragraphs and lists about behavior that follow the summary.
    paragraphs: tuple[Block, ...] = ()
    # The items of the `Returns:` and `Completes with:` lists, without the one
    # for `MLN_STATUS_OK`.
    returns: tuple[str, ...] = ()
    completes: tuple[str, ...] = ()
    # The C parameters that a binding takes from its caller, and those that it
    # takes over, such as a completion, an output, or a callback's context.
    parameters: tuple[str, ...] = ()
    hidden: tuple[str, ...] = ()


def conversion(record: str, options: str) -> Doc:
    """The documentation of a record's generated conversion to the option bag
    that its `options` annotation names."""
    return Doc(
        record, f"Converts this `{record}` to `{options}` that set each of its members."
    )


def index(api: Api) -> dict[str, Doc]:
    """The documentation of each documented public declaration.

    Keys are C names; a record field's key is `record.field`. A declaration
    without a comment is absent, because the reference lists only documented
    declarations. A typedef without its own comment takes the comment of the
    record or enum that it names. A function's diagnostic is hidden, and its
    other parameters are exposed until `with_parameters` says otherwise.
    """
    internal = set(api.runtime_exports) | set(api.runtime_types)
    docs: dict[str, Doc] = {}

    def add(
        name: str,
        comment: str,
        header: str | None,
        parameters: tuple[str, ...] = (),
        hidden: tuple[str, ...] = (),
    ) -> None:
        blocks, sections = _parse(comment)
        text = _paragraph(blocks)
        if not text or name in internal:
            return
        threads = _paragraph(sections.get("Threads", []))
        docs[name] = Doc(
            name,
            text,
            reference_url(header) if header else "",
            threads=(threads,) if threads else (),
            paragraphs=tuple(blocks[1:]),
            returns=_items(sections.get("Returns", [])),
            completes=_items(sections.get("Completes with", [])),
            parameters=parameters,
            hidden=hidden,
        )

    for function in api.functions:
        add(
            function.name,
            function.documentation,
            function.location.path,
            tuple(parameter.name for parameter in function.parameters),
            ("out_diagnostic",) if function.diagnostic else (),
        )
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
            add(
                typedef.name,
                typedef.documentation,
                typedef.location.path,
                tuple(parameter.name for parameter in typedef.parameters),
            )
    return docs


def with_threads(doc: Doc, threads) -> Doc:
    """`doc` ending with each of `threads` that it does not already carry."""
    added = tuple(dict.fromkeys(t for t in threads if t and t not in doc.threads))
    return replace(doc, threads=doc.threads + added) if added else doc


def with_parameters(doc: Doc, exposed: Iterable[str]) -> Doc:
    """`doc` with only `exposed` of its parameters taken from the caller."""
    exposed = tuple(name for name in exposed if name in doc.parameters)
    hidden = tuple(name for name in doc.parameters if name not in exposed)
    return replace(doc, parameters=exposed, hidden=doc.hidden + hidden)


def discarding(doc: Doc | None) -> Doc | None:
    """`doc` for a form that passes no completion, and so reports none of the
    errors that the completion would."""
    return replace(doc, completes=()) if doc else doc


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


# A binding's spelling of each of a declaration's parameters, or None for one
# that the binding takes over.
Parameters = dict[str, "str | None"]


def _rewrite(
    sentence: str, symbol: Symbol, parameters: Parameters | None = None
) -> tuple[str, str | None]:
    """The sentence with each C identifier and parameter spelled as the
    binding spells it, and the first one that it cannot spell, or None."""
    parameters = parameters or {}
    parts = sentence.split("`")
    for index in range(1, len(parts), 2):
        if parts[index] in parameters:
            resolved = parameters[parts[index]]
            if resolved is None:
                return sentence, parts[index]
            parts[index] = resolved
            continue
        match = _REFERENCE.fullmatch(parts[index])
        if match is None:
            continue
        resolved = symbol(match[1])
        if resolved is None:
            return sentence, match[1]
        parts[index] = resolved + (match[2] or "")
    return "`".join(parts), None


def translate(
    markdown: str,
    symbol: Symbol,
    name: str = "",
    parameters: Parameters | None = None,
    *,
    first: bool = True,
) -> str:
    """A paragraph in a binding's terms.

    Each C identifier becomes the symbol that the binding exposes, and each
    parameter the binding's name for it. A sentence that names a declaration
    or parameter the binding does not expose describes something the binding
    hides, such as a queue, a callback context, or an output parameter, and
    leaves the paragraph. A summary's first sentence states what the
    declaration is, so with `first`, it must not name one; `name` identifies
    the summary in the error.
    """
    kept = []
    for index, sentence in enumerate(sentences(markdown)):
        rewritten, missing = _rewrite(sentence, symbol, parameters)
        if missing is None:
            kept.append(rewritten)
        elif first and index == 0:
            raise DocError(
                f"{name}: the first sentence names {missing}, which this binding "
                "does not expose; reword the C comment's first sentence"
            )
    return " ".join(kept)


@dataclass(frozen=True)
class Content:
    """A declaration's documentation in a binding's terms, as Markdown."""

    summary: str
    # The behavior, the threads, and the binding's notes.
    blocks: list[Block]
    errors: list[str]


def _content(doc: Doc, symbol: Symbol) -> Content:
    spell = getattr(symbol, "parameter", None) or (lambda name: name)
    record = doc.symbol.split(".")[0]
    parameters: Parameters = {
        **dict.fromkeys(doc.hidden),
        **dict.fromkeys(getattr(symbol, "fields", {}).get(record, ())),
        **{name: spell(name) for name in doc.parameters},
    }

    def prose(markdown: str) -> str:
        return translate(markdown, symbol, parameters=parameters, first=False)

    def body(block: Block) -> Block:
        if isinstance(block, str):
            return prose(block)
        return tuple(text for item in block if (text := prose(item)))

    # A binding's own paragraph replaces the C ones.
    own = getattr(symbol, "threads", {}).get(doc.symbol)
    if own is None:
        threads = [prose(paragraph) for paragraph in doc.threads]
    else:
        threads = [own] if doc.threads else []
    blocks = [
        *map(body, doc.paragraphs),
        *threads,
        *(translate(note, symbol, doc.symbol, parameters) for note in doc.notes),
    ]
    errors = []
    for item in (*doc.returns, *doc.completes):
        rewritten, missing = _rewrite(item, symbol, parameters)
        if missing is None:
            errors.append(rewritten)
    return Content(
        translate(doc.summary, symbol, doc.symbol, parameters),
        [block for block in blocks if block],
        list(dict.fromkeys(errors)),
    )


def resolver(
    bound: BoundApi,
    *,
    type: Callable[[str], str | None],
    operation: Callable[[OperationPlan], str | None],
    constant: Callable[[ValuePlan, str], str | None],
    field: Callable[[ValuePlan, FieldPlan], str | None],
    flag: Callable[[ValuePlan, MaskFlag], str | None] | None = None,
    extra: dict[str, str] | None = None,
    threads: dict[str, str] | None = None,
    fields: dict[str, frozenset[str]] | None = None,
    parameter: Callable[[str], str] | None = None,
) -> Symbol:
    """A binding's symbol resolver over the declarations that a host can name.

    Those are the public values, handles, and callbacks, each public
    operation, the constants of each public enum, the public fields of each
    public record, and the flags of a record's mask. Each function spells a
    declaration as the binding generates it, or returns None for one that it
    does not generate. `extra` names declarations that a binding's handwritten
    runtime presents. `threads` and `fields` are as `Resolver` describes.
    `parameter` spells a C parameter name as the binding's parameters do;
    without it, a parameter keeps its C name.
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
    return Resolver(found, threads or {}, fields or {}, parameter)


@dataclass(frozen=True)
class Resolver:
    """A binding's symbol resolver, how its own callbacks run, and how it
    spells parameters.

    `threads` replaces the C `Threads:` paragraphs of a declaration, by its C
    name, in a binding that delivers callbacks through a queue it drains
    itself, where the native threads never run host code. An empty paragraph
    drops them. `fields` names the fields of a record, by the record's C name,
    that the binding leaves out, so a sentence that names one leaves the
    record's documentation and its fields'.
    """

    names: dict[str, str]
    threads: dict[str, str] = dataclasses.field(default_factory=dict)
    fields: dict[str, frozenset[str]] = dataclasses.field(default_factory=dict)
    parameter: Callable[[str], str] | None = None

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


# Between a slash and an asterisk, which together open or close a comment.
_COMMENT_MARK = re.compile(r"(?<=/)(?=\*)|(?<=\*)(?=/)")

# Wrapping never breaks the text of the reference link.
_LINK_TEXT = "C API reference"
_UNBROKEN = _LINK_TEXT.replace(" ", "\0")


def _wrap(text: str, width: int, first: str = "", rest: str = "") -> list[str]:
    """`text` wrapped to `width`, its first line after `first` and each other
    line after `rest`, as a list item's marker and continuation."""
    lines = textwrap.wrap(
        text.replace(_LINK_TEXT, _UNBROKEN),
        max(width, 40),
        initial_indent=first,
        subsequent_indent=rest,
        break_long_words=False,
        break_on_hyphens=False,
    )
    return [line.replace(_UNBROKEN, _LINK_TEXT) for line in lines]


def _block_lines(
    block: Block, width: int, render, marker: str = "- ", indent: str = "  "
) -> list[str]:
    """A paragraph's wrapped lines, or a list's items after `marker`."""
    if isinstance(block, str):
        return _wrap(render(block), width)
    return [
        line for item in block for line in _wrap(render(item), width, marker, indent)
    ]


def markdown_lines(
    doc: Doc | None,
    width: int,
    symbol: Symbol,
    escape=_escape_markdown,
    errors: str = "Errors:",
) -> list[str]:
    """The Markdown of a doc comment, whose errors follow the `errors` line."""
    if doc is None:
        return []
    content = _content(doc, symbol)

    def render(markdown: str) -> str:
        return _render(markdown, escape, lambda c: f"`{c}`")

    lines = _wrap(render(content.summary), width)
    for block in content.blocks:
        lines += ["", *_block_lines(block, width, render)]
    if doc.url:
        lines += [
            "",
            *_wrap(f"See `{doc.symbol}` in the [{_LINK_TEXT}]({doc.url}).", width),
        ]
    if content.errors:
        lines += ["", errors, "", *_block_lines(tuple(content.errors), width, render)]
    return lines


def line_comment(
    doc: Doc | None,
    symbol: Symbol,
    indent: str = "",
    marker: str = "///",
    errors: str = "Errors:",
) -> str:
    """A Markdown doc comment of `marker` lines, as Rust, Swift, Dart, and Zig
    use. Rust heads its errors `# Errors`, as rustdoc expects."""
    lines = markdown_lines(
        doc, WIDTH - len(indent) - len(marker) - 1, symbol, errors=errors
    )
    return "".join(f"{indent}{marker} {line}".rstrip() + "\n" for line in lines)


def block_comment(doc: Doc | None, symbol: Symbol, indent: str = "") -> str:
    """A KDoc block comment, which must not contain the closing `*/`, nor
    the `/*` that opens a comment nested in it, as Kotlin reads one."""
    lines = markdown_lines(doc, WIDTH - len(indent) - 3, symbol, _escape_kdoc)
    if not lines:
        return ""
    # A code span keeps its characters, so only a space can split its `*/`.
    body = "".join(
        f"{indent} * {_COMMENT_MARK.sub(' ', line)}".rstrip() + "\n" for line in lines
    )
    return f"{indent}/**\n{body}{indent} */\n"


def _xml_escape(text: str) -> str:
    return html.escape(text, quote=False)


def _xml_text(summary: str) -> str:
    return _render(summary, _xml_escape, lambda c: f"<c>{_xml_escape(c)}</c>")


def _xml_list(items: Iterable[str], width: int) -> list[str]:
    return [
        '<list type="bullet">',
        *(
            line
            for item in items
            for line in _wrap(f"<item><description>{item}</description></item>", width)
        ),
        "</list>",
    ]


def _xml_blocks(blocks: Iterable[Block], width: int) -> list[str]:
    """XML paragraphs and lists, each paragraph a `<para>` when there are
    several blocks."""
    blocks = list(blocks)
    lines = []
    for block in blocks:
        if isinstance(block, tuple):
            lines += _xml_list(block, width)
        elif len(blocks) > 1:
            lines += _wrap(f"<para>{block}</para>", width)
        else:
            lines += _wrap(block, width)
    return lines


def xml_comment(doc: Doc | None, symbol: Symbol, indent: str = "") -> str:
    """A .NET XML doc comment, with code spans as `<c>` elements.

    The summary is the `<summary>`; the rest is the `<remarks>`, which ends
    with the reference link and the errors.
    """
    if doc is None:
        return ""
    width = WIDTH - len(indent) - 4
    content = _content(doc, symbol)
    lines = ["<summary>", *_wrap(_xml_text(content.summary), width), "</summary>"]
    remarks: list[Block] = [
        tuple(map(_xml_text, block)) if isinstance(block, tuple) else _xml_text(block)
        for block in content.blocks
    ]
    if doc.url:
        remarks.append(
            f"See <c>{_xml_escape(doc.symbol)}</c> in the "
            f'<see href="{_xml_escape(doc.url)}">C API reference</see>.'
        )
    if content.errors:
        remarks += ["Errors:", tuple(map(_xml_text, content.errors))]
    if remarks:
        lines += ["<remarks>", *_xml_blocks(remarks, width), "</remarks>"]
    return "".join(f"{indent}/// {line}\n" for line in lines)


def xml_param(doc: Doc | None, symbol: Symbol, name: str, indent: str = "") -> str:
    """A .NET XML `<param>` element, which documents a positional record's
    property."""
    if doc is None:
        return ""
    content = _content(doc, symbol)
    blocks: list[Block] = [
        tuple(map(_xml_text, block)) if isinstance(block, tuple) else _xml_text(block)
        for block in (content.summary, *content.blocks)
    ]
    lines = [
        f'<param name="{name}">',
        *_xml_blocks(blocks, WIDTH - len(indent) - 4),
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
    if doc is not None:
        lines = markdown_lines(replace(doc, notes=(*doc.notes, *notes)), width, symbol)
    else:
        lines = [line for note in notes for line in ["", *_wrap(note, width)]][1:]
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

    Go renders doc comments as plain text and links bare URLs, so the text
    drops its code marks. An operation's summary opens with a verb, which
    continues the name; another declaration's summary follows a sentence that
    names its C declaration. A member takes no `name`. Lists are Go doc lists.
    """
    if doc is None:
        return ""
    content = _content(doc, symbol)

    def plain(markdown: str) -> str:
        return "".join(part for part, _ in spans(markdown))

    width = WIDTH - len(indent) - 3
    text = plain(content.summary)
    if name:
        first, _, rest = text.partition(" ")
        if operation and re.fullmatch(r"[A-Z][a-z]+", first):
            text = f"{name} {first.lower()} {rest}"
        else:
            text = f"{name} corresponds to {doc.symbol}. {text}"
    lines = _wrap(text, width)
    for block in content.blocks:
        lines += ["", *_block_lines(block, width, plain, "  - ", "    ")]
    if doc.url:
        lines += ["", f"See {doc.symbol} in the C API reference:", doc.url]
    if content.errors:
        lines += [
            "",
            "Errors:",
            *_block_lines(tuple(content.errors), width, plain, "  - ", "    "),
        ]
    return "".join(f"{indent}// {line}".rstrip() + "\n" for line in lines)
