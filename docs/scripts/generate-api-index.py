#!/usr/bin/env python3
"""Write each C declaration's name and reference link in every language.

The generator spells each C declaration as each binding's public symbol, and
each language's reference, built into public/reference, gives the symbol's
page. A symbol that a reference doesn't show has no entry, and the Api
component fails the build when a page names it.
"""

import base64
import importlib
import json
import pathlib
import re
import sys
import zlib
from collections.abc import Callable

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
REFERENCE = REPO_ROOT / "docs" / "public" / "reference"
OUTPUT_PATH = REPO_ROOT / "docs" / "src" / "generated" / "api-index.json"
sys.path.insert(0, str(REPO_ROOT))

from tools.bindgen.compiler import compile_api
from tools.bindgen.docs import Resolver, Symbol
from tools.bindgen.emitters import dart_values, typescript
from tools.bindgen.frontend import parse_headers
from tools.bindgen.schema import validate

BINDINGS = ("dart", "dotnet", "go", "kotlin", "python", "rust", "swift", "zig")

# A symbol's link within its language's reference, or None.
Link = Callable[[str], str | None]


def binding_names() -> dict[str, dict[str, str]]:
    """Each binding's spelling of each C declaration that it exposes."""

    def spellings(symbol: Symbol) -> dict[str, str]:
        assert isinstance(symbol, Resolver)
        return symbol.names

    api = parse_headers(REPO_ROOT / "include")
    validate(api)
    bound = compile_api(api)
    names = {
        binding: spellings(
            importlib.import_module(f"tools.bindgen.emitters.{binding}").symbols(bound)
        )
        for binding in BINDINGS
    }
    # TypeScript names its symbols only while it lowers.
    ts_bound, values, _, lowered, _ = typescript.lower_operations(bound)
    names["typescript"] = spellings(typescript._symbols(ts_bound, values, lowered))
    return names


def anchors(page: pathlib.Path) -> set[str]:
    if not page.is_file():
        return set()
    return set(re.findall(r'\bid="([^"]+)"', page.read_text()))


def c_links() -> Link:
    """Doxygen's tag file names each documented declaration's anchor."""
    import xml.etree.ElementTree as ET

    tags = ET.parse(REFERENCE / "c" / "maplibre-native-c.tag").getroot()
    links = {}
    for compound in tags.iter("compound"):
        owner = compound.findtext("name")
        if compound.get("kind") in ("struct", "union"):
            links[owner] = compound.findtext("filename")
        for member in compound.iter("member"):
            link = f"{member.findtext('anchorfile')}#{member.findtext('anchor')}"
            if compound.get("kind") == "file":
                links.setdefault(member.findtext("name"), link)
            else:
                links[f"{owner}.{member.findtext('name')}"] = link
    return links.get


def dart_links() -> Link:
    index = json.loads((REFERENCE / "dart" / "index.json").read_text())
    links = {entry["qualifiedName"]: entry["href"] for entry in index}
    return lambda name: links.get(f"maplibre_native_ffi.{name}")


def dotnet_links() -> Link:
    xrefmap = (REFERENCE / "dotnet" / "xrefmap.yml").read_text()
    links = dict(
        re.findall(r"^- uid: (\S+)\n(?:  .*\n)*?  href: (\S+)$", xrefmap, re.MULTILINE)
    )

    def link(name: str) -> str | None:
        uid = f"Maplibre.NativeFfi.{name}"
        # A method's page section covers all its overloads.
        return links.get(f"{uid}*") or links.get(uid)

    return link


def go_links() -> Link:
    ids = anchors(REFERENCE / "go" / "index.html")
    return lambda name: f"#{name}" if name in ids else None


def kotlin_links() -> Link:
    pages = json.loads((REFERENCE / "kotlin" / "scripts" / "pages.json").read_text())
    package = "org.maplibre.nativeffi"
    links: dict[str, str] = {}
    for page in pages:
        name = page["description"].removeprefix(f"{package}.")
        # An extension function is a package member, so key it by the type
        # it extends. Each platform that defines one has its own page.
        receiver = re.match(r"(?:fun|val|var) (?:<.*?> )?(\w+)\.", page["name"])
        if "." not in name and receiver:
            name = f"{receiver.group(1)}.{name}"
        links.setdefault(name, page["location"])

    def link(name: str) -> str | None:
        owner, _, member = name.rpartition(".")
        # An enum's constants live on its companion object.
        return links.get(name) or links.get(f"{owner}.Companion.{member}")

    return link


