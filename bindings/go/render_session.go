package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

import (
	"runtime"
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

func (m *MapHandle) startRenderAttach(
	start func(C.mln_map, *C.mln_render_session, *C.mln_completion) int32,
) (*RenderSessionHandle, *Future[struct{}], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, nil, err
	}

	var session C.mln_render_session
	attach, err := startCompletion(func(completion *C.mln_completion) int32 {
		return start(C.mln_map(ptr), &session, completion)
	}, completionUnit)
	if err != nil {
		return nil, nil, err
	}
	state, err := handle.New(nativeRenderSession(session), "RenderSessionHandle")
	if err != nil {
		return nil, nil, newBindingError(ErrInvalidState, "render attach did not return a session")
	}

	result := &RenderSessionHandle{state: state}
	attach.retain(result)
	return result, attach, nil
}

// AttachMetalSurface starts attachment of a Metal surface render target. The
// session is returned in its attaching state, and the returned future completes
// once the driver owns the target. A session whose attachment failed still
// needs Detach or Abandon before Close.
//
// The session retains the CAMetalLayer for as long as it holds the target.
func (m *MapHandle) AttachMetalSurface(
	descriptor MetalSurfaceDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_metal_surface_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachVulkanSurface starts attachment of a Vulkan surface render target. See
// AttachMetalSurface for what the session and future report. The borrowed
// Vulkan context and surface handles stay valid until the session detaches or
// closes.
func (m *MapHandle) AttachVulkanSurface(
	descriptor VulkanSurfaceDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_vulkan_surface_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachOpenGLSurface starts attachment of an OpenGL surface render target. See
// AttachMetalSurface for what the session and future report. The borrowed
// context and platform surface handles stay valid until the session detaches or
// closes.
func (m *MapHandle) AttachOpenGLSurface(
	descriptor OpenGLSurfaceDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	if err := descriptor.Context.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	defer runtime.KeepAlive(descriptor)
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_opengl_surface_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachMetalOwnedTexture starts attachment of a Metal session-owned texture
// render target. See AttachMetalSurface for what the session and future report.
// The session allocates the texture ring, so it grants frame acquisition and
// readback.
func (m *MapHandle) AttachMetalOwnedTexture(
	descriptor MetalOwnedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_metal_owned_texture_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachMetalBorrowedTexture starts attachment of a Metal caller-owned texture
// render target. See AttachMetalSurface for what the session and future report.
// The caller keeps the texture valid until the next replacement, detach, or
// close, and synchronizes external use. The session grants neither frame
// acquisition nor readback.
func (m *MapHandle) AttachMetalBorrowedTexture(
	descriptor MetalBorrowedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_metal_borrowed_texture_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachVulkanOwnedTexture starts attachment of a Vulkan session-owned texture
// render target. See AttachMetalOwnedTexture for what a session-owned ring
// grants.
func (m *MapHandle) AttachVulkanOwnedTexture(
	descriptor VulkanOwnedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_vulkan_owned_texture_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachVulkanBorrowedTexture starts attachment of a Vulkan caller-owned
// texture render target. See AttachMetalBorrowedTexture for what a caller-owned
// target grants. The caller owns image lifetime, queue-family ownership, image
// layout transitions, and external synchronization around each frame.
func (m *MapHandle) AttachVulkanBorrowedTexture(
	descriptor VulkanBorrowedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_vulkan_borrowed_texture_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachOpenGLOwnedTexture starts attachment of an OpenGL session-owned texture
// render target. See AttachMetalOwnedTexture for what a session-owned ring
// grants.
func (m *MapHandle) AttachOpenGLOwnedTexture(
	descriptor OpenGLOwnedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	if err := descriptor.Context.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	defer runtime.KeepAlive(descriptor)
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_opengl_owned_texture_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachOpenGLBorrowedTexture starts attachment of an OpenGL caller-owned
// texture render target. See AttachMetalBorrowedTexture for what a caller-owned
// target grants. The texture belongs to the context this session attaches with
// or one in its share group.
func (m *MapHandle) AttachOpenGLBorrowedTexture(
	descriptor OpenGLBorrowedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	if err := descriptor.Context.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	defer runtime.KeepAlive(descriptor)
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_opengl_borrowed_texture_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachWebGPUSurface starts attachment of a WebGPU surface render target. See
// AttachMetalSurface for what the session and future report. The session
// configures the borrowed WGPUSurface for its device and extent, and
// unconfigures it when the session ends.
func (m *MapHandle) AttachWebGPUSurface(
	descriptor WebGPUSurfaceDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(
		m C.mln_map,
		session *C.mln_render_session,
		operation *C.mln_completion,
	) int32 {
		return int32(C.mln_webgpu_surface_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachWebGPUOwnedTexture starts attachment of a WebGPU session-owned texture
// render target. See AttachMetalOwnedTexture for what a session-owned ring
// grants.
func (m *MapHandle) AttachWebGPUOwnedTexture(
	descriptor WebGPUOwnedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(
		m C.mln_map,
		session *C.mln_render_session,
		operation *C.mln_completion,
	) int32 {
		return int32(C.mln_webgpu_owned_texture_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

// AttachWebGPUBorrowedTexture starts attachment of a WebGPU caller-owned
// texture render target. See AttachMetalBorrowedTexture for what a caller-owned
// target grants.
func (m *MapHandle) AttachWebGPUBorrowedTexture(
	descriptor WebGPUBorrowedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(
		m C.mln_map,
		session *C.mln_render_session,
		operation *C.mln_completion,
	) int32 {
		return int32(C.mln_webgpu_borrowed_texture_attach(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (s *RenderSessionHandle) ptr() (nativeRenderSession, error) {
	if s == nil || s.state == nil {
		return 0, newBindingError(ErrInvalidArgument, "RenderSessionHandle is nil")
	}
	value, live := s.state.Handle()
	if !live {
		return 0, newBindingError(ErrInvalidArgument, "RenderSessionHandle is closed")
	}
	return value, nil
}

func (s *RenderSessionHandle) startOperation(
	start func(C.mln_render_session, *C.mln_completion) int32,
) (*Future[struct{}], error) {
	return startRenderCompletion(s, start, completionUnit)
}

// Capabilities returns the driver kind, texture ring depth, and capability
// flags fixed when this session attached.
func (s *RenderSessionHandle) Capabilities() (RenderSessionCapabilities, error) {
	ptr, err := s.ptr()
	if err != nil {
		return RenderSessionCapabilities{}, err
	}

	var raw C.mln_render_session_capabilities
	raw.size = C.uint32_t(unsafe.Sizeof(raw))
	if err := checkNative(func() int32 {
		return int32(C.mln_render_session_get_capabilities(C.mln_render_session(ptr), &raw))
	}); err != nil {
		return RenderSessionCapabilities{}, err
	}
	return RenderSessionCapabilities{
		Driver:           RenderDriver(raw.driver),
		TextureRingDepth: uint32(raw.texture_ring_depth),
		Flags:            RenderSessionCapability(raw.flags),
	}, nil
}

// Snapshot returns a copy of this session's published state. It is callable
// from any goroutine.
func (s *RenderSessionHandle) Snapshot() (RenderSessionSnapshot, error) {
	ptr, err := s.ptr()
	if err != nil {
		return RenderSessionSnapshot{}, err
	}

	var raw C.mln_render_session_snapshot
	raw.size = C.uint32_t(unsafe.Sizeof(raw))
	if err := checkNative(func() int32 {
		return int32(C.mln_render_session_get_snapshot(C.mln_render_session(ptr), &raw))
	}); err != nil {
		return RenderSessionSnapshot{}, err
	}
	return RenderSessionSnapshot{
		State:        RenderSessionState(raw.state),
		RawState:     uint32(raw.state),
		Driver:       RenderDriver(raw.driver),
		LatestResult: RenderResult(raw.latest_result),
		Extent: RenderTargetExtent{
			Width:       uint32(raw.extent.width),
			Height:      uint32(raw.extent.height),
			ScaleFactor: float64(raw.extent.scale_factor),
		},
		Generation:               uint64(raw.generation),
		MapUpdateGeneration:      uint64(raw.map_update_generation),
		RenderedUpdateGeneration: uint64(raw.rendered_update_generation),
		ExtentGeneration:         uint64(raw.extent_generation),
		FrameGeneration:          uint64(raw.frame_generation),
		LatestDemandToken:        uint64(raw.latest_demand_token),
		PendingDemandCount:       uint32(raw.pending_demand_count),
		AcquiredFrameCount:       uint32(raw.acquired_frame_count),
		TargetReady:              bool(raw.target_ready),
		PendingChanges:           bool(raw.pending_changes),
	}, nil
}

// RequestFrame submits one nonblocking frame demand. Every accepted demand
// produces one terminal result, which DrainFrameResults reports. A demand that
// clears FrameDemandPresent still renders, and a presenting target keeps
// whatever it presented last.
//
// It reports ErrInvalidArgument for a flags bit outside FrameDemandFlag and
// ErrInvalidState when the session is not attached.
func (s *RenderSessionHandle) RequestFrame(demand FrameDemand) error {
	ptr, err := s.ptr()
	if err != nil {
		return err
	}

	raw := demand.toC()
	return checkNative(func() int32 {
		return int32(C.mln_render_session_request_frame(C.mln_render_session(ptr), &raw))
	})
}

// ServiceDriverWork runs up to maxWork queued driver calls on the calling
// thread and reports how many ran; zero runs every call currently queued. The
// target context must be current on that thread, and the first successful call
// fixes the session's graphics-thread identity.
//
// It reports ErrWrongThread from any later thread, ErrBusy while another driver
// call is in flight, and ErrInvalidState for a session its own core worker
// drives.
func (s *RenderSessionHandle) ServiceDriverWork(maxWork int) (int, error) {
	if maxWork < 0 {
		return 0, newBindingError(ErrInvalidArgument, "max work is negative")
	}
	ptr, err := s.ptr()
	if err != nil {
		return 0, err
	}

	var serviced C.size_t
	err = checkNative(func() int32 {
		return int32(C.mln_render_session_service_driver_work(
			C.mln_render_session(ptr),
			C.size_t(maxWork),
			&serviced,
		))
	})
	return int(serviced), err
}

// DrainFrameResults copies every queued terminal frame result out of native
// storage, in queue order, and releases the native batch before returning. An
// empty queue is not an error: the call reports no results and the caller
// retries after the next demand.
func (s *RenderSessionHandle) DrainFrameResults() ([]RenderFrameResult, error) {
	ptr, err := s.ptr()
	if err != nil {
		return nil, err
	}

	defer s.state.KeepAlive()

	var batch C.mln_render_frame_batch
	status := int32(C.mln_render_session_drain_frame_results(C.mln_render_session(ptr), &batch))
	if status == int32(C.MLN_STATUS_NOT_READY) {
		return nil, nil
	}
	if err := checkNative(func() int32 { return status }); err != nil {
		return nil, err
	}
	defer C.mln_render_frame_batch_release(batch)

	var count C.size_t
	if err := checkNative(func() int32 {
		return int32(C.mln_render_frame_batch_count(batch, &count))
	}); err != nil {
		return nil, err
	}
	results := make([]RenderFrameResult, int(count))
	for i := range results {
		var raw C.mln_render_frame_result
		raw.size = C.uint32_t(unsafe.Sizeof(raw))
		if err := checkNative(func() int32 {
			return int32(C.mln_render_frame_batch_get(batch, C.size_t(i), &raw))
		}); err != nil {
			return nil, err
		}
		results[i] = frameResultFromC(raw)
	}
	return results, nil
}

// AcquireFrame leases the newest rendered owned-texture ring slot. It reports
// ErrNotReady when no rendered frame is available, ErrUnsupported when the
// session grants no frame acquisition, and ErrInvalidState when the session is
// not attached. Release the returned frame to return the slot to the ring.
func (s *RenderSessionHandle) AcquireFrame() (*AcquiredFrame, error) {
	ptr, err := s.ptr()
	if err != nil {
		return nil, err
	}

	defer s.state.KeepAlive()

	var frame C.mln_acquired_frame
	if err := checkNative(func() int32 {
		return int32(C.mln_render_session_acquire_frame(C.mln_render_session(ptr), &frame))
	}); err != nil {
		return nil, err
	}
	state, err := handle.New(nativeAcquiredFrame(frame), "AcquiredFrame")
	if err != nil {
		return nil, newBindingError(ErrInvalidState, "frame acquisition did not return a frame")
	}
	return &AcquiredFrame{state: state}, nil
}

func (f *AcquiredFrame) use(call func(C.mln_acquired_frame) error) error {
	if f == nil || f.state == nil {
		return newBindingError(ErrInvalidArgument, "AcquiredFrame is nil")
	}
	raw, live := f.state.Handle()
	if !live {
		return newBindingError(ErrInvalidArgument, "AcquiredFrame is released")
	}
	defer f.state.KeepAlive()
	return call(C.mln_acquired_frame(raw))
}

// Result returns the frame metadata common to every backend.
func (f *AcquiredFrame) Result() (RenderFrameResult, error) {
	var result RenderFrameResult
	err := f.use(func(frame C.mln_acquired_frame) error {
		var raw C.mln_render_frame_result
		raw.size = C.uint32_t(unsafe.Sizeof(raw))
		if err := checkNative(func() int32 {
			return int32(C.mln_acquired_frame_get_result(frame, &raw))
		}); err != nil {
			return err
		}
		result = frameResultFromC(raw)
		return nil
	})
	return result, err
}

// ProducerSync returns the producer work a consumer waits for before it reads
// this frame's texture.
func (f *AcquiredFrame) ProducerSync() (GPUSync, error) {
	var sync GPUSync
	err := f.use(func(frame C.mln_acquired_frame) error {
		var raw C.mln_gpu_sync
		raw.size = C.uint32_t(unsafe.Sizeof(raw))
		if err := checkNative(func() int32 {
			return int32(C.mln_acquired_frame_get_producer_sync(frame, &raw))
		}); err != nil {
			return err
		}
		sync = GPUSync{
			Kind:   GPUSyncKind(raw.kind),
			Object: uint64(raw.object),
			Value:  uint64(raw.value),
		}
		return nil
	})
	return sync, err
}

// Release returns this lease to the ring, publishing sync as the consumer work
// native must wait for. Releasing an already released frame is a no-op.
func (f *AcquiredFrame) Release(sync GPUSync) error {
	if f == nil || f.state == nil {
		return newBindingError(ErrInvalidArgument, "AcquiredFrame is nil")
	}
	rawSync := sync.toC()
	return f.state.Close(func(native nativeAcquiredFrame) error {
		frame := C.mln_acquired_frame(native)
		if err := checkNative(func() int32 {
			return int32(C.mln_acquired_frame_release(&frame, &rawSync))
		}); err != nil {
			return err
		}
		if frame != 0 {
			return newBindingError(ErrInvalidState, "frame release did not consume the frame")
		}
		return nil
	})
}

// MetalTexture returns this frame's Metal texture metadata.
func (f *AcquiredFrame) MetalTexture() (MetalOwnedTextureFrameInfo, error) {
	var info MetalOwnedTextureFrameInfo
	err := f.use(func(frame C.mln_acquired_frame) error {
		var raw C.mln_metal_owned_texture_frame
		raw.size = C.uint32_t(unsafe.Sizeof(raw))
		if err := checkNative(func() int32 {
			return int32(C.mln_acquired_frame_get_metal_texture(frame, &raw))
		}); err != nil {
			return err
		}
		info = MetalOwnedTextureFrameInfo{
			Generation:  uint64(raw.generation),
			Width:       uint32(raw.width),
			Height:      uint32(raw.height),
			ScaleFactor: float64(raw.scale_factor),
			FrameID:     uint64(raw.frame_id),
			Texture:     NativePointer(uintptr(raw.texture)),
			Device:      NativePointer(uintptr(raw.device)),
			PixelFormat: uint64(raw.pixel_format),
		}
		return nil
	})
	return info, err
}

// VulkanTexture returns this frame's Vulkan image metadata.
func (f *AcquiredFrame) VulkanTexture() (VulkanOwnedTextureFrameInfo, error) {
	var info VulkanOwnedTextureFrameInfo
	err := f.use(func(frame C.mln_acquired_frame) error {
		var raw C.mln_vulkan_owned_texture_frame
		raw.size = C.uint32_t(unsafe.Sizeof(raw))
		if err := checkNative(func() int32 {
			return int32(C.mln_acquired_frame_get_vulkan_texture(frame, &raw))
		}); err != nil {
			return err
		}
		info = VulkanOwnedTextureFrameInfo{
			Generation:  uint64(raw.generation),
			Width:       uint32(raw.width),
			Height:      uint32(raw.height),
			ScaleFactor: float64(raw.scale_factor),
			FrameID:     uint64(raw.frame_id),
			Image:       VulkanHandle(raw.image),
			ImageView:   VulkanHandle(raw.image_view),
			Device:      NativePointer(uintptr(raw.device)),
			Format:      uint32(raw.format),
			Layout:      uint32(raw.layout),
		}
		return nil
	})
	return info, err
}

// OpenGLTexture returns this frame's OpenGL texture metadata.
func (f *AcquiredFrame) OpenGLTexture() (OpenGLOwnedTextureFrameInfo, error) {
	var info OpenGLOwnedTextureFrameInfo
	err := f.use(func(frame C.mln_acquired_frame) error {
		var raw C.mln_opengl_owned_texture_frame
		raw.size = C.uint32_t(unsafe.Sizeof(raw))
		if err := checkNative(func() int32 {
			return int32(C.mln_acquired_frame_get_opengl_texture(frame, &raw))
		}); err != nil {
			return err
		}
		info = OpenGLOwnedTextureFrameInfo{
			Generation:     uint64(raw.generation),
			Width:          uint32(raw.width),
			Height:         uint32(raw.height),
			ScaleFactor:    float64(raw.scale_factor),
			FrameID:        uint64(raw.frame_id),
			Texture:        uint32(raw.texture),
			Target:         uint32(raw.target),
			InternalFormat: uint32(raw.internal_format),
			Format:         uint32(raw.format),
			Type:           uint32(raw._type),
		}
		return nil
	})
	return info, err
}

// WebGPUTexture returns this frame's WebGPU texture metadata.
func (f *AcquiredFrame) WebGPUTexture() (WebGPUOwnedTextureFrameInfo, error) {
	var info WebGPUOwnedTextureFrameInfo
	err := f.use(func(frame C.mln_acquired_frame) error {
		var raw C.mln_webgpu_owned_texture_frame
		raw.size = C.uint32_t(unsafe.Sizeof(raw))
		if err := checkNative(func() int32 {
			return int32(C.mln_acquired_frame_get_webgpu_texture(frame, &raw))
		}); err != nil {
			return err
		}
		info = WebGPUOwnedTextureFrameInfo{
			Generation:  uint64(raw.generation),
			Width:       uint32(raw.width),
			Height:      uint32(raw.height),
			ScaleFactor: float64(raw.scale_factor),
			FrameID:     uint64(raw.frame_id),
			Texture:     NativePointer(uintptr(raw.texture)),
			TextureView: NativePointer(uintptr(raw.texture_view)),
			Device:      NativePointer(uintptr(raw.device)),
			Format:      uint32(raw.format),
		}
		return nil
	})
	return info, err
}

// Resize starts an ordered render target extent update. The completion runs
// after the driver applies the extent and updates the map viewport, and reports
// CommandDispositionSuperseded when a later resize replaced this one.
//
// The session keeps its renderer across a resize, along with the tile pyramid,
// glyph and image atlases, symbol placement, and map-owned feature state.
// RenderTargetExtent.ScaleFactor is fixed when the session attaches, because
// the renderer bakes its pixel ratio into compiled shaders: an extent that
// changes it reports ErrInvalidArgument, and a host that needs another scale
// factor attaches a new session.
//
// A caller-owned texture target reports ErrUnsupported, because its owner sizes
// it through the matching Set*BorrowedTextureTarget method. An acquired frame
// reports ErrInvalidState.
func (s *RenderSessionHandle) Resize(extent RenderTargetExtent) (*Future[struct{}], error) {
	if err := extent.validate(); err != nil {
		return nil, err
	}
	raw := extent.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_render_session_resize(session, &raw, operation))
	})
}

// Barrier starts an ordered operation that completes after every render
// operation accepted before it has a terminal result. A barrier requests no
// frame.
func (s *RenderSessionHandle) Barrier() (*Future[struct{}], error) {
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_render_session_barrier(session, operation))
	})
}

// ReduceMemoryUse starts best-effort release of this session's renderer caches.
func (s *RenderSessionHandle) ReduceMemoryUse() (*Future[struct{}], error) {
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_render_session_reduce_memory_use(session, operation))
	})
}

// ClearData starts renderer-data clearing.
func (s *RenderSessionHandle) ClearData() (*Future[struct{}], error) {
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_render_session_clear_data(session, operation))
	})
}

// DumpDebugLogs starts renderer diagnostic-log emission.
func (s *RenderSessionHandle) DumpDebugLogs() (*Future[struct{}], error) {
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_render_session_dump_debug_logs(session, operation))
	})
}

// Detach starts normal graphics-owner teardown and map detachment. The session
// stays live and still needs Close, and demands still outstanding report
// RenderResultTargetNotReady. An acquired frame leaves the session attached and
// reports ErrInvalidState.
func (s *RenderSessionHandle) Detach() (*Future[struct{}], error) {
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_render_session_detach(session, operation))
	})
}

// ReadPremultipliedRGBA8 starts a readback of the latest session-owned texture
// frame. The completed operation yields copied tightly packed premultiplied
// RGBA8 pixels and their image metadata.
func (s *RenderSessionHandle) ReadPremultipliedRGBA8() (*Future[TextureReadback], error) {
	return startRenderCompletion(s, func(session C.mln_render_session, completion *C.mln_completion) int32 {
		return int32(C.mln_texture_read_premultiplied_rgba8(session, completion))
	}, func(result *C.mln_completion_result) (TextureReadback, error) {
		raw, err := completionValue[C.mln_texture_readback_result](result)
		if err != nil {
			return TextureReadback{}, err
		}
		data, ok := goByteSlice(raw.data.data, raw.data.size)
		if !ok {
			return TextureReadback{}, newBindingError(ErrNative, "native texture readback is invalid")
		}
		return TextureReadback{Data: data, Info: textureImageInfoFromC(raw.info)}, nil
	})
}

// Abandon irreversibly closes control and mailboxes without a graphics call.
// It waits for the map's in-flight tile work before returning, so no library
// thread touches the session's target or device afterward and the host may
// destroy its graphics objects immediately. Do not call it from a MapLibre
// worker callback.
func (s *RenderSessionHandle) Abandon() (RenderAbandonResult, error) {
	ptr, err := s.ptr()
	if err != nil {
		return RenderAbandonResult{}, err
	}

	var raw C.mln_render_abandon_result
	raw.size = C.uint32_t(unsafe.Sizeof(raw))
	if err := checkNative(func() int32 {
		return int32(C.mln_render_session_abandon(C.mln_render_session(ptr), &raw))
	}); err != nil {
		return RenderAbandonResult{}, err
	}
	return RenderAbandonResult{
		Disposition:              RenderAbandonDisposition(raw.disposition),
		QuarantinedResourceCount: uint32(raw.quarantined_resource_count),
	}, nil
}

// Close destroys this session's native handle. A successful close makes later
// calls no-ops, and a failed close leaves the handle live so callers can retry.
// Close is callable from one of this session's own completions.
func (s *RenderSessionHandle) Close() error {
	if s == nil || s.state == nil {
		return newBindingError(ErrInvalidArgument, "RenderSessionHandle is nil")
	}
	return s.state.Close(destroyRenderSessionHandle)
}

// SetOpenGLBorrowedTextureTarget renders this attached texture session into a
// new caller-owned OpenGL texture. See SetMetalBorrowedTextureTarget for what
// replacing a target preserves. The replacement belongs to the context this
// session attached with or one in its share group, and that context is current
// on the calling thread.
func (s *RenderSessionHandle) SetOpenGLBorrowedTextureTarget(
	descriptor OpenGLBorrowedTextureDescriptor,
) (*Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	if err := descriptor.Context.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	defer runtime.KeepAlive(descriptor)
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_opengl_borrowed_texture_set_target(session, &raw, operation))
	})
}

