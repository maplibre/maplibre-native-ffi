package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

import (
	"runtime"
	"unsafe"
)

func (m *MapHandle) startRenderAttach(
	start func(C.mln_map, *C.mln_render_session, *C.mln_operation) int32,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, nil, err
	}

	var session C.mln_render_session
	var operation C.mln_operation
	if err := checkNative(func() int32 {
		return start(C.mln_map(ptr), &session, &operation)
	}); err != nil {
		return nil, nil, err
	}
	if session == 0 || operation == 0 {
		if operation != 0 {
			C.mln_operation_release(operation)
		}
		if session != 0 {
			var abandoned C.mln_render_abandon_result
			abandoned.size = C.uint32_t(unsafe.Sizeof(abandoned))
			_ = C.mln_render_session_abandon(session, &abandoned)
			_ = C.mln_render_session_destroy(session)
		}
		return nil, nil, newBindingError(ErrInvalidState, "render attach did not return a session and operation")
	}

	result, err := newRenderSessionHandle(m, nativeRenderSession(session))
	if err != nil {
		C.mln_operation_release(operation)
		var abandoned C.mln_render_abandon_result
		abandoned.size = C.uint32_t(unsafe.Sizeof(abandoned))
		_ = C.mln_render_session_abandon(session, &abandoned)
		_ = C.mln_render_session_destroy(session)
		return nil, nil, err
	}
	attach := newOperationHandle[struct{}](
		m.runtime,
		uint64(operation),
		0,
		operationResultNone,
	)
	return result, attach, nil
}

