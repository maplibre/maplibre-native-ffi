/**
 * A projection helper, which converts between screen and geographic space.
 *
 * A projection takes a snapshot of the map's camera and viewport when it is
 * created, and answers from that snapshot afterwards. It outlives the map it
 * came from: the answers depend on the snapshot rather than on the map's current
 * state, so a host may keep one and keep asking.
 */

import type { CameraOptions } from "./camera.ts";
import type { EdgeInsets, LatLng, ScreenPoint } from "./geo.ts";
import { HandleState } from "./internal/handle.ts";
import type { Native } from "./internal/native.ts";
import { attachHandleState, handleStateOf } from "./internal/private.ts";
import { readCameraOptions, writeCameraOptions } from "./internal/struct.ts";
import type { Ptr } from "./internal/transport.ts";
import type { Map } from "./map.ts";
import { EP } from "./raw/entrypoints.ts";

export class MapProjection {
  readonly #state: HandleState;

  private constructor(native: Native, id: bigint) {
    // No parent: after creation this owns a standalone snapshot, so it stays
    // valid once the map it came from has closed.
    this.#state = new HandleState(native, "MapProjection", id);
    this.#state.watchForLeaks(this);
    attachHandleState(this, this.#state);
  }

  /** @internal */
  static create(native: Native, map: Map): MapProjection {
    const id = native.scope((scope) => {
      const outProjection = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_map_projection_create, [
        handleStateOf(map).use("Map.createProjection"),
        outProjection,
      ]);
      return native.memory.view(outProjection, 8).getBigUint64(0, true);
    });
    try {
      return new MapProjection(native, id);
    } catch (error) {
      native.scope((scope) => {
        native.raw(scope, EP.mln_map_projection_destroy, [id]);
      });
      throw error;
    }
  }

  /** The camera this projection answers from. */
  getCamera(): CameraOptions {
    const id = this.#state.use("MapProjection.getCamera");
    const native = this.#state.native;
    return native.scope((scope) => {
      const storage = scope.allocateZeroed(
        native.layout("mln_camera_options").size,
      );
      native.structValue(scope, EP.mln_camera_options_default, storage);
      native.checked(scope, EP.mln_map_projection_get_camera, [id, storage]);
      return readCameraOptions(native, storage);
    });
  }

  /** Moves the camera this projection answers from. */
  setCamera(camera: CameraOptions): void {
    const id = this.#state.use("MapProjection.setCamera");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_map_projection_set_camera, [
        id,
        writeCameraOptions(native, scope, camera),
      ]);
    });
  }

  /** Fits the camera so every coordinate is visible inside the padding. */
  setVisibleCoordinates(
    coordinates: readonly LatLng[],
    padding: EdgeInsets = { top: 0, left: 0, bottom: 0, right: 0 },
  ): void {
    const id = this.#state.use("MapProjection.setVisibleCoordinates");
    const native = this.#state.native;
    native.scope((scope) => {
      const layout = native.layout("mln_lat_lng");
      const array = scope.allocateZeroed(
        Math.max(layout.size * coordinates.length, 1),
        layout.align,
      );
      coordinates.forEach((coordinate, index) => {
        const view = native.memory.view(
          (array + BigInt(index * layout.size)) as Ptr,
          layout.size,
        );
        view.setFloat64(
          layout.fields.latitude!.offset,
          coordinate.latitude,
          true,
        );
        view.setFloat64(
          layout.fields.longitude!.offset,
          coordinate.longitude,
          true,
        );
      });
      const insets = native.layout("mln_edge_insets");
      const paddingStorage = scope.allocateZeroed(insets.size, insets.align);
      const paddingView = native.memory.view(paddingStorage, insets.size);
      paddingView.setFloat64(insets.fields.top!.offset, padding.top, true);
      paddingView.setFloat64(insets.fields.left!.offset, padding.left, true);
      paddingView.setFloat64(
        insets.fields.bottom!.offset,
        padding.bottom,
        true,
      );
      paddingView.setFloat64(insets.fields.right!.offset, padding.right, true);
      native.checked(scope, EP.mln_map_projection_set_visible_coordinates, [
        id,
        array,
        BigInt(coordinates.length),
        paddingStorage,
      ]);
    });
  }

  /** Where a coordinate lands on screen. */
  pixelForLatLng(coordinate: LatLng): ScreenPoint {
    const id = this.#state.use("MapProjection.pixelForLatLng");
    const native = this.#state.native;
    return native.scope((scope) => {
      const coordinateLayout = native.layout("mln_lat_lng");
      const input = scope.allocateZeroed(
        coordinateLayout.size,
        coordinateLayout.align,
      );
      writeLatLng(native, input, coordinate);
      const point = native.layout("mln_screen_point");
      const output = scope.allocateZeroed(point.size, point.align);
      native.checked(scope, EP.mln_map_projection_pixel_for_lat_lng, [
        id,
        input,
        output,
      ]);
      const view = native.memory.view(output, point.size);
      return {
        x: view.getFloat64(point.fields.x!.offset, true),
        y: view.getFloat64(point.fields.y!.offset, true),
      };
    });
  }

  /** Which coordinate a screen point names. */
  latLngForPixel(point: ScreenPoint): LatLng {
    const id = this.#state.use("MapProjection.latLngForPixel");
    const native = this.#state.native;
    return native.scope((scope) => {
      const screen = native.layout("mln_screen_point");
      const input = scope.allocateZeroed(screen.size, screen.align);
      const inputView = native.memory.view(input, screen.size);
      inputView.setFloat64(screen.fields.x!.offset, point.x, true);
      inputView.setFloat64(screen.fields.y!.offset, point.y, true);
      const coordinate = native.layout("mln_lat_lng");
      const output = scope.allocateZeroed(coordinate.size, coordinate.align);
      native.checked(scope, EP.mln_map_projection_lat_lng_for_pixel, [
        id,
        input,
        output,
      ]);
      const view = native.memory.view(output, coordinate.size);
      return {
        latitude: view.getFloat64(coordinate.fields.latitude!.offset, true),
        longitude: view.getFloat64(coordinate.fields.longitude!.offset, true),
      };
    });
  }

  /** Releases the projection. Closing twice succeeds. */
  close(): void {
    const native = this.#state.native;
    this.#state.close((id) => {
      native.scope((scope) => {
        native.checked(scope, EP.mln_map_projection_destroy, [id]);
      });
    });
  }

  get isClosed(): boolean {
    return this.#state.isClosed;
  }
}

function writeLatLng(native: Native, storage: Ptr, coordinate: LatLng): void {
  const layout = native.layout("mln_lat_lng");
  const view = native.memory.view(storage, layout.size);
  view.setFloat64(layout.fields.latitude!.offset, coordinate.latitude, true);
  view.setFloat64(layout.fields.longitude!.offset, coordinate.longitude, true);
}
