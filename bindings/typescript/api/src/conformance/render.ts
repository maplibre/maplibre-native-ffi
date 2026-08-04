/**
 * Render sessions, and the queries that only a session can answer.
 *
 * Every case here needs a live graphics context, which is a property of the
 * host rather than of the transport: a browser has WebGL, and a bare Node
 * process has none. A runner that cannot supply one leaves these out through
 * `needs`, rather than each case checking for itself and passing when it found
 * nothing.
 */

import { jsonBool, jsonObject, type JsonValue } from "../json.ts";
import { pointQuery } from "../query.ts";
import type { ConformanceGroup } from "./harness.ts";
import { EMPTY_STYLE, loadStyle, withRuntime } from "./harness.ts";

const NEEDS_CONTEXT = ["renderContext"] as const;
const EXTENT = { width: 256, height: 256 };

/** A style with one source and one layer, so a query has something to find. */
const POINT_STYLE = JSON.stringify({
  version: 8,
  name: "one point",
  sources: {
    points: {
      type: "geojson",
      data: {
        type: "FeatureCollection",
        features: [
          {
            type: "Feature",
            id: 7,
            properties: { name: "middle", rank: 1 },
            geometry: { type: "Point", coordinates: [0, 0] },
          },
        ],
      },
    },
  },
  layers: [
    {
      id: "points",
      type: "circle",
      source: "points",
      paint: { "circle-radius": 40 },
    },
  ],
});

