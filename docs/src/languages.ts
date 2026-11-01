/**
 * The languages that guide snippets show, in picker order. The site config
 * reads this too, so it must not use Vite-only features.
 */

export interface Language {
  /** The value the picker saves and the snippets match. */
  id: string;
  label: string;
  /** The highlighter's name for the language. */
  lang: string;
  /** The file name each snippet shows in its header. */
  title: string;
  /** The language's logo. */
  logo: string;
}

/** A color logo from Devicon (MIT), served by jsDelivr. */
function devicon(name: string, version = "v2.18.0"): string {
  return `https://cdn.jsdelivr.net/gh/devicons/devicon@${version}/icons/${name}.svg`;
}

export const languages: Language[] = [
  {
    id: "c",
    label: "C",
    lang: "c",
    title: "main.c",
    // Later releases replaced the blue hexagon with a gray letter.
    logo: devicon("c/c-original", "v2.16.0"),
  },
  {
    id: "dart",
    label: "Dart",
    lang: "dart",
    title: "main.dart",
    logo: devicon("dart/dart-original"),
  },
  {
    id: "dotnet",
    label: ".NET",
    lang: "csharp",
    title: "Program.cs",
    logo: devicon("csharp/csharp-original"),
  },
  {
    id: "go",
    label: "Go",
    lang: "go",
    title: "main.go",
    logo: devicon("go/go-original-wordmark"),
  },
  {
    id: "kotlin",
    label: "Kotlin",
    lang: "kotlin",
    title: "Main.kt",
    logo: devicon("kotlin/kotlin-original"),
  },
  {
    id: "python",
    label: "Python",
    lang: "python",
    title: "main.py",
    logo: devicon("python/python-original"),
  },
  {
    id: "rust",
    label: "Rust",
    lang: "rust",
    title: "main.rs",
    logo: devicon("rust/rust-original"),
  },
  {
    id: "swift",
    label: "Swift",
    lang: "swift",
    title: "main.swift",
    logo: devicon("swift/swift-original"),
  },
  {
    id: "typescript",
    label: "TypeScript",
    lang: "ts",
    title: "main.ts",
    logo: devicon("typescript/typescript-original"),
  },
  {
    id: "zig",
    label: "Zig",
    lang: "zig",
    title: "main.zig",
    logo: devicon("zig/zig-original"),
  },
];

/** The language a reader sees until they pick one. */
export const defaultLanguage = "c";

/** The local storage key that keeps the reader's pick. */
export const languageStorageKey = "mln-language";
