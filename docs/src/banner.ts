import { defineRouteMiddleware } from "@astrojs/starlight/route-data";

/** Shows the build's HTML preview banner. Release builds leave DOCS_BANNER unset. */
export const onRequest = defineRouteMiddleware(({ locals }) => {
  const content = process.env.DOCS_BANNER;
  if (content) {
    locals.starlightRoute.entry.data.banner ??= { content };
  }
});
