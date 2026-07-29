"use strict";

const assert = require("node:assert/strict");
const test = require("node:test");

const {
  NativePointer,
  RuntimeHandle,
  supportedOpenGLContextProviders,
  supportedRenderBackends,
} = require("..");
const { NativeTestRenderContext } = require("../index.js");

const WIDTH = 32;
const HEIGHT = 16;
const STYLE_JSON =
  '{"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#123456"}}]}';

/**
 * @param {bigint} address
 * @param {string} field
 */
function pointer(address, field) {
  assert.equal(typeof address, "bigint", `${field} address must be a bigint`);
  assert.notEqual(address, 0n, `${field} address must be non-null`);
  return NativePointer.unsafeFromAddress(address);
}

/**
 * @param {import("..").MapHandle} map
 * @param {any} fixture
 * @returns {import("..").RenderSessionHandle}
 */
function attachOwnedTexture(map, fixture) {
  const extent = { width: WIDTH, height: HEIGHT, scaleFactor: 1 };
  switch (fixture.backend) {
    case "metal":
      assert.equal(supportedRenderBackends().metal, true);
      return map.attachMetalOwnedTexture({
        extent,
        context: { device: pointer(fixture.deviceAddress, "Metal device") },
      });
    case "vulkan":
      assert.equal(supportedRenderBackends().vulkan, true);
      return map.attachVulkanOwnedTexture({
        extent,
        context: {
          instance: pointer(fixture.instanceAddress, "Vulkan instance"),
          physicalDevice: pointer(
            fixture.physicalDeviceAddress,
            "Vulkan physical device",
          ),
          device: pointer(fixture.deviceAddress, "Vulkan device"),
          graphicsQueue: pointer(
            fixture.graphicsQueueAddress,
            "Vulkan graphics queue",
          ),
          graphicsQueueFamilyIndex: fixture.graphicsQueueFamilyIndex,
          getInstanceProcAddr: pointer(
            fixture.getInstanceProcAddrAddress,
            "vkGetInstanceProcAddr",
          ),
          getDeviceProcAddr: pointer(
            fixture.getDeviceProcAddrAddress,
            "vkGetDeviceProcAddr",
          ),
        },
      });
    case "egl":
      assert.equal(supportedRenderBackends().opengl, true);
      assert.equal(supportedOpenGLContextProviders().egl, true);
      return map.attachOpenGLOwnedTexture({
        extent,
        context: {
          platform: "egl",
          display: pointer(fixture.displayAddress, "EGL display"),
          config: pointer(fixture.configAddress, "EGL config"),
          shareContext: pointer(
            fixture.shareContextAddress,
            "EGL share context",
          ),
        },
      });
    case "wgl":
      assert.equal(supportedRenderBackends().opengl, true);
      assert.equal(supportedOpenGLContextProviders().wgl, true);
      return map.attachOpenGLOwnedTexture({
        extent,
        context: {
          platform: "wgl",
          deviceContext: pointer(
            fixture.deviceContextAddress,
            "WGL device context",
          ),
          shareContext: pointer(
            fixture.shareContextAddress,
            "WGL share context",
          ),
          getProcAddress: pointer(
            fixture.getProcAddressAddress,
            "wglGetProcAddress",
          ),
        },
      });
    default:
      assert.fail(`unexpected configured test backend: ${fixture.backend}`);
  }
}

/**
 * @param {import("..").RenderSessionHandle} session
 * @param {string} backend
 */
function acquireOwnedTextureFrame(session, backend) {
  switch (backend) {
    case "metal":
      return session.acquireMetalOwnedTextureFrame();
    case "vulkan":
      return session.acquireVulkanOwnedTextureFrame();
    case "egl":
    case "wgl":
      return session.acquireOpenGLOwnedTextureFrame();
    default:
      assert.fail(`unexpected configured test backend: ${backend}`);
  }
}

/**
 * @param {import("..").RuntimeHandle} runtime
 * @param {import("..").MapHandle} map
 * @param {import("..").RenderSessionHandle} session
 */
async function renderStillImage(runtime, map, session) {
  let rendered = false;
  let finished = false;
  const deadline = Date.now() + 30_000;
  while (!finished && Date.now() < deadline) {
    runtime.pump();
    let event;
    while ((event = runtime.pollEvent()) != null) {
      if (event.sourceMap !== map) {
        continue;
      }
      switch (event.eventType) {
        case "map-render-update-available":
          rendered = session.renderUpdate() || rendered;
          break;
        case "map-still-image-finished":
          finished = true;
          break;
        case "map-loading-failed":
        case "map-render-error":
        case "map-still-image-failed":
          assert.fail(
            `${event.eventType}: ${event.message || "no native diagnostic"}`,
          );
      }
    }
    if (!finished) {
      await new Promise((resolve) => setTimeout(resolve, 5));
    }
  }
  assert.equal(finished, true, "static map render timed out");
  assert.equal(rendered, true, "no render update produced a frame");
}

test("configured backend renders and reads an owned texture through public JS", async () => {
  const nativeFixture = NativeTestRenderContext.create();
  const runtime = new RuntimeHandle({ cachePath: ":memory:" });
  const map = runtime.createMap({
    width: WIDTH,
    height: HEIGHT,
    scaleFactor: 1,
    mapMode: "static",
  });
  let session;
  let frame;

  try {
    const fixture = nativeFixture.descriptor();
    session = attachOwnedTexture(map, fixture);
    map.setStyleJson(STYLE_JSON);
    map.requestStillImage();
    await renderStillImage(runtime, map, session);

    const pixels = new Uint8Array(WIDTH * HEIGHT * 4);
    const image = session.readPremultipliedRgba8Into(pixels);
    assert.deepEqual(image, {
      width: WIDTH,
      height: HEIGHT,
      stride: WIDTH * 4,
      byteLength: pixels.byteLength,
    });
    assert.equal(
      pixels.some((channel) => channel !== 0),
      true,
      "render readback must contain non-zero pixel data",
    );

    frame = acquireOwnedTextureFrame(session, fixture.backend);
    assert.equal(frame.width, WIDTH);
    assert.equal(frame.height, HEIGHT);
    assert.equal(frame.scaleFactor, 1);
    assert.equal(frame.closed, false);
    const frameDetails = /** @type {any} */ (frame);
    if (fixture.backend === "metal") {
      assert.equal(frameDetails.texture.isNull, false);
      assert.equal(frameDetails.device.isNull, false);
    } else if (fixture.backend === "vulkan") {
      assert.equal(frameDetails.image.isNull, false);
      assert.equal(frameDetails.imageView.isNull, false);
      assert.equal(frameDetails.device.isNull, false);
    } else {
      assert.notEqual(frameDetails.texture, 0);
    }
    frame.close();
    frame.close();
    assert.equal(frame.closed, true);
  } finally {
    frame?.close();
    session?.close();
    map.close();
    runtime.close();
    nativeFixture.close();
    nativeFixture.close();
    assert.equal(nativeFixture.closed, true);
  }
});