export const RENDER_SESSION_GROUP: ConformanceGroup = {
  name: "render sessions",
  cases: [
    {
      name: "attaches a session-owned texture and renders through it",
      spec: ["BND-162"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open(EXTENT);
          const session = map.attachOpenGlOwnedTexture({
            extent: EXTENT,
            context: renderContext(),
          });
          try {
            expect.equal(session.isClosed, false, "the session is live");
            loadStyle(runtime, map, EMPTY_STYLE);
            // The first update after a style loads has something to draw, and
            // whether it does is the C API's answer rather than this binding's,
            // so only the shape of the answer is asserted here.
            expect.equal(
              typeof session.renderUpdate(),
              "boolean",
              "an update reports whether it drew",
            );
          } finally {
            session.close();
          }
          expect.equal(session.isClosed, true, "the session closed");
        });
      },
    },
    {
      name: "refuses a second session on the same map",
      spec: ["BND-163"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext }) {
        withRuntime(maplibre, (_runtime, open) => {
          const map = open(EXTENT);
          const session = map.attachOpenGlOwnedTexture({
            extent: EXTENT,
            context: renderContext(),
          });
          try {
            const error = expect.throws(
              () =>
                map.attachOpenGlOwnedTexture({
                  extent: EXTENT,
                  context: renderContext(),
                }),
              "attaching a second session",
            );
            expect.equal(error.kind, "invalidState", "the error kind");
            expect.equal(
              session.isClosed,
              false,
              "the first session is untouched",
            );
          } finally {
            session.close();
          }
        });
      },
    },
    {
      name: "reports an update with nothing to draw without closing",
      spec: ["BND-164"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext }) {
        withRuntime(maplibre, (_runtime, open) => {
          const map = open(EXTENT);
          const session = map.attachOpenGlOwnedTexture({
            extent: EXTENT,
            context: renderContext(),
          });
          try {
            // No style has been loaded, so there is nothing to draw. That is an
            // answer rather than a failure, and the session stays usable.
            expect.equal(
              session.renderUpdate(),
              false,
              "no update was available",
            );
            expect.equal(session.isClosed, false, "the session is still live");
            expect.equal(
              session.renderUpdate(),
              false,
              "and answers again the same way",
            );
          } finally {
            session.close();
          }
        });
      },
    },
    {
      name: "resizes through the session",
      spec: ["BND-165"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext }) {
        withRuntime(maplibre, (_runtime, open) => {
          const map = open(EXTENT);
          const session = map.attachOpenGlOwnedTexture({
            extent: EXTENT,
            context: renderContext(),
          });
          try {
            // A resize changes the target this session draws into, which is not
            // the map's own extent and which the C API offers no way to read
            // back, so what is checked is that the call reaches native code,
            // leaves the session usable, and validates what it is given.
            session.resize({ width: 320, height: 200, scaleFactor: 2 });
            expect.equal(session.isClosed, false, "the session survived");
            expect.equal(
              session.renderUpdate(),
              false,
              "and still answers an update",
            );
            expect.equal(
              map.getSize().width,
              EXTENT.width,
              "the map's own extent is untouched",
            );
            expect.throwsAny(
              () => session.resize({ width: -1, height: 200 }),
              "resizing to an extent that is not a size",
            );
          } finally {
            session.close();
          }
        });
      },
    },
    {
      name: "refuses to attach through a map that has closed",
      spec: ["BND-196"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext }) {
        const runtime = maplibre.createRuntime();
        try {
          const map = runtime.createMap(EXTENT);
          map.close();
          const error = expect.throws(
            () =>
              map.attachOpenGlOwnedTexture({
                extent: EXTENT,
                context: renderContext(),
              }),
            "attaching through a closed map",
          );
          expect.ok(
            error.kind === "invalidInput" || error.kind === "closedHandle",
            `the error names the map state, and was ${error.kind}`,
          );
        } finally {
          runtime.close();
        }
      },
    },
    {
      name: "copies queried features, their properties, and their state",
      spec: ["BND-106"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open(EXTENT);
          const session = map.attachOpenGlOwnedTexture({
            extent: EXTENT,
            context: renderContext(),
          });
          try {
            map.jumpTo({ center: { latitude: 0, longitude: 0 }, zoom: 4 });
            expect.ok(loadStyle(runtime, map, POINT_STYLE), "the style loaded");
            // A query reads the last frame, so one is drawn first. Tiles arrive
            // through the loader, which is why this pumps rather than asking
            // once.
            for (let attempt = 0; attempt < 120; attempt += 1) {
              runtime.pump(25);
              session.renderUpdate();
            }

            // The source query answers from the source rather than the frame,
            // so it finds the feature whatever was drawn. Everything it returns
            // is copied out of memory the result owns, which is the point.
            const fromSource = session.querySourceFeatures("points", {});
            const first = expect.defined(fromSource[0], "a feature");
            const named = first.feature.properties?.find(
              (member) => member.name === "name",
            );
            expect.equal(
              expect.defined(named, "the name property").value,
              { kind: "string", value: "middle" } as JsonValue,
              "the copied property",
            );
            expect.equal(
              first.feature.geometry.kind,
              "point",
              "the copied geometry",
            );
          } finally {
            session.close();
          }
        });
      },
    },
    {
      name: "reads a rendered frame back to the CPU",
      spec: ["BND-166"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open(EXTENT);
          const session = map.attachOpenGlOwnedTexture({
            extent: EXTENT,
            context: renderContext(),
          });
          try {
            expect.ok(loadStyle(runtime, map, POINT_STYLE), "the style loaded");
            for (let attempt = 0; attempt < 60; attempt += 1) {
              runtime.pump(25);
              session.renderUpdate();
            }

            const info = session.imageInfo();
            expect.equal(info.width, EXTENT.width, "the image width");
            expect.equal(info.height, EXTENT.height, "the image height");
            expect.ok(
              info.byteLength >= info.width * info.height * 4,
              "the image needs at least four bytes a pixel",
            );
            expect.ok(
              info.stride >= info.width * 4,
              "a row is at least four bytes a pixel",
            );

            // A buffer too small is refused, and stays the caller's: nothing is
            // written into it and it can be grown and offered again.
            const small = new Uint8Array(info.byteLength - 1).fill(0xab);
            expect.throws(
              () => session.readPremultipliedRgba8(small),
              "reading into a buffer too small for the image",
            );
            expect.ok(
              small.every((byte) => byte === 0xab),
              "the buffer it refused is untouched",
            );

            // The same buffer serves again, which is what sizing it once is
            // for.
            const pixels = new Uint8Array(info.byteLength);
            const first = session.readPremultipliedRgba8(pixels);
            expect.equal(first.byteLength, info.byteLength, "the copied size");
            expect.ok(
              pixels.some((byte) => byte !== 0),
              "the image has something in it",
            );
            const again = session.readPremultipliedRgba8(pixels);
            expect.equal(
              again.width,
              first.width,
              "a reused buffer reads again",
            );
          } finally {
            session.close();
          }
        });
      },
    },
    {
      name: "lends a rendered texture and takes it back",
      spec: ["BND-167", "BND-168", "BND-170", "BND-173"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open(EXTENT);
          const session = map.attachOpenGlOwnedTexture({
            extent: EXTENT,
            context: renderContext(),
          });
          try {
            expect.ok(loadStyle(runtime, map, POINT_STYLE), "the style loaded");
            for (let attempt = 0; attempt < 60; attempt += 1) {
              runtime.pump(25);
              session.renderUpdate();
            }

            const frame = session.acquireOpenGlFrame();
            expect.ok(frame.handles.texture > 0, "a borrowed texture name");
            expect.equal(frame.handles.width, EXTENT.width, "the frame width");
            expect.equal(frame.isReleased, false, "the frame is held");

            // While it is held the session cannot disturb the texture, so
            // everything that would reports rather than doing it.
            expect.throwsAny(
              () => session.acquireOpenGlFrame(),
              "a second acquire while one is held",
            );
            expect.throwsAny(
              () => session.renderUpdate(),
              "rendering while a frame is held",
            );
            expect.throwsAny(
              () => session.resize({ width: 64, height: 64 }),
              "resizing while a frame is held",
            );

            frame.release();
            expect.equal(frame.isReleased, true, "the frame was given back");
            // A released frame hands out no handles: the session may have
            // reused that texture name by now.
            expect.equal(
              expect.throws(
                () => frame.handles,
                "reading handles after release",
              ).kind,
              "closedHandle",
              "the error a released frame reports",
            );
            // Giving it back twice is not an error; it is already back.
            frame.release();
            // And the session works again.
            expect.equal(
              typeof session.renderUpdate(),
              "boolean",
              "the session renders again",
            );
          } finally {
            session.close();
          }
        });
      },
    },
    {
      name: "leaves a caller-owned texture to its owner",
      spec: ["BND-171"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext, hostTexture }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open(EXTENT);
          const owned = hostTexture(EXTENT.width, EXTENT.height);
          const session = map.attachOpenGlBorrowedTexture({
            extent: EXTENT,
            physicalWidth: EXTENT.width,
            physicalHeight: EXTENT.height,
            context: renderContext(),
            texture: owned.texture,
            target: owned.target,
          });
          loadStyle(runtime, map, EMPTY_STYLE);
          session.renderUpdate();

          // Readback belongs to a session that owns its target. This one draws
          // into the caller's texture, so the caller reads it with its own
          // graphics API instead.
          expect.throws(
            () => session.imageInfo(),
            "reading back from a caller-owned target",
          );

          session.close();
          // Closing the session releases what the session made. The texture was
          // never the session's to release, and the host still holds the same
          // one it passed in.
          const after = hostTexture(EXTENT.width, EXTENT.height);
          expect.notEqual(
            after.texture,
            owned.texture,
            "the host's texture name was not handed back out",
          );
        });
      },
    },
    {
      name: "points a caller-owned session at another texture",
      spec: ["BND-175", "BND-176"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext, hostTexture }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open(EXTENT);
          const first = hostTexture(EXTENT.width, EXTENT.height);
          const session = map.attachOpenGlBorrowedTexture({
            extent: EXTENT,
            physicalWidth: EXTENT.width,
            physicalHeight: EXTENT.height,
            context: renderContext(),
            texture: first.texture,
            target: first.target,
          });
          try {
            loadStyle(runtime, map, EMPTY_STYLE);
            session.renderUpdate();

            // The session is kept: only where it draws changes.
            const second = hostTexture(EXTENT.width, EXTENT.height);
            session.setOpenGlBorrowedTexture({
              extent: EXTENT,
              physicalWidth: EXTENT.width,
              physicalHeight: EXTENT.height,
              context: renderContext(),
              texture: second.texture,
              target: second.target,
            });
            expect.equal(session.isClosed, false, "the session was kept");
            expect.equal(
              typeof session.renderUpdate(),
              "boolean",
              "and renders into the new target",
            );
          } finally {
            session.close();
          }

          // A session that owns its target has no host target to replace, and
          // says so rather than quietly doing nothing.
          const owned = map.attachOpenGlOwnedTexture({
            extent: EXTENT,
            context: renderContext(),
          });
          try {
            const spare = hostTexture(EXTENT.width, EXTENT.height);
            const error = expect.throws(
              () =>
                owned.setOpenGlBorrowedTexture({
                  extent: EXTENT,
                  physicalWidth: EXTENT.width,
                  physicalHeight: EXTENT.height,
                  context: renderContext(),
                  texture: spare.texture,
                  target: spare.target,
                }),
              "replacing the target of a session-owned texture",
            );
            expect.equal(error.kind, "unsupported", "the error kind");
          } finally {
            owned.close();
          }
        });
      },
    },
  ],
};
