/**
 * Extracts a named region from a snippet file imported with `?raw`.
 *
 * Regions are named rather than numbered so that editing or reformatting a
 * snippet cannot silently point a page at the wrong lines. Mark one with
 * matching comments, `//` or `#` as the language uses:
 *
 * ```c
 * // #region runtime
 * mln_runtime_create(&options, &runtime, NULL);
 * // #endregion runtime
 * ```
 *
 * A name may mark several parts of a file, such as a callback and the call
 * that uses it. The parts join in file order, separated by a blank line.
 */
export function region(source: string, name: string): string {
  const lines = source.split("\n");
  const parts: string[] = [];
  let index = 0;
  for (;;) {
    const start = lines.findIndex(
      (line, at) => at >= index && isMarker(line, "region", name),
    );
    if (start === -1) break;
    const end = lines.findIndex(
      (line, at) => at > start && isMarker(line, "endregion", name),
    );
    if (end === -1) {
      throw new Error(`snippet region "${name}" has no "#endregion ${name}"`);
    }
    const body = lines
      .slice(start + 1, end)
      .filter(
        (line) => !isMarker(line, "region") && !isMarker(line, "endregion"),
      );
    parts.push(dedent(trimBlankEdges(body)).join("\n"));
    index = end + 1;
  }
  if (parts.length === 0) {
    throw new Error(`snippet region "${name}" has no "#region ${name}"`);
  }
  return parts.join("\n\n");
}

function isMarker(line: string, kind: string, name?: string): boolean {
  const suffix = name === undefined ? "\\s*\\S*" : `\\s+${escape(name)}`;
  return new RegExp(`^\\s*(?://|#)\\s*#${kind}${suffix}\\s*$`).test(line);
}

function escape(value: string): string {
  return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function trimBlankEdges(lines: string[]): string[] {
  let first = 0;
  let last = lines.length;
  while (first < last && lines[first]?.trim() === "") first++;
  while (last > first && lines[last - 1]?.trim() === "") last--;
  return lines.slice(first, last);
}

function dedent(lines: string[]): string[] {
  const indents = lines
    .filter((line) => line.trim() !== "")
    .map((line) => line.length - line.trimStart().length);
  const common = indents.length === 0 ? 0 : Math.min(...indents);
  return lines.map((line) => line.slice(common));
}
