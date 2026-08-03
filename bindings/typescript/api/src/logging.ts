/**
 * Process-global logging.
 *
 * MapLibre logs from its worker, network, and rendering threads and asks the
 * callback whether it consumed the record. JavaScript cannot answer from those
 * threads, so the registration supplies one fixed answer and every record is
 * copied and delivered on the host's own execution context.
 */

import { NamedValue } from "./events.ts";
import { MLN_LOG_EVENT, MLN_LOG_SEVERITY } from "./raw/enums.ts";

/** How serious a log record is. */
export class LogSeverity extends NamedValue {
  static readonly info = new LogSeverity(
    MLN_LOG_SEVERITY.MLN_LOG_SEVERITY_INFO,
    "info",
  );
  static readonly warning = new LogSeverity(
    MLN_LOG_SEVERITY.MLN_LOG_SEVERITY_WARNING,
    "warning",
  );
  static readonly error = new LogSeverity(
    MLN_LOG_SEVERITY.MLN_LOG_SEVERITY_ERROR,
    "error",
  );

  static fromRawValue(rawValue: number): LogSeverity {
    return (
      [LogSeverity.info, LogSeverity.warning, LogSeverity.error].find(
        (value) => value.rawValue === rawValue,
      ) ?? new LogSeverity(rawValue, `unknown(${rawValue})`)
    );
  }
}

/** Which part of MapLibre produced a log record. */
export class LogEvent extends NamedValue {
  static readonly general = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_GENERAL,
    "general",
  );
  static readonly setup = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_SETUP,
    "setup",
  );
  static readonly shader = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_SHADER,
    "shader",
  );
  static readonly parseStyle = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_PARSE_STYLE,
    "parseStyle",
  );
  static readonly parseTile = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_PARSE_TILE,
    "parseTile",
  );
  static readonly render = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_RENDER,
    "render",
  );
  static readonly style = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_STYLE,
    "style",
  );
  static readonly database = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_DATABASE,
    "database",
  );
  static readonly httpRequest = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_HTTP_REQUEST,
    "httpRequest",
  );
  static readonly sprite = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_SPRITE,
    "sprite",
  );
  static readonly image = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_IMAGE,
    "image",
  );
  static readonly opengl = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_OPENGL,
    "opengl",
  );
  static readonly jni = new LogEvent(MLN_LOG_EVENT.MLN_LOG_EVENT_JNI, "jni");
  static readonly android = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_ANDROID,
    "android",
  );
  static readonly crash = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_CRASH,
    "crash",
  );
  static readonly glyph = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_GLYPH,
    "glyph",
  );
  static readonly timing = new LogEvent(
    MLN_LOG_EVENT.MLN_LOG_EVENT_TIMING,
    "timing",
  );

  static readonly #known: readonly LogEvent[] = [
    LogEvent.general,
    LogEvent.setup,
    LogEvent.shader,
    LogEvent.parseStyle,
    LogEvent.parseTile,
    LogEvent.render,
    LogEvent.style,
    LogEvent.database,
    LogEvent.httpRequest,
    LogEvent.sprite,
    LogEvent.image,
    LogEvent.opengl,
    LogEvent.jni,
    LogEvent.android,
    LogEvent.crash,
    LogEvent.glyph,
    LogEvent.timing,
  ];

  static fromRawValue(rawValue: number): LogEvent {
    return (
      LogEvent.#known.find((value) => value.rawValue === rawValue) ??
      new LogEvent(rawValue, `unknown(${rawValue})`)
    );
  }
}

/** One log record, copied out of the record native handed over. */
export interface LogRecord {
  readonly severity: LogSeverity;
  readonly event: LogEvent;
  /** A subsystem-specific code, or zero when the record carries none. */
  readonly code: bigint;
  readonly message: string;
}

export interface LogCallbackOptions {
  /**
   * The answer this registration reports to MapLibre for every record.
   *
   * MapLibre asks whether the host consumed the record so it can skip its own
   * logging. JavaScript runs after the answer is due, so the answer is fixed
   * here: `true` silences MapLibre's own output, and `false` leaves it.
   */
  readonly consume?: boolean;
}
