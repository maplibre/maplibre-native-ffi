import type { BrowserMapModule, Viewport } from "./types";
import { setCanvasPhysicalSize } from "./viewport";

const textureVertexShader = `
struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
};

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
  var positions = array<vec2f, 3>(
    vec2f(-1.0, -1.0),
    vec2f(3.0, -1.0),
    vec2f(-1.0, 3.0),
  );
  var uvs = array<vec2f, 3>(
    vec2f(0.0, 1.0),
    vec2f(2.0, 1.0),
    vec2f(0.0, -1.0),
  );

  var output: VertexOutput;
  output.position = vec4f(positions[vertex_index], 0.0, 1.0);
  output.uv = uvs[vertex_index];
  return output;
}
`;

const textureFragmentShader = `
@group(0) @binding(0) var map_sampler: sampler;
@group(0) @binding(1) var map_texture: texture_2d<f32>;

@fragment
fn fragment_main(@location(0) uv: vec2f) -> @location(0) vec4f {
  let color = textureSample(map_texture, map_sampler, uv);
  // The browser host compositor presents an opaque map viewport. Forwarding the
  // sampled alpha through the WebGPU canvas path currently presents black in
  // this browser path even though the sampled RGB data is valid.
  return vec4f(color.rgb, 1.0);
}
`;

export class WebGPUDeviceHost {
  static async create(module: BrowserMapModule): Promise<WebGPUDeviceHost> {
    const adapter = await navigator.gpu.requestAdapter();
    if (!adapter) {
      throw new Error("No WebGPU adapter found");
    }
    const device = await adapter.requestDevice();
    device.addEventListener("uncapturederror", (event) => {
      console.error(`WebGPU uncaptured error: ${event.error.message}`);
    });
    void device.lost.then((info) => {
      console.error(`WebGPU device lost: ${info.reason}: ${info.message}`);
    });
    return new WebGPUDeviceHost(module, device);
  }

  readonly device: GPUDevice;
  readonly devicePtr: number;

  private constructor(module: BrowserMapModule, device: GPUDevice) {
    this.device = device;
    this.devicePtr = module.webgpu.importJsDevice(device);
  }
}

export class WebGPUTextureHost {
  private readonly module: BrowserMapModule;
  private readonly canvas: HTMLCanvasElement;
  private readonly device: GPUDevice;
  private readonly canvasContext: GPUCanvasContext;
  private readonly presentationFormat: GPUTextureFormat;
  private readonly sampler: GPUSampler;
  private readonly bindGroupLayout: GPUBindGroupLayout;
  private readonly pipeline: GPURenderPipeline;
  private presentedTexturePtr = 0;
  private presentedTextureView: GPUTextureView | null = null;
  private presentedBindGroup: GPUBindGroup | null = null;

  constructor(
    module: BrowserMapModule,
    canvas: HTMLCanvasElement,
    deviceHost: WebGPUDeviceHost,
    initialViewport: Viewport,
  ) {
    this.module = module;
    this.canvas = canvas;
    this.device = deviceHost.device;
    const canvasContext = canvas.getContext("webgpu");
    if (!canvasContext) {
      throw new Error("No WebGPU canvas context found");
    }
    this.canvasContext = canvasContext;
    this.presentationFormat = navigator.gpu.getPreferredCanvasFormat();
    this.configure(initialViewport);

    this.sampler = this.createSampler();
    this.bindGroupLayout = this.createBindGroupLayout();
    this.pipeline = this.createPipeline();
  }

  resize(viewport: Viewport): number {
    const status = this.module._mln_browser_map_resize(
      viewport.width,
      viewport.height,
      viewport.scale,
    );
    if (status !== 0) {
      return status;
    }
    this.configure(viewport);
    this.presentedTexturePtr = 0;
    this.presentedTextureView = null;
    this.presentedBindGroup = null;
    return 0;
  }

  presentOwnedTexture(texturePtr: number): void {
    this.presentTexture(texturePtr);
  }

  private configure(viewport: Viewport): void {
    setCanvasPhysicalSize(this.canvas, viewport);
    this.canvasContext.configure({
      device: this.device,
      format: this.presentationFormat,
      alphaMode: "opaque",
    });
  }

  private createSampler(): GPUSampler {
    return this.device.createSampler({
      magFilter: "linear",
      minFilter: "linear",
    });
  }

  private createBindGroupLayout(): GPUBindGroupLayout {
    return this.device.createBindGroupLayout({
      entries: [
        {
          binding: 0,
          visibility: GPUShaderStage.FRAGMENT,
          sampler: { type: "filtering" },
        },
        {
          binding: 1,
          visibility: GPUShaderStage.FRAGMENT,
          texture: { sampleType: "float", viewDimension: "2d" },
        },
      ],
    });
  }

  private createPipeline(): GPURenderPipeline {
    return this.device.createRenderPipeline({
      layout: this.device.createPipelineLayout({
        bindGroupLayouts: [this.bindGroupLayout],
      }),
      vertex: {
        module: this.device.createShaderModule({ code: textureVertexShader }),
        entryPoint: "vertex_main",
      },
      fragment: {
        module: this.device.createShaderModule({
          code: textureFragmentShader,
        }),
        entryPoint: "fragment_main",
        targets: [{ format: this.presentationFormat }],
      },
      primitive: { topology: "triangle-list" },
    });
  }

  private presentTexture(texturePtr: number): void {
    if (texturePtr !== this.presentedTexturePtr) {
      const texture = this.module.webgpu.getJsObject(texturePtr);
      this.presentedTextureView = texture.createView();
      this.presentedBindGroup = this.device.createBindGroup({
        layout: this.bindGroupLayout,
        entries: [
          { binding: 0, resource: this.sampler },
          { binding: 1, resource: this.presentedTextureView },
        ],
      });
      this.presentedTexturePtr = texturePtr;
    }
    if (!this.presentedBindGroup) {
      return;
    }

    const outputView = this.canvasContext.getCurrentTexture().createView();
    const encoder = this.device.createCommandEncoder();
    const pass = encoder.beginRenderPass({
      colorAttachments: [
        {
          view: outputView,
          clearValue: { r: 0.04, g: 0.06, b: 0.1, a: 1 },
          loadOp: "clear",
          storeOp: "store",
        },
      ],
    });
    pass.setPipeline(this.pipeline);
    pass.setBindGroup(0, this.presentedBindGroup);
    pass.draw(3);
    pass.end();
    this.device.queue.submit([encoder.finish()]);
  }
}
