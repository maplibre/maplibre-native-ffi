// The control panel: the style picker, the backend links, the comparison
// toggle, and the status line.

import { styles, type Style } from "./styles.ts";

const svgNs = "http://www.w3.org/2000/svg";

/**
 * A small map tile in `style`'s colors: land, a coast of water, a main road,
 * and a lesser one.
 */
export function swatch(style: Style): SVGSVGElement {
  const svg = document.createElementNS(svgNs, "svg");
  svg.setAttribute("viewBox", "0 0 48 32");
  svg.setAttribute("preserveAspectRatio", "xMidYMid slice");
  svg.setAttribute("aria-hidden", "true");
  svg.classList.add("swatch");
  const { land, water, road } = style.swatch;
  const shapes: [string, Record<string, string>][] = [
    ["rect", { width: "48", height: "32", fill: land }],
    ["path", { d: "M0 19C9 18 15 24 19 32H0Z", fill: water }],
    [
      "path",
      {
        d: "M30 -2C29 10 36 20 50 24",
        fill: "none",
        stroke: road,
        "stroke-width": "1.5",
        opacity: "0.7",
      },
    ],
    [
      "path",
      {
        d: "M-2 9C14 13 26 7 50 13",
        fill: "none",
        stroke: road,
        "stroke-width": "3.5",
        "stroke-linecap": "round",
      },
    ],
  ];
  for (const [tag, attributes] of shapes) {
    const shape = document.createElementNS(svgNs, tag);
    for (const [name, value] of Object.entries(attributes)) {
      shape.setAttribute(name, value);
    }
    svg.append(shape);
  }
  return svg;
}

/**
 * Fills the style picker, and calls `pick` when the user picks a style. The
 * picker stays open, so the user can step through the styles.
 */
export function addStylePicker(
  current: Style,
  pick: (style: Style) => void,
): void {
  const button = document.querySelector<HTMLButtonElement>("#style-button")!;
  const list = document.querySelector<HTMLElement>("#styles")!;

  const showCurrent = (style: Style) => {
    const name = document.createElement("span");
    name.className = "style-name";
    name.textContent = style.name;
    const provider = document.createElement("span");
    provider.className = "style-provider";
    provider.textContent = style.provider;
    const text = document.createElement("span");
    text.className = "style-text";
    text.append(name);
    if (style.provider !== style.name) text.append(provider);
    const chevron = document.createElementNS(svgNs, "svg");
    chevron.setAttribute("viewBox", "0 0 16 16");
    chevron.setAttribute("aria-hidden", "true");
    chevron.classList.add("chevron");
    const path = document.createElementNS(svgNs, "path");
    path.setAttribute("d", "M4 6l4 4 4-4");
    chevron.append(path);
    button.replaceChildren(swatch(style), text, chevron);
  };

  // The picker groups styles by provider, and the providers of one style
  // together.
  const providers = Map.groupBy(styles, (style) => style.provider);
  const groups = Map.groupBy(styles, (style) =>
    providers.get(style.provider)!.length > 1 ? style.provider : "Other",
  );
  for (const [heading, group] of groups) {
    const fieldset = document.createElement("fieldset");
    const legend = document.createElement("legend");
    legend.textContent = heading;
    const options = document.createElement("div");
    options.className = "style-options";
    fieldset.append(legend, options);
    for (const style of group) {
      const radio = document.createElement("input");
      radio.type = "radio";
      radio.name = "style";
      radio.value = style.id;
      radio.checked = style === current;
      radio.addEventListener("change", () => {
        showCurrent(style);
        pick(style);
      });
      const name = document.createElement("span");
      name.textContent = style.name;
      const option = document.createElement("label");
      option.className = "style-option";
      option.append(radio, swatch(style), name);
      options.append(option);
    }
    list.append(fieldset);
  }

  const setOpen = (open: boolean) => {
    list.hidden = !open;
    button.ariaExpanded = String(open);
  };
  button.addEventListener("click", () => {
    const open = button.ariaExpanded !== "true";
    setOpen(open);
    if (open) list.querySelector<HTMLInputElement>("input:checked")?.focus();
  });
  list.addEventListener("keydown", (event) => {
    if (event.key !== "Escape") return;
    setOpen(false);
    button.focus();
  });
  showCurrent(current);
}

/** Lets the header's button hide the controls, and hides them on a phone. */
export function addCollapse(): void {
  const button = document.querySelector<HTMLButtonElement>("#collapse")!;
  const controls = document.querySelector<HTMLElement>("#controls")!;
  const setOpen = (open: boolean) => {
    controls.hidden = !open;
    button.ariaExpanded = String(open);
  };
  setOpen(!matchMedia("(max-width: 560px)").matches);
  button.addEventListener("click", () =>
    setOpen(button.ariaExpanded !== "true"),
  );
}

/**
 * Shows the style's attributions, which are HTML. The page keeps only their
 * text and their links.
 */
export function showAttributions(attributions: readonly string[]): void {
  const footer = document.querySelector<HTMLElement>("#attribution")!;
  const items = [...new Set(attributions.filter((html) => html !== ""))].map(
    (html) => {
      const item = document.createElement("span");
      const parsed = new DOMParser().parseFromString(html, "text/html");
      item.append(...safeNodes(parsed.body));
      return item;
    },
  );
  footer.replaceChildren(...items);
  footer.hidden = items.length === 0;
}

function safeNodes(parent: Node): Node[] {
  return [...parent.childNodes].flatMap((node): Node[] => {
    if (node.nodeType === Node.TEXT_NODE) {
      return [document.createTextNode(node.textContent ?? "")];
    }
    if (!(node instanceof Element)) return [];
    const href = node instanceof HTMLAnchorElement ? node.href : "";
    if (/^https?:/.test(href)) {
      const link = document.createElement("a");
      link.href = href;
      link.target = "_blank";
      link.rel = "noopener";
      link.textContent = node.textContent;
      return [link];
    }
    return safeNodes(node);
  });
}
