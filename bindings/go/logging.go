package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"

// LogSeverity is a native log severity.
type LogSeverity uint32

const (
	LogSeverityInfo    LogSeverity = LogSeverity(C.MLN_LOG_SEVERITY_INFO)
	LogSeverityWarning LogSeverity = LogSeverity(C.MLN_LOG_SEVERITY_WARNING)
	LogSeverityError   LogSeverity = LogSeverity(C.MLN_LOG_SEVERITY_ERROR)
)

// LogSeverityMask selects severities that native logging may dispatch
// asynchronously.
type LogSeverityMask uint32

const (
	LogSeverityMaskInfo    LogSeverityMask = LogSeverityMask(C.MLN_LOG_SEVERITY_MASK_INFO)
	LogSeverityMaskWarning LogSeverityMask = LogSeverityMask(C.MLN_LOG_SEVERITY_MASK_WARNING)
	LogSeverityMaskError   LogSeverityMask = LogSeverityMask(C.MLN_LOG_SEVERITY_MASK_ERROR)
	LogSeverityMaskDefault LogSeverityMask = LogSeverityMask(C.MLN_LOG_SEVERITY_MASK_DEFAULT)
	LogSeverityMaskAll     LogSeverityMask = LogSeverityMask(C.MLN_LOG_SEVERITY_MASK_ALL)
)

// LogEvent is a native log category.
type LogEvent uint32

const (
	LogEventGeneral     LogEvent = LogEvent(C.MLN_LOG_EVENT_GENERAL)
	LogEventSetup       LogEvent = LogEvent(C.MLN_LOG_EVENT_SETUP)
	LogEventShader      LogEvent = LogEvent(C.MLN_LOG_EVENT_SHADER)
	LogEventParseStyle  LogEvent = LogEvent(C.MLN_LOG_EVENT_PARSE_STYLE)
	LogEventParseTile   LogEvent = LogEvent(C.MLN_LOG_EVENT_PARSE_TILE)
	LogEventRender      LogEvent = LogEvent(C.MLN_LOG_EVENT_RENDER)
	LogEventStyle       LogEvent = LogEvent(C.MLN_LOG_EVENT_STYLE)
	LogEventDatabase    LogEvent = LogEvent(C.MLN_LOG_EVENT_DATABASE)
	LogEventHTTPRequest LogEvent = LogEvent(C.MLN_LOG_EVENT_HTTP_REQUEST)
	LogEventSprite      LogEvent = LogEvent(C.MLN_LOG_EVENT_SPRITE)
	LogEventImage       LogEvent = LogEvent(C.MLN_LOG_EVENT_IMAGE)
	LogEventOpenGL      LogEvent = LogEvent(C.MLN_LOG_EVENT_OPENGL)
	LogEventJNI         LogEvent = LogEvent(C.MLN_LOG_EVENT_JNI)
	LogEventAndroid     LogEvent = LogEvent(C.MLN_LOG_EVENT_ANDROID)
	LogEventCrash       LogEvent = LogEvent(C.MLN_LOG_EVENT_CRASH)
	LogEventGlyph       LogEvent = LogEvent(C.MLN_LOG_EVENT_GLYPH)
	LogEventTiming      LogEvent = LogEvent(C.MLN_LOG_EVENT_TIMING)
)

// LogRecord is a copied native log record.
type LogRecord struct {
	Severity LogSeverity
	Event    LogEvent
	Code     int64
	Message  string
}

// LogCallback receives copied native log records. Native code may invoke it on
// worker threads or while internal logging locks are held. The callback must be
// thread-safe, return quickly, and must not call MapLibre APIs. Returning true
// consumes the record. Returning false, panicking, or installing no callback
// lets MapLibre Native's platform logger handle it.
type LogCallback func(LogRecord) bool

// SetLogCallback installs or replaces the process-global native log callback.
func SetLogCallback(logCallback LogCallback) error {
	if logCallback == nil {
		return ClearLogCallback()
	}
	return checkNative(func() int32 {
		return callback.SetLogCallback(func(severity uint32, event uint32, code int64, message string) bool {
			return logCallback(LogRecord{
				Severity: LogSeverity(severity),
				Event:    LogEvent(event),
				Code:     code,
				Message:  message,
			})
		})
	})
}

// ClearLogCallback clears the process-global native log callback.
func ClearLogCallback() error {
	return checkNative(callback.ClearLogCallback)
}

// SetAsyncLogSeverityMask controls which native log severities may dispatch
// asynchronously.
func SetAsyncLogSeverityMask(mask LogSeverityMask) error {
	return checkNative(func() int32 {
		return callback.SetAsyncLogSeverityMask(uint32(mask))
	})
}