func (m *MapHandle) AttachMetalSurface(
	descriptor MetalSurfaceDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_metal_surface_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachVulkanSurface(
	descriptor VulkanSurfaceDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_vulkan_surface_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachOpenGLSurface(
	descriptor OpenGLSurfaceDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	if err := descriptor.Context.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	defer runtime.KeepAlive(descriptor)
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_opengl_surface_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachMetalOwnedTexture(
	descriptor MetalOwnedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_metal_owned_texture_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachMetalBorrowedTexture(
	descriptor MetalBorrowedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_metal_borrowed_texture_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachVulkanOwnedTexture(
	descriptor VulkanOwnedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_vulkan_owned_texture_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachVulkanBorrowedTexture(
	descriptor VulkanBorrowedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_vulkan_borrowed_texture_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachOpenGLOwnedTexture(
	descriptor OpenGLOwnedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	if err := descriptor.Context.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	defer runtime.KeepAlive(descriptor)
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_opengl_owned_texture_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachOpenGLBorrowedTexture(
	descriptor OpenGLBorrowedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	if err := descriptor.Context.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	defer runtime.KeepAlive(descriptor)
	return m.startRenderAttach(func(m C.mln_map, session *C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_opengl_borrowed_texture_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachWebGPUSurface(
	descriptor WebGPUSurfaceDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(
		m C.mln_map,
		session *C.mln_render_session,
		operation *C.mln_operation,
	) int32 {
		return int32(C.mln_webgpu_surface_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachWebGPUOwnedTexture(
	descriptor WebGPUOwnedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(
		m C.mln_map,
		session *C.mln_render_session,
		operation *C.mln_operation,
	) int32 {
		return int32(C.mln_webgpu_owned_texture_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
	})
}

func (m *MapHandle) AttachWebGPUBorrowedTexture(
	descriptor WebGPUBorrowedTextureDescriptor,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, nil, err
	}
	rawDescriptor, rawOptions := descriptor.toC(), options.toC()
	return m.startRenderAttach(func(
		m C.mln_map,
		session *C.mln_render_session,
		operation *C.mln_operation,
	) int32 {
		return int32(C.mln_webgpu_borrowed_texture_attach_start(m, &rawDescriptor, &rawOptions, session, operation))
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
	start func(C.mln_render_session, *C.mln_operation) int32,
) (*OperationHandle[struct{}], error) {
	ptr, err := s.ptr()
	if err != nil {
		return nil, err
	}

	var operation C.mln_operation
	if err := checkNative(func() int32 {
		return start(C.mln_render_session(ptr), &operation)
	}); err != nil {
		return nil, err
	}
	if operation == 0 {
		return nil, newBindingError(ErrInvalidState, "render operation did not return an operation")
	}
	return newOperationHandle[struct{}](
		s.parent.runtime,
		uint64(operation),
		0,
		operationResultNone,
	), nil
}

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

func (s *RenderSessionHandle) DrainFrameResults() (*RenderFrameBatch, error) {
	ptr, err := s.ptr()
	if err != nil {
		return nil, err
	}

	var batch C.mln_render_frame_batch
	if err := checkNative(func() int32 {
		return int32(C.mln_render_session_drain_frame_results(
			C.mln_render_session(ptr),
			&batch,
		))
	}); err != nil {
		return nil, err
	}
	return &RenderFrameBatch{handle: uint64(batch)}, nil
}

func (b *RenderFrameBatch) Results() ([]RenderFrameResult, error) {
	if b == nil {
		return nil, newBindingError(ErrInvalidArgument, "RenderFrameBatch is nil")
	}
	b.mu.Lock()
	defer b.mu.Unlock()
	if b.closed {
		return nil, newBindingError(ErrInvalidArgument, "RenderFrameBatch is closed")
	}

	var count C.size_t
	if err := checkNative(func() int32 {
		return int32(C.mln_render_frame_batch_count(C.mln_render_frame_batch(b.handle), &count))
	}); err != nil {
		return nil, err
	}
	results := make([]RenderFrameResult, int(count))
	for i := range results {
		var raw C.mln_render_frame_result
		raw.size = C.uint32_t(unsafe.Sizeof(raw))
		if err := checkNative(func() int32 {
			return int32(C.mln_render_frame_batch_get(
				C.mln_render_frame_batch(b.handle),
				C.size_t(i),
				&raw,
			))
		}); err != nil {
			return nil, err
		}
		results[i] = frameResultFromC(raw)
	}
	return results, nil
}

func (b *RenderFrameBatch) Close() {
	if b == nil {
		return
	}
	b.mu.Lock()
	defer b.mu.Unlock()
	if b.closed {
		return
	}
	C.mln_render_frame_batch_release(C.mln_render_frame_batch(b.handle))
	b.closed = true
	b.handle = 0
}

func (s *RenderSessionHandle) AcquireFrame() (*AcquiredFrame, error) {
	ptr, err := s.ptr()
	if err != nil {
		return nil, err
	}

	var frame C.mln_acquired_frame
	if err := checkNative(func() int32 {
		return int32(C.mln_render_session_acquire_frame(C.mln_render_session(ptr), &frame))
	}); err != nil {
		return nil, err
	}
	if frame == 0 {
		return nil, newBindingError(ErrInvalidState, "frame acquisition did not return a frame")
	}
	return &AcquiredFrame{handle: uint64(frame)}, nil
}

func (f *AcquiredFrame) use(call func(C.mln_acquired_frame) error) error {
	if f == nil {
		return newBindingError(ErrInvalidArgument, "AcquiredFrame is nil")
	}
	f.mu.Lock()
	defer f.mu.Unlock()
	if f.closed {
		return newBindingError(ErrInvalidArgument, "AcquiredFrame is closed")
	}
	return call(C.mln_acquired_frame(f.handle))
}

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
			Object: NativePointer(uintptr(raw.object)),
			Value:  uint64(raw.value),
		}
		return nil
	})
	return sync, err
}

func (f *AcquiredFrame) Release(sync GPUSync) error {
	if f == nil {
		return newBindingError(ErrInvalidArgument, "AcquiredFrame is nil")
	}
	f.mu.Lock()
	defer f.mu.Unlock()
	if f.closed {
		return newBindingError(ErrInvalidArgument, "AcquiredFrame is closed")
	}

	frame := C.mln_acquired_frame(f.handle)
	rawSync := sync.toC()
	if err := checkNative(func() int32 {
		return int32(C.mln_acquired_frame_release(&frame, &rawSync))
	}); err != nil {
		return err
	}
	if frame != 0 {
		return newBindingError(ErrInvalidState, "frame release did not consume the frame")
	}
	f.closed = true
	f.handle = 0
	return nil
}

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
			Image:       NativePointer(uintptr(raw.image)),
			ImageView:   NativePointer(uintptr(raw.image_view)),
			Device:      NativePointer(uintptr(raw.device)),
			Format:      uint32(raw.format),
			Layout:      uint32(raw.layout),
		}
		return nil
	})
	return info, err
}

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

func (s *RenderSessionHandle) ResizeStart(extent RenderTargetExtent) (*OperationHandle[struct{}], error) {
	if err := extent.validate(); err != nil {
		return nil, err
	}
	raw := extent.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_render_session_resize_start(session, &raw, operation))
	})
}

func (s *RenderSessionHandle) BarrierStart() (*OperationHandle[struct{}], error) {
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_render_session_barrier_start(session, operation))
	})
}

func (s *RenderSessionHandle) ReduceMemoryUseStart() (*OperationHandle[struct{}], error) {
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_render_session_reduce_memory_use_start(session, operation))
	})
}

func (s *RenderSessionHandle) ClearDataStart() (*OperationHandle[struct{}], error) {
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_render_session_clear_data_start(session, operation))
	})
}

func (s *RenderSessionHandle) DumpDebugLogsStart() (*OperationHandle[struct{}], error) {
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_render_session_dump_debug_logs_start(session, operation))
	})
}