// SetOpenGLSurfaceTarget presents this attached surface session through a new
// OpenGL surface. See SetMetalSurfaceTarget for what replacing a surface
// preserves. The new surface is made current on the next frame, so a host may
// hand over a replacement for one it has already destroyed.
func (s *RenderSessionHandle) SetOpenGLSurfaceTarget(
	descriptor OpenGLSurfaceDescriptor,
) (*Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	if err := descriptor.Context.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	defer runtime.KeepAlive(descriptor)
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_opengl_surface_set_target(session, &raw, operation))
	})
}

// SetMetalSurfaceTarget presents this attached surface session through a new
// Metal surface. Replacing the surface in place keeps this session's renderer,
// and with it the tile pyramid, glyph and image atlases, symbol placement, and
// feature state.
//
// A descriptor whose Context.Device is neither zero nor this session's device
// reports ErrInvalidArgument and leaves this session rendering into the surface
// it has. The session assigns the layer its own device and pixel format.
func (s *RenderSessionHandle) SetMetalSurfaceTarget(
	descriptor MetalSurfaceDescriptor,
) (*Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_metal_surface_set_target(session, &raw, operation))
	})
}

// SetVulkanSurfaceTarget presents this attached surface session through a new
// Vulkan surface. See SetMetalSurfaceTarget for what replacing a surface
// preserves.
//
// The outgoing VkSurfaceKHR stays valid, because this session holds a swapchain
// built from it; a host that must release its surface first closes this session
// and attaches again afterward. The replacement supports the color format and
// surface transform this session compiled its render pass and shaders for, and
// reports ErrUnsupported otherwise.
func (s *RenderSessionHandle) SetVulkanSurfaceTarget(
	descriptor VulkanSurfaceDescriptor,
) (*Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_vulkan_surface_set_target(session, &raw, operation))
	})
}

