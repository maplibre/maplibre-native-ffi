package maplibre

/*
#include <stdint.h>
#include "maplibre_native_c.h"

extern void mln_go_notification_callback(void* user_data);

static inline mln_status mln_go_notification_source_set_callback(
  mln_notification_source source, uintptr_t user_data
) {
  return mln_notification_source_set_callback(
    source, mln_go_notification_callback, (void*)user_data
  );
}
*/
import "C"

import (
	"runtime/cgo"
	"unsafe"
)

// NotificationEndpointKind identifies a ready receiver endpoint.
type NotificationEndpointKind uint32

const (
	NotificationEndpointRuntimeEvents           NotificationEndpointKind = NotificationEndpointKind(C.MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS)
	NotificationEndpointOperation               NotificationEndpointKind = NotificationEndpointKind(C.MLN_NOTIFICATION_ENDPOINT_OPERATION)
	NotificationEndpointAdapterResourceRequests NotificationEndpointKind = NotificationEndpointKind(C.MLN_NOTIFICATION_ENDPOINT_ADAPTER_RESOURCE_REQUESTS)
	NotificationEndpointAdapterLogRecords       NotificationEndpointKind = NotificationEndpointKind(C.MLN_NOTIFICATION_ENDPOINT_ADAPTER_LOG_RECORDS)
	NotificationEndpointRenderFrames            NotificationEndpointKind = NotificationEndpointKind(C.MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES)
	NotificationEndpointDriverWork              NotificationEndpointKind = NotificationEndpointKind(C.MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK)
)

// NotificationEndpoint is a copied ready endpoint.
type NotificationEndpoint struct {
	Kind NotificationEndpointKind
	ID   uint64
}

// SetNotificationCallback installs a callback that only schedules a later
// DrainReady call on the receiver's goroutine. Native code may invoke callback
// from any thread, and calls may coalesce.
func (runtime *RuntimeHandle) SetNotificationCallback(callback func()) error {
	if callback == nil {
		return runtime.ClearNotificationCallback()
	}
	_, release, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer release()
	handle := cgo.NewHandle(callback)
	runtime.notificationMu.Lock()
	defer runtime.notificationMu.Unlock()
	if err := checkNative(func() int32 {
		return int32(C.mln_go_notification_source_set_callback(
			C.mln_notification_source(runtime.notificationSource),
			C.uintptr_t(handle),
		))
	}); err != nil {
		handle.Delete()
		return err
	}
	if runtime.notificationCallback != 0 {
		cgo.Handle(runtime.notificationCallback).Delete()
	}
	runtime.notificationCallback = uintptr(handle)
	return nil
}

// ClearNotificationCallback waits for in-flight callback entries and removes
// the receiver callback. Ready endpoint state remains stored.
func (runtime *RuntimeHandle) ClearNotificationCallback() error {
	_, release, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer release()
	return runtime.clearNotificationCallback()
}

func (runtime *RuntimeHandle) clearNotificationCallback() error {
	runtime.notificationMu.Lock()
	defer runtime.notificationMu.Unlock()
	if runtime.notificationSource == 0 {
		return nil
	}
	if err := checkNative(func() int32 {
		return int32(C.mln_notification_source_clear_callback(C.mln_notification_source(runtime.notificationSource)))
	}); err != nil {
		return err
	}
	if runtime.notificationCallback != 0 {
		cgo.Handle(runtime.notificationCallback).Delete()
		runtime.notificationCallback = 0
	}
	return nil
}

// DrainReady returns a copied owned snapshot of ready endpoints.
func (runtime *RuntimeHandle) DrainReady() ([]NotificationEndpoint, error) {
	if _, release, err := runtime.ptr(); err != nil {
		return nil, err
	} else {
		defer release()
	}
	var batch C.mln_ready_batch
	if err := checkNative(func() int32 {
		return int32(C.mln_notification_source_drain_ready(C.mln_notification_source(runtime.notificationSource), &batch))
	}); err != nil {
		return nil, err
	}
	defer C.mln_ready_batch_release(batch)
	view := C.mln_ready_batch_view{size: C.uint32_t(unsafe.Sizeof(C.mln_ready_batch_view{}))}
	if err := checkNative(func() int32 { return int32(C.mln_ready_batch_get(batch, &view)) }); err != nil {
		return nil, err
	}
	if view.endpoint_count == 0 {
		return nil, nil
	}
	endpoints := make([]NotificationEndpoint, int(view.endpoint_count))
	for i := range endpoints {
		raw := (*C.mln_ready_endpoint)(unsafe.Add(
			unsafe.Pointer(view.endpoints),
			uintptr(i)*uintptr(view.endpoint_size),
		))
		endpoints[i] = NotificationEndpoint{
			Kind: NotificationEndpointKind(raw.kind),
			ID:   uint64(raw.id),
		}
	}
	return endpoints, nil
}

//export mln_go_notification_callback
func mln_go_notification_callback(userData unsafe.Pointer) {
	defer func() { _ = recover() }()
	handle := cgo.Handle(uintptr(userData))
	callback, ok := handle.Value().(func())
	if ok {
		callback()
	}
}
