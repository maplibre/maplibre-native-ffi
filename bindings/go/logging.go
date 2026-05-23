package maplibre

import (
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
)

// LogSeverity is a native log severity.
type LogSeverity uint32

const (
	LogSeverityInfo    LogSeverity = LogSeverity(capi.LogSeverityInfo)
	LogSeverityWarning LogSeverity = LogSeverity(capi.LogSeverityWarning)
	LogSeverityError   LogSeverity = LogSeverity(capi.LogSeverityError)
)

// LogSeverityMask selects severities that native logging may dispatch
// asynchronously.
type LogSeverityMask uint32

const (
	LogSeverityMaskInfo    LogSeverityMask = LogSeverityMask(capi.LogSeverityMaskInfo)
	LogSeverityMaskWarning LogSeverityMask = LogSeverityMask(capi.LogSeverityMaskWarning)
	LogSeverityMaskError   LogSeverityMask = LogSeverityMask(capi.LogSeverityMaskError)
	LogSeverityMaskDefault LogSeverityMask = LogSeverityMask(capi.LogSeverityMaskDefault)
	LogSeverityMaskAll     LogSeverityMask = LogSeverityMask(capi.LogSeverityMaskAll)
)

// LogEvent is a native log category.
type LogEvent uint32

const (
	LogEventGeneral     LogEvent = LogEvent(capi.LogEventGeneral)
	LogEventSetup       LogEvent = LogEvent(capi.LogEventSetup)
	LogEventShader      LogEvent = LogEvent(capi.LogEventShader)
	LogEventParseStyle  LogEvent = LogEvent(capi.LogEventParseStyle)
	LogEventParseTile   LogEvent = LogEvent(capi.LogEventParseTile)
	LogEventRender      LogEvent = LogEvent(capi.LogEventRender)
	LogEventStyle       LogEvent = LogEvent(capi.LogEventStyle)
	LogEventDatabase    LogEvent = LogEvent(capi.LogEventDatabase)
	LogEventHTTPRequest LogEvent = LogEvent(capi.LogEventHTTPRequest)
	LogEventSprite      LogEvent = LogEvent(capi.LogEventSprite)
	LogEventImage       LogEvent = LogEvent(capi.LogEventImage)
	LogEventOpenGL      LogEvent = LogEvent(capi.LogEventOpenGL)
	LogEventJNI         LogEvent = LogEvent(capi.LogEventJNI)
	LogEventAndroid     LogEvent = LogEvent(capi.LogEventAndroid)
	LogEventCrash       LogEvent = LogEvent(capi.LogEventCrash)
	LogEventGlyph       LogEvent = LogEvent(capi.LogEventGlyph)
	LogEventTiming      LogEvent = LogEvent(capi.LogEventTiming)
)

// LogRecord is a copied native log record.
type LogRecord struct {
	Severity LogSeverity
	Event    LogEvent
	Code     int64
	Message  string
}

// LogCallback receives copied native log records. Native code may invoke it on
// worker threads. Returning true consumes the record. Returning false, panicking,
// or installing no callback lets MapLibre Native's platform logger handle it.
type LogCallback func(LogRecord) bool

// SetLogCallback installs or replaces the process-global native log callback.
func SetLogCallback(logCallback LogCallback) error {
	if logCallback == nil {
		return ClearLogCallback()
	}
	return checkNative(func() capi.Status {
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
	return checkNative(func() capi.Status {
		return callback.SetAsyncLogSeverityMask(uint32(mask))
	})
}
