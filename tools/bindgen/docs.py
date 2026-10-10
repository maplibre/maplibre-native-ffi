"""Documentation that the generated bindings carry from the C headers.

A generated declaration carries the first paragraph of its C comment and a link
to its header's page in the C API reference. The C header remains the full
contract: status lists, output parameters, and ownership rules name C concepts
that each binding expresses differently, so a binding repeats only the summary.

A summary keeps C identifiers verbatim and marks them as code. Each language
renders the summary through one function here, which applies that language's
escaping to the prose and leaves code spans intact.
"""

from __future__ import annotations

import html
import re
import textwrap
from dataclasses import dataclass

from .model import Api

REFERENCE = "https://maplibre.org/maplibre-native-ffi/reference/c/"

# Generated comment lines stay within this width, including indentation.
WIDTH = 80

_SECTION = re.compile(r"^[A-Z][A-Za-z ]*:$")
_IDENTIFIER = re.compile(
    r"\b(?:mln|MLN)_\w+(?:\.[A-Za-z_]\w*)*(?:\[\w*\])?(?:\(\))?",
)


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


def summary(raw_comment: str) -> str:
    """The first paragraph of a C comment, as one line of Markdown.

    The paragraph ends at a blank line, a list item, or a section heading such
    as `Returns:`. A Doxygen `@file` command line is skipped. The C API's
    identifiers become code spans; the remaining prose stays as written.
    """
    paragraph: list[str] = []
    for line in _comment_lines(raw_comment):
        stripped = line.strip()
        if not paragraph and (not stripped or stripped.startswith(("@file", "\\file"))):
            continue
        if not stripped or _SECTION.match(stripped) or stripped.startswith("- "):
            break
        paragraph.append(stripped)
    return _mark_code(" ".join(" ".join(paragraph).split()))


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
    # The reference page, or empty for a member, whose owner carries the link.
    url: str = ""


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
            docs[name] = Doc(name, text, reference_url(header) if header else "")

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


def spans(markdown: str) -> list[tuple[str, bool]]:
    """Split Markdown into (text, is_code) runs at its code spans."""
    return [
        (part, index % 2 == 1) for index, part in enumerate(markdown.split("`")) if part
    ]


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
    closing the comment.
    """
    return re.sub(r"[\[\]<>*&]", lambda match: f"&#{ord(match[0])};", text)


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


def markdown_lines(doc: Doc | None, width: int, escape=_escape_markdown) -> list[str]:
    """A Markdown doc comment's lines, wrapped to `width` columns of text."""
    if doc is None:
        return []
    lines = _wrap(_render(doc.summary, escape, lambda c: f"`{c}`"), width)
    if doc.url:
        lines += [
            "",
            *_wrap(f"See `{doc.symbol}` in the [{_LINK_TEXT}]({doc.url}).", width),
        ]
    return lines


def line_comment(doc: Doc | None, indent: str = "", marker: str = "///") -> str:
    """A Markdown doc comment of `marker` lines, as Rust, Swift, Dart, and Zig use."""
    lines = markdown_lines(doc, WIDTH - len(indent) - len(marker) - 1)
    return "".join(f"{indent}{marker} {line}".rstrip() + "\n" for line in lines)


def block_comment(doc: Doc | None, indent: str = "") -> str:
    """A KDoc block comment, which must not contain the closing `*/`."""
    lines = markdown_lines(doc, WIDTH - len(indent) - 3, _escape_kdoc)
    if not lines:
        return ""
    # A code span keeps its characters, so only a space can split its `*/`.
    body = "".join(
        f"{indent} * {line.replace('*/', '* /')}".rstrip() + "\n" for line in lines
    )
    return f"{indent}/**\n{body}{indent} */\n"


def xml_comment(doc: Doc | None, indent: str = "") -> str:
    """A .NET XML doc comment, with the C names as `<c>` elements."""
    if doc is None:
        return ""
    width = WIDTH - len(indent) - 4

    def escape(text: str) -> str:
        return html.escape(text, quote=False)

    text = _render(doc.summary, escape, lambda c: f"<c>{escape(c)}</c>")
    lines = ["<summary>", *_wrap(text, width), "</summary>"]
    if doc.url:
        lines += [
            "<remarks>",
            *_wrap(
                f'See <c>{escape(doc.symbol)}</c> in the <see href="{escape(doc.url)}">'
                "C API reference</see>.",
                width,
            ),
            "</remarks>",
        ]
    return "".join(f"{indent}/// {line}\n" for line in lines)


def docstring(doc: Doc | None, indent: str, notes: tuple[str, ...] = ()) -> str:
    """A Python docstring statement, with `notes` as further paragraphs.

    The lines escape backslashes and the quotes that would end the literal.
    """
    width = WIDTH - len(indent) - 3
    lines = markdown_lines(doc, width)
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
    doc: Doc | None, name: str | None, indent: str = "", *, operation: bool = False
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
    text = "".join(part for part, _ in spans(doc.summary))
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
