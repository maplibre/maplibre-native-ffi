/**
 * Draws the page's Mermaid diagrams in the site's light or dark theme, and
 * redraws them when the theme changes. `src/integrations/mermaid.ts` writes the
 * placeholders.
 */
import mermaid from "mermaid";

export async function drawDiagrams(): Promise<void> {
  const figures = document.querySelectorAll<HTMLElement>(".mermaid-diagram");
  const dark = document.documentElement.dataset.theme !== "light";
  mermaid.initialize({
    startOnLoad: false,
    theme: dark ? "dark" : "default",
    // The site writes every diagram, so `click` links may run.
    securityLevel: "loose",
  });
  let index = 0;
  for (const figure of figures) {
    const source = figure.querySelector(".mermaid-source")?.textContent ?? "";
    const { svg } = await mermaid.render(`mermaid-${index++}`, source);
    figure.querySelector("svg")?.remove();
    figure.insertAdjacentHTML("beforeend", svg);
  }
}
