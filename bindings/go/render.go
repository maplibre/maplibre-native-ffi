package maplibre

/*
#include "maplibre_native_c.h"
#include "internal/cgo_shim.h"
*/
import "C"

import (
	"math"
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

// RenderBackendMask preserves the render backend bits reported by the native
// library. Unknown future bits remain in the mask.
type RenderBackendMask uint32

const (
	RenderBackendMetal  RenderBackendMask = RenderBackendMask(C.MLN_RENDER_BACKEND_FLAG_METAL)
	RenderBackendVulkan RenderBackendMask = RenderBackendMask(C.MLN_RENDER_BACKEND_FLAG_VULKAN)
	RenderBackendOpenGL RenderBackendMask = RenderBackendMask(C.MLN_RENDER_BACKEND_FLAG_OPENGL)
	RenderBackendWebGPU RenderBackendMask = RenderBackendMask(C.MLN_RENDER_BACKEND_FLAG_WEBGPU)
)

// OpenGLContextProviderMask preserves the OpenGL context provider bits reported
// by the native library. Unknown future bits remain in the mask.
type OpenGLContextProviderMask uint32

const (
	OpenGLContextProviderWGL   OpenGLContextProviderMask = OpenGLContextProviderMask(C.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL)
	OpenGLContextProviderEGL   OpenGLContextProviderMask = OpenGLContextProviderMask(C.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL)
	OpenGLContextProviderWebGL OpenGLContextProviderMask = OpenGLContextProviderMask(C.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WEBGL)
)

// Has reports whether all provider bits in provider are present.
func (mask OpenGLContextProviderMask) Has(provider OpenGLContextProviderMask) bool {
	return mask&provider == provider
}

// Has reports whether all backend bits in backend are present.
func (mask RenderBackendMask) Has(backend RenderBackendMask) bool {
	return mask&backend == backend
}

// RenderResult is the outcome of a successful render-update call. This is an
// open domain: a value may have no named constant here, so a switch over it
// needs a default case. Unknown values keep their raw value.
type RenderResult uint32

const (
	// RenderResultRendered means the call rendered a frame into the render
	// target.
	RenderResultRendered RenderResult = RenderResult(C.MLN_RENDER_RESULT_RENDERED)
	// RenderResultNoUpdate means the call produced no frame. Wait for a
	// render-update-available event.
	RenderResultNoUpdate RenderResult = RenderResult(C.MLN_RENDER_RESULT_NO_UPDATE)
	// RenderResultSizePending means the map has not applied the session's
	// current size yet. Wait for the next render-update-available event.
	RenderResultSizePending RenderResult = RenderResult(C.MLN_RENDER_RESULT_SIZE_PENDING)
	// RenderResultTargetNotReady means the render target had no frame to draw
	// into. Wait for a host event that changes the render target, or back off
	// and retry.
	RenderResultTargetNotReady RenderResult = RenderResult(C.MLN_RENDER_RESULT_TARGET_NOT_READY)
	RenderResultSuperseded     RenderResult = RenderResult(C.MLN_RENDER_RESULT_SUPERSEDED)
	RenderResultDeadlineMissed RenderResult = RenderResult(C.MLN_RENDER_RESULT_DEADLINE_MISSED)
)

// OpenGLContextOwnership names how a session's OpenGL context relates to its
// driver thread and host graphics state.
type OpenGLContextOwnership uint32

const (
	// OpenGLContextOwnershipShared leaves the thread as the session found it:
	// every render makes the session context current and restores whatever was
	// current before. The session context joins the host share group named by
	// the descriptor, so a host may hand the session a texture and sample it
	// from its own context.
	OpenGLContextOwnershipShared OpenGLContextOwnership = OpenGLContextOwnership(C.MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED)
	// OpenGLContextOwnershipDedicated gives the session its driver thread's
	// context. It keeps the context current between renders and joins no share
	// group. The driver may be a native core worker or a dedicated host thread.
	OpenGLContextOwnershipDedicated OpenGLContextOwnership = OpenGLContextOwnership(C.MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED)
)

// OpenGLClientAPI names the OpenGL client API a dedicated EGL session creates
// its context for.
type OpenGLClientAPI uint32

const (
	// OpenGLClientAPIUnspecified names no client API.
	OpenGLClientAPIUnspecified OpenGLClientAPI = OpenGLClientAPI(C.MLN_OPENGL_CLIENT_API_UNSPECIFIED)
	// OpenGLClientAPIGL is desktop OpenGL, as EGL_OPENGL_API names it.
	OpenGLClientAPIGL OpenGLClientAPI = OpenGLClientAPI(C.MLN_OPENGL_CLIENT_API_GL)
	// OpenGLClientAPIGLES is OpenGL ES, as EGL_OPENGL_ES_API names it.
	OpenGLClientAPIGLES OpenGLClientAPI = OpenGLClientAPI(C.MLN_OPENGL_CLIENT_API_GLES)
)

// NativePointer is a borrowed opaque backend-native address. It grants no
// memory access and transfers no ownership.
type NativePointer uintptr

// VulkanHandle is the 64-bit bit pattern of a borrowed Vulkan
// non-dispatchable handle. It grants no memory access and transfers no
// ownership. Zero represents VK_NULL_HANDLE.
type VulkanHandle uint64

// RenderTargetExtent is a logical render target extent in UI pixels.
type RenderTargetExtent struct {
	// Width is the logical width in UI pixels.
	Width uint32
	// Height is the logical height in UI pixels.
	Height uint32
	// ScaleFactor is the UI-to-device pixel scale. It is fixed when a session
	// attaches, so RenderSessionHandle.Resize passes the attaching value.
	ScaleFactor float64
}

// TextureImageInfo describes CPU readback image metadata.
type TextureImageInfo struct {
	Width      uint32
	Height     uint32
	Stride     uint32
	ByteLength uint64
}

// MetalContextDescriptor contains Metal backend context handles.
type MetalContextDescriptor struct {
	Device NativePointer
}

// VulkanContextDescriptor contains Vulkan backend context handles.
type VulkanContextDescriptor struct {
	Instance                 NativePointer
	PhysicalDevice           NativePointer
	Device                   NativePointer
	GraphicsQueue            NativePointer
	GraphicsQueueFamilyIndex uint32
	GetInstanceProcAddr      NativePointer
	GetDeviceProcAddr        NativePointer
}

// WebGPUContextDescriptor contains WebGPU backend context handles.
type WebGPUContextDescriptor struct {
	Instance NativePointer
	Device   NativePointer
	Queue    NativePointer
}

// WebGPUSurfaceDescriptor describes a WebGPU surface target.
type WebGPUSurfaceDescriptor struct {
	Extent  RenderTargetExtent
	Context WebGPUContextDescriptor
	Surface NativePointer
	Format  uint32
}

// WebGPUOwnedTextureDescriptor describes a session-owned WebGPU texture ring.
type WebGPUOwnedTextureDescriptor struct {
	Extent  RenderTargetExtent
	Context WebGPUContextDescriptor
}

// WebGPUBorrowedTextureDescriptor describes a caller-owned WebGPU texture.
type WebGPUBorrowedTextureDescriptor struct {
	Extent         RenderTargetExtent
	PhysicalWidth  uint32
	PhysicalHeight uint32
	Context        WebGPUContextDescriptor
	Texture        NativePointer
	TextureView    NativePointer
	Format         uint32
}

// TextureReadback owns premultiplied RGBA8 bytes and its image layout.
type TextureReadback struct {
	Data []byte
	Info TextureImageInfo
}

// MetalSurfaceDescriptor describes a Metal-backed surface render target. The
// session retains the layer and optional device while attached.
type MetalSurfaceDescriptor struct {
	Extent  RenderTargetExtent
	Context MetalContextDescriptor
	Layer   NativePointer
}

// VulkanSurfaceDescriptor describes a Vulkan-backed surface render target.
// Vulkan handles are borrowed and must remain valid until detach or session
// close. The device must support swapchain presentation on the graphics queue
// family for Surface.
type VulkanSurfaceDescriptor struct {
	Extent  RenderTargetExtent
	Context VulkanContextDescriptor
	Surface VulkanHandle
}

// WGLContextDescriptor contains WGL context provider data for OpenGL render targets.
type WGLContextDescriptor struct {
	DeviceContext NativePointer
	// ShareContext is the HGLRC whose share group the session context joins.
	// Required under shared ownership. A dedicated session joins no share
	// group, so it must be zero there.
	ShareContext   NativePointer
	GetProcAddress NativePointer
}

// EGLContextDescriptor contains EGL context provider data for OpenGL render targets.
type EGLContextDescriptor struct {
	Display NativePointer
	Config  NativePointer
	// ShareContext is the EGLContext whose share group the session context
	// joins. Required under shared ownership, where the session also takes its
	// client API from this context. A dedicated session joins no share group,
	// so it must be zero there and names ClientAPI instead.
	ShareContext NativePointer
	// ClientAPI is the client API the session creates its context for.
	// Required under dedicated ownership. A shared session queries
	// ShareContext for it, so this is ignored there.
	ClientAPI      OpenGLClientAPI
	GetProcAddress NativePointer
}

// WebGLContextKind selects an existing agent-local context or a transferred canvas.
type WebGLContextKind uint32

const (
	WebGLContextExisting          WebGLContextKind = WebGLContextKind(C.MLN_WEBGL_CONTEXT_EXISTING)
	WebGLContextTransferredCanvas WebGLContextKind = WebGLContextKind(C.MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS)
)

// WebGLContextDescriptor describes browser WebGL context placement.
type WebGLContextDescriptor struct {
	Kind           WebGLContextKind
	Context        int32
	CanvasSelector string
}

// OpenGLContextDescriptor contains one OpenGL platform context provider.
type OpenGLContextDescriptor struct {
	WGL   *WGLContextDescriptor
	EGL   *EGLContextDescriptor
	WebGL *WebGLContextDescriptor
	// A private EGL owned texture and a transferred canvas are dedicated to
	// their core worker.
	Ownership OpenGLContextOwnership
}

func (context OpenGLContextDescriptor) validate() error {
	count := 0
	if context.WGL != nil {
		count++
	}
	if context.EGL != nil {
		count++
	}
	if context.WebGL != nil {
		count++
	}
	if count != 1 {
		return newBindingError(ErrInvalidArgument, "OpenGL context descriptor must specify exactly one platform")
	}
	return nil
}

// OpenGLSurfaceDescriptor describes an OpenGL-backed surface render target.
type OpenGLSurfaceDescriptor struct {
	Extent  RenderTargetExtent
	Context OpenGLContextDescriptor
	Surface NativePointer
}

// MetalOwnedTextureDescriptor describes a Metal session-owned texture render target.
type MetalOwnedTextureDescriptor struct {
	Extent  RenderTargetExtent
	Context MetalContextDescriptor
}

// MetalBorrowedTextureDescriptor describes a Metal caller-owned texture render
// target. The caller keeps Texture valid until detach or session close and
// synchronizes all use outside this session.
type MetalBorrowedTextureDescriptor struct {
	Extent RenderTargetExtent
	// PhysicalWidth and PhysicalHeight are the texture's size in device pixels,
	// stated rather than derived from Extent because its owner sizes it.
	PhysicalWidth  uint32
	PhysicalHeight uint32
	Texture        NativePointer
}

// VulkanOwnedTextureDescriptor describes a Vulkan session-owned texture render
// target. Vulkan context handles are borrowed and must remain valid until detach
// or session close.
type VulkanOwnedTextureDescriptor struct {
	Extent  RenderTargetExtent
	Context VulkanContextDescriptor
}

// VulkanBorrowedTextureDescriptor describes a Vulkan caller-owned texture render
// target. The caller keeps Image and ImageView valid until detach or session
// close, manages queue-family ownership, makes the image available in
// InitialLayout before each RenderUpdate, avoids concurrent use during the
// update, and observes FinalLayout after RenderUpdate returns.
type VulkanBorrowedTextureDescriptor struct {
	Extent RenderTargetExtent
	// PhysicalWidth and PhysicalHeight are the image's size in device pixels,
	// stated rather than derived from Extent because its owner sizes it.
	PhysicalWidth  uint32
	PhysicalHeight uint32
	Context        VulkanContextDescriptor
	Image          VulkanHandle
	ImageView      VulkanHandle
	Format         uint32
	InitialLayout  uint32
	FinalLayout    uint32
}

// OpenGLOwnedTextureDescriptor describes an OpenGL session-owned texture render target.
type OpenGLOwnedTextureDescriptor struct {
	Extent  RenderTargetExtent
	Context OpenGLContextDescriptor
}

// OpenGLBorrowedTextureDescriptor describes an OpenGL caller-owned texture render target.
type OpenGLBorrowedTextureDescriptor struct {
	Extent RenderTargetExtent
	// PhysicalWidth and PhysicalHeight are the texture's size in device pixels,
	// stated rather than derived from Extent because its owner sizes it.
	PhysicalWidth  uint32
	PhysicalHeight uint32
	Context        OpenGLContextDescriptor
	Texture        uint32
	Target         uint32
}

// RenderDriver selects native render execution placement.
type RenderDriver uint32

const (
	RenderDriverCoreWorker           RenderDriver = RenderDriver(C.MLN_RENDER_DRIVER_CORE_WORKER)
	RenderDriverCallerGraphicsThread RenderDriver = RenderDriver(C.MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD)
)

// RenderSessionAttachOptions configures execution placement and an owned-texture ring.
type RenderSessionAttachOptions struct {
	Driver                    RenderDriver
	RequestedTextureRingDepth uint32
}

// NewRenderSessionAttachOptions returns core-worker attachment defaults.
func NewRenderSessionAttachOptions() RenderSessionAttachOptions {
	return RenderSessionAttachOptions{Driver: RenderDriverCoreWorker}
}

func (options RenderSessionAttachOptions) toC() C.mln_render_session_attach_options {
	raw := C.mln_render_session_attach_options_default()
	if options.Driver != 0 {
		raw.driver = C.uint32_t(options.Driver)
	}
	raw.requested_texture_ring_depth = C.uint32_t(options.RequestedTextureRingDepth)
	return raw
}

// RenderSessionCapability is a render-session capability bit.
type RenderSessionCapability uint32

const (
	RenderSessionCapabilityFrameAcquisition RenderSessionCapability = RenderSessionCapability(C.MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION)
	RenderSessionCapabilityReadback         RenderSessionCapability = RenderSessionCapability(C.MLN_RENDER_SESSION_CAPABILITY_READBACK)
	RenderSessionCapabilityConsumerSync     RenderSessionCapability = RenderSessionCapability(C.MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC)
	RenderSessionCapabilityPresentation     RenderSessionCapability = RenderSessionCapability(C.MLN_RENDER_SESSION_CAPABILITY_PRESENTATION)
)

// RenderSessionCapabilities are immutable negotiated attachment capabilities.
type RenderSessionCapabilities struct {
	Driver           RenderDriver
	TextureRingDepth uint32
	Flags            RenderSessionCapability
}

// RenderSessionState is the published session lifecycle state.
type RenderSessionState uint32

const (
	RenderSessionAttaching  RenderSessionState = RenderSessionState(C.MLN_RENDER_SESSION_STATE_ATTACHING)
	RenderSessionAttached   RenderSessionState = RenderSessionState(C.MLN_RENDER_SESSION_STATE_ATTACHED)
	RenderSessionDetaching  RenderSessionState = RenderSessionState(C.MLN_RENDER_SESSION_STATE_DETACHING)
	RenderSessionDetached   RenderSessionState = RenderSessionState(C.MLN_RENDER_SESSION_STATE_DETACHED)
	RenderSessionTargetLost RenderSessionState = RenderSessionState(C.MLN_RENDER_SESSION_STATE_TARGET_LOST)
	RenderSessionAbandoned  RenderSessionState = RenderSessionState(C.MLN_RENDER_SESSION_STATE_ABANDONED)
)

// RenderSessionSnapshot is a copy of one session's published state.
type RenderSessionSnapshot struct {
	// State is the session lifecycle state this binding version names.
	State RenderSessionState
	// RawState is the same value as the C API reported it, so a host can read a
	// state this binding version does not name.
	RawState uint32
	// Driver is the driver kind that services this session's graphics work.
	Driver RenderDriver
	// LatestResult is the outcome of the most recent frame.
	LatestResult RenderResult
	// Extent is the render target extent the session renders at.
	Extent RenderTargetExtent
	// Generation counts published session snapshots.
	Generation uint64
	// MapUpdateGeneration is the map generation of the latest render update the
	// session took.
	MapUpdateGeneration uint64
	// RenderedUpdateGeneration is the map generation of the latest update the
	// session rendered.
	RenderedUpdateGeneration uint64
	// ExtentGeneration counts applied extent changes.
	ExtentGeneration uint64
	// FrameGeneration counts rendered frames.
	FrameGeneration uint64
	// LatestDemandToken is the Token of the most recent accepted demand.
	LatestDemandToken uint64
	// PendingDemandCount is how many accepted demands have no terminal result
	// yet.
	PendingDemandCount uint32
	// AcquiredFrameCount is how many texture ring slots are leased.
	AcquiredFrameCount uint32
	// TargetReady reports whether the driver owns a usable target.
	TargetReady bool
	// PendingChanges reports whether the map or the target changed since the
	// last rendered frame.
	PendingChanges bool
}

// FrameDemandFlag controls one nonblocking frame demand.
type FrameDemandFlag uint32

const (
	// FrameDemandIfNeeded renders only when the map or the target changed since
	// the last frame. A demand without it renders unconditionally.
	FrameDemandIfNeeded FrameDemandFlag = FrameDemandFlag(C.MLN_FRAME_DEMAND_IF_NEEDED)
	// FrameDemandPresent presents the rendered frame. A demand without it still
	// renders, and a presenting target keeps whatever it presented last.
	FrameDemandPresent FrameDemandFlag = FrameDemandFlag(C.MLN_FRAME_DEMAND_PRESENT)
)

// FrameDemand is one nonblocking frame request, copied by RequestFrame.
type FrameDemand struct {
	// Flags selects the demand's rendering and presentation policy.
	Flags FrameDemandFlag
	// Token is the host-chosen identity this demand's terminal result carries.
	Token uint64
	// CoalescingBoundary separates demands that must not coalesce: native
	// coalesces only demands that carry the same boundary.
	CoalescingBoundary uint64
	// TimeoutNS bounds how long native waits for the target, 0 for no bound.
	TimeoutNS uint64
}

// NewFrameDemand returns the native frame demand defaults: render if needed,
// nonpresenting, with no token, coalescing boundary, or timeout.
func NewFrameDemand() FrameDemand {
	raw := C.mln_frame_demand_default()
	return FrameDemand{
		Flags:              FrameDemandFlag(raw.flags),
		Token:              uint64(raw.token),
		CoalescingBoundary: uint64(raw.coalescing_boundary),
		TimeoutNS:          uint64(raw.timeout_ns),
	}
}

func (d FrameDemand) toC() C.mln_frame_demand {
	raw := C.mln_frame_demand_default()
	raw.flags = C.uint32_t(d.Flags)
	raw.token = C.uint64_t(d.Token)
	raw.coalescing_boundary = C.uint64_t(d.CoalescingBoundary)
	raw.timeout_ns = C.uint64_t(d.TimeoutNS)
	return raw
}

// RenderFrameResult is one terminal result for an accepted demand.
type RenderFrameResult struct {
	Disposition         RenderResult
	Token               uint64
	MapUpdateGeneration uint64
	ExtentGeneration    uint64
	FrameGeneration     uint64
	// NeedsRepaint reports whether the map asked for another frame while it
	// rendered this one, as during an ongoing camera transition. It is true
	// only when Disposition is RenderResultRendered and reads false for every
	// other outcome. This is the same signal a
	// RuntimeEventMapRenderFrameFinished event carries in its NeedsRepaint
	// field, delivered with the frame result so a host can re-arm its frame
	// loop without the runtime event round trip.
	NeedsRepaint bool
}

func (sync GPUSync) toC() C.mln_gpu_sync {
	raw := C.mln_gpu_sync_default()
	raw.kind = C.uint32_t(sync.Kind)
	raw.object = C.uint64_t(sync.Object)
	raw.value = C.uint64_t(sync.Value)
	return raw
}

func frameResultFromC(raw C.mln_render_frame_result) RenderFrameResult {
	return RenderFrameResult{
		Disposition:         RenderResult(raw.disposition),
		Token:               uint64(raw.token),
		MapUpdateGeneration: uint64(raw.map_update_generation),
		ExtentGeneration:    uint64(raw.extent_generation),
		FrameGeneration:     uint64(raw.frame_generation),
		NeedsRepaint:        bool(raw.needs_repaint),
	}
}

// GPUSyncKind identifies a producer or consumer completion primitive.
type GPUSyncKind uint32

const (
	GPUSyncCPUComplete             GPUSyncKind = GPUSyncKind(C.MLN_GPU_SYNC_CPU_COMPLETE)
	GPUSyncMetalSharedEvent        GPUSyncKind = GPUSyncKind(C.MLN_GPU_SYNC_METAL_SHARED_EVENT)
	GPUSyncVulkanTimelineSemaphore GPUSyncKind = GPUSyncKind(C.MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE)
	GPUSyncOpenGLFence             GPUSyncKind = GPUSyncKind(C.MLN_GPU_SYNC_OPENGL_FENCE)
	GPUSyncWebGPUToken             GPUSyncKind = GPUSyncKind(C.MLN_GPU_SYNC_WEBGPU_TOKEN)
)

// GPUSync is a copied backend synchronization payload. Object is the bit
// pattern of the backend object that Kind names: the id<MTLSharedEvent>
// pointer, the VkSemaphore handle, the GLsync pointer, or the WebGPU token. A
// Vulkan handle stays 64 bits wide even where a pointer is not, so the field is
// uint64 rather than a pointer-width value; convert a NativePointer or a
// VulkanHandle into it. Object is zero when Kind is GPUSyncCPUComplete.
type GPUSync struct {
	Kind   GPUSyncKind
	Object uint64
	Value  uint64
}

// RenderSessionHandle owns a map render session. Control methods are safe from
// any goroutine; only ServiceDriverWork and OpenGL frame access require the
// caller graphics thread.
type RenderSessionHandle struct {
	state *handle.State[nativeRenderSession]
}

func destroyRenderSessionHandle(native nativeRenderSession) error {
	return checkNative(func() int32 {
		return int32(C.mln_render_session_destroy(C.mln_render_session(native)))
	})
}

// RenderAbandonDisposition reports whether graphics resources were quarantined.
type RenderAbandonDisposition uint32

const (
	RenderAbandonClean       RenderAbandonDisposition = RenderAbandonDisposition(C.MLN_RENDER_ABANDON_DISPOSITION_CLEAN)
	RenderAbandonQuarantined RenderAbandonDisposition = RenderAbandonDisposition(C.MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED)
)

// RenderAbandonResult is the result of irreversible CPU-side abandonment.
type RenderAbandonResult struct {
	Disposition              RenderAbandonDisposition
	QuarantinedResourceCount uint32
}

// AcquiredFrame leases one owned-texture ring slot. Release consumes the lease.
type AcquiredFrame struct {
	state *handle.State[nativeAcquiredFrame]
}

// WebGPUOwnedTextureFrameInfo contains backend-native WebGPU frame metadata.
type WebGPUOwnedTextureFrameInfo struct {
	Generation  uint64
	Width       uint32
	Height      uint32
	ScaleFactor float64
	FrameID     uint64
	Texture     NativePointer
	TextureView NativePointer
	Device      NativePointer
	Format      uint32
}

// MetalOwnedTextureFrameInfo contains backend-native Metal frame metadata.
type MetalOwnedTextureFrameInfo struct {
	Generation  uint64
	Width       uint32
	Height      uint32
	ScaleFactor float64
	FrameID     uint64
	Texture     NativePointer
	Device      NativePointer
	PixelFormat uint64
}

// VulkanOwnedTextureFrameInfo contains backend-native Vulkan frame metadata.
type VulkanOwnedTextureFrameInfo struct {
	Generation  uint64
	Width       uint32
	Height      uint32
	ScaleFactor float64
	FrameID     uint64
	Image       VulkanHandle
	ImageView   VulkanHandle
	Device      NativePointer
	Format      uint32
	Layout      uint32
}

// OpenGLOwnedTextureFrameInfo contains backend-native OpenGL frame metadata.
type OpenGLOwnedTextureFrameInfo struct {
	Generation     uint64
	Width          uint32
	Height         uint32
	ScaleFactor    float64
	FrameID        uint64
	Texture        uint32
	Target         uint32
	InternalFormat uint32
	Format         uint32
	Type           uint32
}

func (extent RenderTargetExtent) validate() error {
	if math.IsNaN(extent.ScaleFactor) || math.IsInf(extent.ScaleFactor, 0) || extent.ScaleFactor <= 0 {
		return newBindingError(ErrInvalidArgument, "render target scale factor must be positive and finite")
	}
	return nil
}

func (extent RenderTargetExtent) toC() C.mln_render_target_extent {
	return C.mln_render_target_extent{
		size:         C.uint32_t(unsafe.Sizeof(C.mln_render_target_extent{})),
		width:        C.uint32_t(extent.Width),
		height:       C.uint32_t(extent.Height),
		scale_factor: C.double(extent.ScaleFactor),
	}
}

// PhysicalSize returns the extent's physical device-pixel size as
// ceil(logical * ScaleFactor) per dimension. Surface and session-owned texture
// targets are sized this way; borrowed texture targets state their physical
// size instead.
func (extent RenderTargetExtent) PhysicalSize() (width uint32, height uint32, err error) {
	raw := extent.toC()
	var rawWidth, rawHeight C.uint32_t
	if err := checkNative(func() int32 {
		return int32(C.mln_render_target_extent_physical_size(&raw, &rawWidth, &rawHeight))
	}); err != nil {
		return 0, 0, err
	}
	return uint32(rawWidth), uint32(rawHeight), nil
}

func textureImageInfoFromC(info C.mln_texture_image_info) TextureImageInfo {
	return TextureImageInfo{Width: uint32(info.width), Height: uint32(info.height), Stride: uint32(info.stride), ByteLength: uint64(info.byte_length)}
}

func cPointer(pointer NativePointer) unsafe.Pointer {
	return C.mln_go_handle_to_pointer(C.uintptr_t(pointer))
}

func (descriptor MetalSurfaceDescriptor) toC() C.mln_metal_surface_descriptor {
	raw := C.mln_metal_surface_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.context.device = cPointer(descriptor.Context.Device)
	raw.layer = cPointer(descriptor.Layer)
	return raw
}

func (descriptor VulkanSurfaceDescriptor) toC() C.mln_vulkan_surface_descriptor {
	raw := C.mln_vulkan_surface_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.context = descriptor.Context.toC()
	raw.surface = C.mln_vulkan_non_dispatchable_handle(descriptor.Surface)
	return raw
}

func (descriptor OpenGLSurfaceDescriptor) toC() C.mln_opengl_surface_descriptor {
	raw := C.mln_opengl_surface_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.context = descriptor.Context.toC()
	raw.surface = cPointer(descriptor.Surface)
	return raw
}

func (descriptor MetalOwnedTextureDescriptor) toC() C.mln_metal_owned_texture_descriptor {
	raw := C.mln_metal_owned_texture_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.context.device = cPointer(descriptor.Context.Device)
	return raw
}

func (descriptor MetalBorrowedTextureDescriptor) toC() C.mln_metal_borrowed_texture_descriptor {
	raw := C.mln_metal_borrowed_texture_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.physical_width = C.uint32_t(descriptor.PhysicalWidth)
	raw.physical_height = C.uint32_t(descriptor.PhysicalHeight)
	raw.texture = cPointer(descriptor.Texture)
	return raw
}

func (descriptor VulkanOwnedTextureDescriptor) toC() C.mln_vulkan_owned_texture_descriptor {
	raw := C.mln_vulkan_owned_texture_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.context = descriptor.Context.toC()
	return raw
}

func (descriptor VulkanBorrowedTextureDescriptor) toC() C.mln_vulkan_borrowed_texture_descriptor {
	raw := C.mln_vulkan_borrowed_texture_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.physical_width = C.uint32_t(descriptor.PhysicalWidth)
	raw.physical_height = C.uint32_t(descriptor.PhysicalHeight)
	raw.context = descriptor.Context.toC()
	raw.image = C.mln_vulkan_non_dispatchable_handle(descriptor.Image)
	raw.image_view = C.mln_vulkan_non_dispatchable_handle(descriptor.ImageView)
	raw.format = C.uint32_t(descriptor.Format)
	raw.initial_layout = C.uint32_t(descriptor.InitialLayout)
	raw.final_layout = C.uint32_t(descriptor.FinalLayout)
	return raw
}

func (descriptor OpenGLOwnedTextureDescriptor) toC() C.mln_opengl_owned_texture_descriptor {
	raw := C.mln_opengl_owned_texture_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.context = descriptor.Context.toC()
	return raw
}

func (context WebGPUContextDescriptor) toC() C.mln_webgpu_context_descriptor {
	return C.mln_webgpu_context_descriptor{
		size:     C.uint32_t(unsafe.Sizeof(C.mln_webgpu_context_descriptor{})),
		instance: cPointer(context.Instance),
		device:   cPointer(context.Device),
		queue:    cPointer(context.Queue),
	}
}

func (descriptor WebGPUSurfaceDescriptor) toC() C.mln_webgpu_surface_descriptor {
	raw := C.mln_webgpu_surface_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.context = descriptor.Context.toC()
	raw.surface = cPointer(descriptor.Surface)
	raw.format = C.uint32_t(descriptor.Format)
	return raw
}

func (descriptor WebGPUOwnedTextureDescriptor) toC() C.mln_webgpu_owned_texture_descriptor {
	raw := C.mln_webgpu_owned_texture_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.context = descriptor.Context.toC()
	return raw
}

func (descriptor WebGPUBorrowedTextureDescriptor) toC() C.mln_webgpu_borrowed_texture_descriptor {
	raw := C.mln_webgpu_borrowed_texture_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.physical_width = C.uint32_t(descriptor.PhysicalWidth)
	raw.physical_height = C.uint32_t(descriptor.PhysicalHeight)
	raw.context = descriptor.Context.toC()
	raw.texture = cPointer(descriptor.Texture)
	raw.texture_view = cPointer(descriptor.TextureView)
	raw.format = C.uint32_t(descriptor.Format)
	return raw
}

func (descriptor OpenGLBorrowedTextureDescriptor) toC() C.mln_opengl_borrowed_texture_descriptor {
	raw := C.mln_opengl_borrowed_texture_descriptor_default()
	raw.extent = descriptor.Extent.toC()
	raw.physical_width = C.uint32_t(descriptor.PhysicalWidth)
	raw.physical_height = C.uint32_t(descriptor.PhysicalHeight)
	raw.context = descriptor.Context.toC()
	raw.texture = C.uint32_t(descriptor.Texture)
	raw.target = C.uint32_t(descriptor.Target)
	return raw
}

func (context VulkanContextDescriptor) toC() C.mln_vulkan_context_descriptor {
	return C.mln_vulkan_context_descriptor{
		size:                        C.uint32_t(unsafe.Sizeof(C.mln_vulkan_context_descriptor{})),
		instance:                    cPointer(context.Instance),
		physical_device:             cPointer(context.PhysicalDevice),
		device:                      cPointer(context.Device),
		graphics_queue:              cPointer(context.GraphicsQueue),
		graphics_queue_family_index: C.uint32_t(context.GraphicsQueueFamilyIndex),
		get_instance_proc_addr:      cPointer(context.GetInstanceProcAddr),
		get_device_proc_addr:        cPointer(context.GetDeviceProcAddr),
	}
}

func (context OpenGLContextDescriptor) toC() C.mln_opengl_context_descriptor {
	raw := C.mln_opengl_context_descriptor{size: C.uint32_t(unsafe.Sizeof(C.mln_opengl_context_descriptor{}))}
	raw.ownership = C.mln_opengl_context_ownership(context.Ownership)
	if context.WGL != nil {
		C.mln_go_opengl_context_set_wgl(&raw, cPointer(context.WGL.DeviceContext), cPointer(context.WGL.ShareContext), cPointer(context.WGL.GetProcAddress))
		return raw
	}
	if context.EGL != nil {
		C.mln_go_opengl_context_set_egl(&raw, cPointer(context.EGL.Display), cPointer(context.EGL.Config), cPointer(context.EGL.ShareContext), C.mln_opengl_client_api(context.EGL.ClientAPI), cPointer(context.EGL.GetProcAddress))
		return raw
	}
	if context.WebGL != nil {
		var selector *C.char
		if context.WebGL.CanvasSelector != "" {
			selector = (*C.char)(unsafe.Pointer(unsafe.StringData(context.WebGL.CanvasSelector)))
		}
		C.mln_go_opengl_context_set_webgl(&raw, C.mln_webgl_context_kind(context.WebGL.Kind), C.int32_t(context.WebGL.Context), selector, C.size_t(len(context.WebGL.CanvasSelector)))
		return raw
	}
	return raw
}