// SetMetalBorrowedTextureTarget renders this attached texture session into a
// new caller-owned Metal texture. Handing over a replacement keeps this
// session's renderer, unlike Resize, which reports ErrUnsupported for a
// caller-owned texture.
//
// The replacement belongs to the device this session attached with, which
// reports ErrInvalidArgument otherwise, and carries the pixel format it
// attached with, which reports ErrUnsupported otherwise; both leave this
// session rendering into the texture it has. The caller keeps the replacement
// valid until the next replacement, detach, or close. This session never
// retains or releases the outgoing texture.
func (s *RenderSessionHandle) SetMetalBorrowedTextureTarget(
	descriptor MetalBorrowedTextureDescriptor,
) (*Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_metal_borrowed_texture_set_target(session, &raw, operation))
	})
}

// SetVulkanBorrowedTextureTarget renders this attached texture session into a
// new caller-owned Vulkan image. See SetMetalBorrowedTextureTarget for what
// replacing a target preserves. The replacement carries the format and both
// layouts this session attached with, because its render pass was built around
// them.
func (s *RenderSessionHandle) SetVulkanBorrowedTextureTarget(
	descriptor VulkanBorrowedTextureDescriptor,
) (*Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_vulkan_borrowed_texture_set_target(session, &raw, operation))
	})
}

// SetWebGPUSurfaceTarget presents this attached surface session through a new
// WebGPU surface. See SetMetalSurfaceTarget for what replacing a surface
// preserves.
func (s *RenderSessionHandle) SetWebGPUSurfaceTarget(
	descriptor WebGPUSurfaceDescriptor,
) (*Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_webgpu_surface_set_target(session, &raw, operation))
	})
}

// SetWebGPUBorrowedTextureTarget renders this attached texture session into a
// new caller-owned WebGPU texture. See SetMetalBorrowedTextureTarget for what
// replacing a target preserves.
func (s *RenderSessionHandle) SetWebGPUBorrowedTextureTarget(
	descriptor WebGPUBorrowedTextureDescriptor,
) (*Future[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_completion) int32 {
		return int32(C.mln_webgpu_borrowed_texture_set_target(session, &raw, operation))
	})
}