func (s *RenderSessionHandle) DetachStart() (*OperationHandle[struct{}], error) {
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_render_session_detach_start(session, operation))
	})
}

func (s *RenderSessionHandle) ReadPremultipliedRGBA8Start() (*OperationHandle[TextureReadback], error) {
	ptr, err := s.ptr()
	if err != nil {
		return nil, err
	}

	var rawOperation C.mln_operation
	if err := checkNative(func() int32 {
		return int32(C.mln_texture_read_premultiplied_rgba8_start(
			C.mln_render_session(ptr),
			&rawOperation,
		))
	}); err != nil {
		return nil, err
	}
	if rawOperation == 0 {
		return nil, newBindingError(ErrInvalidState, "texture readback did not return an operation")
	}
	operation := newOperationHandle[TextureReadback](
		s.parent.runtime,
		uint64(rawOperation),
		0,
		0,
	)
	operation.takeResult = func(id uint64) (TextureReadback, bool, error) {
		var buffer C.mln_buffer
		var info C.mln_texture_image_info
		info.size = C.uint32_t(unsafe.Sizeof(info))
		if err := checkNative(func() int32 {
			return int32(C.mln_texture_read_premultiplied_rgba8_take_result(
				C.mln_operation(id),
				&buffer,
				&info,
			))
		}); err != nil {
			return TextureReadback{}, false, err
		}
		if buffer == 0 {
			return TextureReadback{Info: textureImageInfoFromC(info)}, true, nil
		}
		defer C.mln_buffer_destroy(buffer)

		var view C.mln_buffer_view
		if err := checkNative(func() int32 {
			return int32(C.mln_buffer_get(buffer, &view))
		}); err != nil {
			return TextureReadback{}, true, err
		}
		data := append([]byte(nil), unsafe.Slice((*byte)(view.data), int(view.size))...)
		return TextureReadback{
			Data: data,
			Info: textureImageInfoFromC(info),
		}, true, nil
	}
	return operation, nil
}

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

func (s *RenderSessionHandle) Close() error {
	if s == nil || s.state == nil {
		return newBindingError(ErrInvalidArgument, "RenderSessionHandle is nil")
	}
	s.closeMu.Lock()
	defer s.closeMu.Unlock()
	if err := checkNative(func() int32 {
		return s.state.Close(destroyRenderSessionHandle)
	}); err != nil {
		return err
	}
	return nil
}

func (s *RenderSessionHandle) SetOpenGLBorrowedTextureTargetStart(
	descriptor OpenGLBorrowedTextureDescriptor,
) (*OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	if err := descriptor.Context.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	defer runtime.KeepAlive(descriptor)
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_opengl_borrowed_texture_set_target_start(session, &raw, operation))
	})
}

func (s *RenderSessionHandle) SetOpenGLSurfaceTargetStart(
	descriptor OpenGLSurfaceDescriptor,
) (*OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	if err := descriptor.Context.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	defer runtime.KeepAlive(descriptor)
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_opengl_surface_set_target_start(session, &raw, operation))
	})
}

func (s *RenderSessionHandle) SetMetalSurfaceTargetStart(
	descriptor MetalSurfaceDescriptor,
) (*OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_metal_surface_set_target_start(session, &raw, operation))
	})
}

func (s *RenderSessionHandle) SetVulkanSurfaceTargetStart(
	descriptor VulkanSurfaceDescriptor,
) (*OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_vulkan_surface_set_target_start(session, &raw, operation))
	})
}

func (s *RenderSessionHandle) SetMetalBorrowedTextureTargetStart(
	descriptor MetalBorrowedTextureDescriptor,
) (*OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_metal_borrowed_texture_set_target_start(session, &raw, operation))
	})
}

func (s *RenderSessionHandle) SetVulkanBorrowedTextureTargetStart(
	descriptor VulkanBorrowedTextureDescriptor,
) (*OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_vulkan_borrowed_texture_set_target_start(session, &raw, operation))
	})
}

func (s *RenderSessionHandle) SetWebGPUSurfaceTargetStart(
	descriptor WebGPUSurfaceDescriptor,
) (*OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_webgpu_surface_set_target_start(session, &raw, operation))
	})
}

func (s *RenderSessionHandle) SetWebGPUBorrowedTextureTargetStart(
	descriptor WebGPUBorrowedTextureDescriptor,
) (*OperationHandle[struct{}], error) {
	if err := descriptor.Extent.validate(); err != nil {
		return nil, err
	}
	raw := descriptor.toC()
	return s.startOperation(func(session C.mln_render_session, operation *C.mln_operation) int32 {
		return int32(C.mln_webgpu_borrowed_texture_set_target_start(session, &raw, operation))
	})
}
