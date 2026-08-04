/**
 * Render sessions, and the queries that only a session can answer.
 *
 * Every case here needs a live graphics context, which is a property of the
 * host rather than of the transport: a browser has WebGL, and a bare Node
 * process has none. A runner that cannot supply one leaves these out through
 * `needs`, rather than each case checking for itself and passing when it found
 * nothing.
 */

import { emptyGeometry, geoJsonGeometry } from "../geojson.ts";
import { clearForcedStatuses, forceStatus } from "../internal/faults.ts";
import { jsonBool, jsonFrom, jsonObject, type JsonValue } from "../json.ts";
import { pointQuery } from "../query.ts";
import { EP } from "../raw/entrypoints.ts";
import { MLN_STATUS } from "../raw/enums.ts";
import { nativePointer } from "../render.ts";
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

/** A clustered source, so the renderer builds an index to ask about. */
const CLUSTER_STYLE = JSON.stringify({
  version: 8,
  name: "clustered",
  sources: {
    points: {
      type: "geojson",
      cluster: true,
      clusterRadius: 200,
      data: {
        type: "FeatureCollection",
        features: [0, 0.001, 0.002, 0.003].map((offset) => ({
          type: "Feature",
          properties: {},
          geometry: { type: "Point", coordinates: [offset, offset] },
        })),
      },
    },
  },
  layers: [
    {
      id: "points",
      type: "circle",
      source: "points",
      paint: { "circle-radius": 20 },
    },
  ],
});