def python_links() -> Link:
    ids = anchors(REFERENCE / "python" / "maplibre_native_ffi.html")
    return lambda name: f"maplibre_native_ffi.html#{name}" if name in ids else None


def rust_links() -> Link:
    crate = REFERENCE / "rust" / "maplibre_native_ffi"
    kinds = ("struct", "enum", "type", "trait", "fn", "constant")
    members = ("method", "variant", "structfield", "associatedconstant", "tymethod")

    def page(item: str) -> str | None:
        return next(
            (
                f"{kind}.{item}.html"
                for kind in kinds
                if (crate / f"{kind}.{item}.html").is_file()
            ),
            None,
        )

    def link(name: str) -> str | None:
        item, _, member = name.partition("::")
        found = page(item)
        if found is None or not member:
            return found and f"maplibre_native_ffi/{found}"
        ids = anchors(crate / found)
        member = member.removeprefix("r#")
        anchor = next(
            (f"{kind}.{member}" for kind in members if f"{kind}.{member}" in ids),
            None,
        )
        return anchor and f"maplibre_native_ffi/{found}#{anchor}"

    return link


def swift_links() -> Link:
    index = json.loads((REFERENCE / "swift" / "index" / "index.json").read_text())
    root = "/documentation/maplibrenativeffi/"
    links: dict[str, str] = {}

    def visit(node: dict) -> None:
        path = node.get("path", "")
        if path.startswith(root):
            # A path spells a function's argument labels, and DocC appends a
            # hash to a path that two symbols share.
            key = re.sub(r"\(.*\)|-\w+$", "", path.removeprefix(root))
            links.setdefault(key, path.removeprefix("/"))
        for child in node.get("children", []):
            visit(child)

    for nodes in index["interfaceLanguages"].values():
        for node in nodes:
            visit(node)
    return lambda name: links.get(name.lower().replace(".", "/"))


def typescript_links() -> Link:
    search = (REFERENCE / "typescript" / "assets" / "search.js").read_text()
    data = re.search(r'"([^"]+)"', search)
    if data is None:
        raise SystemExit("error: TypeDoc's search index has no data")
    rows = json.loads(zlib.decompress(base64.b64decode(data.group(1))))["rows"]
    links: dict[str, list[str]] = {}
    for row in rows:
        parent = row.get("parent", "")
        # The root module holds the API that every platform shares.
        if parent == "index" or parent.startswith("index."):
            key = f"{parent.removeprefix('index').lstrip('.')}.{row['name']}"
            links.setdefault(key.lstrip("."), []).append(row["url"])

    def link(name: str) -> str | None:
        if name in links:
            return links[name][0]
        # The search index leaves out enum members, so look in the pages of
        # the enum's type and its value.
        owner, _, member = name.rpartition(".")
        anchor = member.lower()
        return next(
            (
                f"{page}#{anchor}"
                for page in links.get(owner, [])
                if anchor in anchors(REFERENCE / "typescript" / page)
            ),
            None,
        )

    return link


def zig_links(names: dict[str, str]) -> Link:
    """Autodoc renders in the browser from a hash route of a declaration's
    full name, so no page lists the declarations to check against.

    Fields and enum constants aren't declarations, so they link to their
    container's route.
    """
    members = {
        spelled
        for native, spelled in names.items()
        if "." in native or native.startswith("MLN_")
    }

    def link(name: str) -> str:
        if name in members:
            name = name.rpartition(".")[0]
        return f"#maplibre_native_ffi.{name}"

    return link


# The declarations that a binding leaves to host native code. Its readers see
# the C declaration, linked to the C reference.
NATIVE_ONLY = {"dart": dart_values.NATIVE_ONLY}


LINKS: dict[str, Callable[[], Link]] = {
    "c": c_links,
    "dart": dart_links,
    "dotnet": dotnet_links,
    "go": go_links,
    "kotlin": kotlin_links,
    "python": python_links,
    "rust": rust_links,
    "swift": swift_links,
    "typescript": typescript_links,
}


def main() -> None:
    names = binding_names()
    declarations = set().union(*(spelled.keys() for spelled in names.values()))
    names["c"] = {name: name for name in declarations}
    index: dict[str, dict[str, dict[str, str]]] = {}
    links = {language: make() for language, make in LINKS.items()}
    links["zig"] = zig_links(names["zig"])
    for language, link in links.items():
        for native, spelled in names[language].items():
            url = link(spelled)
            if url is not None:
                index.setdefault(native, {})[language] = {
                    "name": spelled,
                    "url": f"{language}/{url}",
                }
    for language, natives in NATIVE_ONLY.items():
        for native in natives:
            index[native][language] = index[native]["c"]
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(index, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
