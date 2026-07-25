"use strict";

const root = require("./index.cjs");

exports.setLogCallback = root.setLogCallback;
exports.clearLogCallback = root.clearLogCallback;
exports.setAsyncLogSeverities = root.setAsyncLogSeverities;
exports.restoreDefaultAsyncLogSeverities =
  root.restoreDefaultAsyncLogSeverities;