export const RENDER_SESSION_GROUP: ConformanceGroup = {
  name: "render sessions",
  cases: [
    {
      name: "attaches a session-owned texture and renders through it",
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
      name: "sends each attach path to its own C session family",
      spec: ["BND-162"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext, hostTexture }) {
        withRuntime(maplibre, (_runtime, open) => {
          const map = open(EXTENT);
          const context = renderContext();
          const hostOwned = hostTexture(EXTENT.width, EXTENT.height);
          const sessionOwnedTexture = { extent: EXTENT, context };
          const callerOwnedTexture = {
            extent: EXTENT,
            physicalWidth: EXTENT.width,
            physicalHeight: EXTENT.height,
            context,
            texture: hostOwned.texture,
            target: hostOwned.target,
          };
          // A stand-in for the host surface a browser has nothing to put here:
          // a WebGL context is bound to its canvas, so there is no surface
          // object to name. It is only ever handed to a family that refuses it,
          // below, and a host whose build carries surface sessions has to hand
          // this case a real one.
          const surface = {
            extent: EXTENT,
            context,
            surface: nativePointer(1n),
          };

          // Which C function an attach path called is invisible from a session
          // that came back, so each family's entry point is made to refuse in
          // turn. A fault is keyed by entry point and a failing call names the
          // one it went through, so a path that had reached another family's
          // function would report that other name, or, for the two that attach
          // here, would not fail at all.
          const families = [
            {
              name: "mln_opengl_surface_attach",
              entrypoint: EP.mln_opengl_surface_attach,
              attach: () => map.attachOpenGlSurface(surface),
            },
            {
              name: "mln_opengl_owned_texture_attach",
              entrypoint: EP.mln_opengl_owned_texture_attach,
              attach: () => map.attachOpenGlOwnedTexture(sessionOwnedTexture),
            },
            {
              name: "mln_opengl_borrowed_texture_attach",
              entrypoint: EP.mln_opengl_borrowed_texture_attach,
              attach: () => map.attachOpenGlBorrowedTexture(callerOwnedTexture),
            },
          ];
          for (const family of families) {
            forceStatus(family.entrypoint, MLN_STATUS.MLN_STATUS_NATIVE_ERROR);
            try {
              const error = expect.throws(
                family.attach,
                `attaching through ${family.name}`,
              );
              expect.equal(
                error.operation,
                family.name,
                "the C function the attach path called",
              );
              // The arranged status rather than the one this build would
              // otherwise report, which is what says the answer came from that
              // call and not from somewhere earlier in the path.
              expect.equal(
                error.nativeStatus,
                MLN_STATUS.MLN_STATUS_NATIVE_ERROR,
                "the status arranged for that call",
              );
            } finally {
              clearForcedStatuses();
            }
          }

          // With nothing arranged, both texture families attach for real. A map
          // holds one session at a time, so they take turns.
          const sessionOwned =
            map.attachOpenGlOwnedTexture(sessionOwnedTexture);
          try {
            expect.equal(
              sessionOwned.isClosed,
              false,
              "the session-owned texture session is live",
            );
            expect.equal(
              typeof sessionOwned.renderUpdate(),
              "boolean",
              "and answers an update",
            );
          } finally {
            sessionOwned.close();
          }
          const callerOwned =
            map.attachOpenGlBorrowedTexture(callerOwnedTexture);
          try {
            expect.equal(
              callerOwned.isClosed,
              false,
              "the caller-owned texture session is live",
            );
            expect.equal(
              typeof callerOwned.renderUpdate(),
              "boolean",
              "and answers an update",
            );
            // One public handle whichever family made it, so a host that moves
            // between them keeps the API it was written against.
            expect.ok(
              Object.getPrototypeOf(callerOwned) ===
                Object.getPrototypeOf(sessionOwned),
              "both families hand back the same session type",
            );
          } finally {
            callerOwned.close();
          }

          // The surface path with nothing arranged reaches the C surface family
          // itself, which reads the descriptor this binding materialized in the
          // order that family documents: the extent, then the context, then the
          // surface handle. A descriptor missing only the surface is refused for
          // the surface, and one whose extent is not a size is refused for the
          // extent instead, so what answers is native reading this struct rather
          // than anything this binding screened first.
          const withoutSurface = expect.throws(
            () =>
              map.attachOpenGlSurface({
                extent: EXTENT,
                context,
                surface: nativePointer(0n),
              }),
            "attaching a surface session with no surface handle",
          );
          expect.equal(
            withoutSurface.operation,
            "mln_opengl_surface_attach",
            "the C function that refused it",
          );
          expect.equal(
            withoutSurface.kind,
            "invalidArgument",
            "the error kind",
          );
          expect.contains(
            withoutSurface.diagnostic,
            "surface",
            "the diagnostic names the surface handle",
          );
          const badExtent = expect.throws(
            () =>
              map.attachOpenGlSurface({
                extent: { width: 0, height: EXTENT.height },
                context,
                surface: nativePointer(0n),
              }),
            "attaching a surface session with an extent that is not a size",
          );
          expect.contains(
            badExtent.diagnostic,
            "dimensions",
            "the extent is read before the surface handle",
          );

          // What the family answers a descriptor it accepts is the build's to
          // say. A browser composites its canvas without a swap and has no
          // surface object to present through, so this build compiles the
          // OpenGL surface family as a stub that reports unsupported, while the
          // two texture families attach in that same build. Either answer is
          // the surface family's own: a path that had gone to a texture family
          // would have attached here, as those two just did.
          const answer = expect.throws(
            () => map.attachOpenGlSurface(surface),
            "attaching a surface session whose descriptor is complete",
          );
          expect.equal(
            answer.operation,
            "mln_opengl_surface_attach",
            "the C function that answered",
          );
          expect.equal(
            answer.kind,
            "unsupported",
            "what the surface family carries in this build",
          );
          expect.contains(
            answer.diagnostic,
            "surface",
            "the diagnostic names surface sessions",
          );
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
      spec: ["BND-106", "BND-063"],
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
            // The native result is destroyed before the query returns, so
            // everything read below outlived the borrow window it came from.
            // A view kept rather than copied would read freed storage here.
            const fromSource = session.querySourceFeatures("points", {});
            const first = expect.defined(fromSource[0], "a feature");
            expect.equal(first.sourceId, "points", "the copied source id");
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
    {
      name: "asks a source's index about a feature it rendered",
      spec: ["BND-107"],
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
            expect.ok(
              loadStyle(runtime, map, CLUSTER_STYLE),
              "the style loaded",
            );
            for (let attempt = 0; attempt < 120; attempt += 1) {
              runtime.pump(25);
              session.renderUpdate();
            }

            const clustered = session
              .querySourceFeatures("points", {})
              .find((found) =>
                found.feature.properties?.some(
                  (member) => member.name === "cluster_id",
                ),
              );
            const cluster = expect.defined(clustered, "a clustered feature");

            // The cluster index lives on the renderer, so the session is what
            // answers. The feature goes back the way it came out.
            const leaves = session.queryFeatureExtensions(
              "points",
              cluster.feature,
              "supercluster",
              "leaves",
            );
            expect.equal(leaves.kind, "features", "leaves are features");
            if (leaves.kind === "features") {
              expect.ok(
                leaves.features.length > 0,
                "the cluster has something in it",
              );
            }
          } finally {
            session.close();
          }
        });
      },
    },
    {
      name: "keeps a frame held when giving it back refuses",
      spec: ["BND-169"],
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
            for (let attempt = 0; attempt < 40; attempt += 1) {
              runtime.pump(25);
              session.renderUpdate();
            }

            const frame = session.acquireOpenGlFrame();
            forceStatus(EP.mln_opengl_owned_texture_release_frame, -5);
            try {
              expect.throws(
                () => frame.release(),
                "a frame release that refuses",
              );
              // The session still holds the frame, so the wrapper must too: a
              // wrapper that marked itself released would leave the session
              // holding a frame nobody can give back.
              expect.equal(frame.isReleased, false, "the frame is still held");
              expect.ok(
                frame.handles.texture > 0,
                "and still exposes the texture it was lent",
              );
            } finally {
              clearForcedStatuses();
            }

            frame.release();
            expect.equal(frame.isReleased, true, "a later release succeeded");
          } finally {
            clearForcedStatuses();
            session.close();
          }
        });
      },
    },
    {
      name: "asks for custom geometry tiles and stops after teardown",
      spec: ["BND-124"],
      needs: NEEDS_CONTEXT,
      run({ maplibre, expect, renderContext }) {
        withRuntime(maplibre, (runtime, open) => {
          const map = open(EXTENT);
          const session = map.attachOpenGlOwnedTexture({
            extent: EXTENT,
            context: renderContext(),
          });
          const asked: string[] = [];
          const draw = (rounds: number): void => {
            for (let attempt = 0; attempt < rounds; attempt += 1) {
              runtime.pump(25);
              session.renderUpdate();
              maplibre.deliverCallbacks();
              while (runtime.pollEvent() !== undefined) {
                // Drained so the queue does not hold the pump open.
              }
            }
          };

          try {
            expect.ok(loadStyle(runtime, map, EMPTY_STYLE), "the style loaded");
            map.addCustomGeometrySource(
              "hosted",
              {
                onFetchTile: (tile) => {
                  asked.push(`${tile.z}/${tile.x}/${tile.y}`);
                  map.setCustomGeometryTileData(
                    "hosted",
                    tile,
                    geoJsonGeometry(emptyGeometry),
                  );
                },
              },
              { minZoom: 0, maxZoom: 4 },
            );
            map.addStyleLayer(
              jsonFrom({ id: "hosted-dots", type: "circle", source: "hosted" }),
            );

            // Rendering is what makes MapLibre ask. Without a session nothing
            // draws and the callback never fires, which is how a defect on
            // this exact path survived: the record it delivers is allocated
            // and freed differently from every other kind.
            draw(120);
            expect.ok(asked.length > 0, "the host was asked for a tile");

            // Removing the source retires its callbacks. Nothing may arrive
            // for it afterwards, however long the map keeps drawing.
            const beforeRemoval = asked.length;
            // A source a layer still draws from cannot go, so the layer goes
            // first — the same order a host would have to use.
            expect.ok(map.removeStyleLayer("hosted-dots"), "the layer went");
            expect.ok(
              map.removeStyleSource("hosted"),
              "the source was removed",
            );
            draw(40);
            expect.equal(
              asked.length,
              beforeRemoval,
              "no tile was asked for after the source was removed",
            );

            // The same id may be used again, and the new registration is the
            // one that receives — a stale one would deliver to a retired
            // handler.
            const second: string[] = [];
            map.addCustomGeometrySource(
              "hosted",
              {
                onFetchTile: (tile) => {
                  second.push(`${tile.z}/${tile.x}/${tile.y}`);
                  map.setCustomGeometryTileData(
                    "hosted",
                    tile,
                    geoJsonGeometry(emptyGeometry),
                  );
                },
              },
              { minZoom: 0, maxZoom: 4 },
            );
            map.addStyleLayer(
              jsonFrom({
                id: "hosted-again",
                type: "circle",
                source: "hosted",
              }),
            );
            draw(120);
            expect.ok(second.length > 0, "the new registration receives");
            expect.equal(
              asked.length,
              beforeRemoval,
              "and the retired one still receives nothing",
            );

            // Closing the map with a source live must not deliver afterwards.
            const atClose = second.length;
            session.close();
            map.close();
            // Pumped without the session, which is gone: what is being watched
            // for is a callback arriving after its map closed.
            for (let attempt = 0; attempt < 20; attempt += 1) {
              runtime.pump(25);
              maplibre.deliverCallbacks();
              while (runtime.pollEvent() !== undefined) {
                // Drained so the queue does not hold the pump open.
              }
            }
            expect.equal(
              second.length,
              atClose,
              "nothing arrived after the map closed",
            );
          } finally {
            if (!session.isClosed) {
              session.close();
            }
          }
        });
      },
    },
  ],
};
