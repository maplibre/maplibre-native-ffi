use std::ffi::c_char;

use crate::{LogEvent, LogSeverity, Result};

/// Copied MapLibre Native log record.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LogRecord {
    pub severity: LogSeverity,
    pub event: LogEvent,
    pub code: i64,
    pub message: String,
}

/// Copies a native log callback payload into an owned Rust log record.
///
/// # Safety
///
/// `message` must be either null or a valid NUL-terminated string pointer for
/// the duration of this call, as provided by the C logging callback contract.
pub unsafe fn copy_log_record(
    raw_severity: u32,
    raw_event: u32,
    code: i64,
    message: *const c_char,
) -> Result<LogRecord> {
    // SAFETY: The caller promises message follows the C logging callback contract.
    let message = unsafe { crate::string::copy_c_string(message) }?;
    Ok(LogRecord {
        severity: LogSeverity::from_raw(raw_severity),
        event: LogEvent::from_raw(raw_event),
        code,
        message,
    })
}

#[cfg(test)]
mod tests {
    use std::ffi::CString;

    use super::*;

    #[test]
    fn log_record_copies_message_and_preserves_unknown_domains() {
        let message = CString::new("hello").unwrap();
        // SAFETY: message is a valid NUL-terminated string for this call.
        let record = unsafe { copy_log_record(999, 998, -7, message.as_ptr()) }.unwrap();

        assert_eq!(record.severity, LogSeverity::Unknown(999));
        assert_eq!(record.event, LogEvent::Unknown(998));
        assert_eq!(record.code, -7);
        assert_eq!(record.message, "hello");
    }

    #[test]
    fn log_record_treats_null_message_as_empty() {
        // SAFETY: The C string helper treats a null log message as empty.
        let record = unsafe { copy_log_record(0, 0, 0, std::ptr::null()) }.unwrap();
        assert_eq!(record.message, "");
    }
}
