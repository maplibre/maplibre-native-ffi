// The styles that the example apps offer, which examples/styles.json lists for
// all of them.

import list from "../../styles.json";

export interface Style {
  /** Names the style in the page's URL, as ?style=. */
  readonly id: string;
  readonly name: string;
  readonly provider: string;
  /** Where the style loads from, if the list doesn't hold the style itself. */
  readonly url?: string;
  /** The style itself, if it isn't at a URL. */
  readonly json?: object;
  /** Whether the style draws a dark map. */
  readonly dark: boolean;
  /** Colors from the style, for a preview of it. */
  readonly swatch: {
    readonly land: string;
    readonly water: string;
    readonly road: string;
  };
}

export const styles: readonly Style[] = list.styles;

export const defaultStyle: Style = styles.find(
  (style) => style.id === list.default,
)!;
